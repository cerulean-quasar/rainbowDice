/**
 * Copyright 2019 Cerulean Quasar. All Rights Reserved.
 *
 *  This file is part of RainbowDice.
 *
 *  RainbowDice is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RainbowDice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RainbowDice.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
package com.quasar.cerulean.rainbowdice;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.TypedArray;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.Parcelable;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.Locale;
import java.util.Objects;

import static com.quasar.cerulean.rainbowdice.Constants.DICE_CUSTOMIZATION_ACTIVITY;
import static com.quasar.cerulean.rainbowdice.Constants.DICE_THEME_SELECTION_ACTIVITY;
import static com.quasar.cerulean.rainbowdice.Constants.themeNameConfigValue;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        try {
            System.loadLibrary("native-lib");
        } catch (UnsatisfiedLinkError e) {
            System.out.println("Could not load library: native-lib: " + (e.getMessage() != null ? e.getMessage() : "<no error message>"));
        }
    }

    // for the texture dimensions.
    private static final int DICE_CONFIGURATION_ACTIVITY = 1;

    private Thread drawer = null;

    public class LogFileLoadedResult {
        public String name = null;
        public DieConfiguration[] diceConfig = null;
        public DiceResult diceResult = null;
    }

    private class GraphicsDescription {
        public boolean hasLinearAccelerationSensor;
        public boolean hasGravitySensor;
        public boolean hasAccelerometer;
        public boolean isVulkan;
        public String apiName;
        public String apiVersion;
        public String deviceName;

        public GraphicsDescription() {
            hasLinearAccelerationSensor = false;
            hasGravitySensor = false;
            hasAccelerometer = false;
            isVulkan = false;
            apiName = null;
            apiVersion = null;
            deviceName = null;
        }
    }

    private GraphicsDescription graphicsDescription = null;

    private LogFile logFile = null;
    private AssetManager assetManager = null;
    int logItemToLoad = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        DiceConfigurationManager configurationFile = new DiceConfigurationManager(this);

        String themeName = configurationFile.getTheme();
        int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
        setTheme(currentThemeId);

        super.onCreate(savedInstanceState);

        assetManager = getAssets();
        createDefaults(configurationFile);
        logFile = new LogFile(this);

        initGui(configurationFile);
    }

    void initGui(DiceConfigurationManager configurationFile) {
        setContentView(R.layout.activity_main);

        resetDiceList(configurationFile);

        // Need to do this here because we reloaded the layout.  When the layout gets reloaded,
        // the surface gets destroyed and so does its callback.
        SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
        drawSurfaceView.setZOrderOnTop(true);
        SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
        drawSurfaceHolder.setFormat(PixelFormat.TRANSPARENT);
        drawSurfaceHolder.addCallback(new MySurfaceCallback(this));
    }

    void resetDiceList(DiceConfigurationManager configurationFile) {
        LinkedList<String> diceList = configurationFile.getDiceList();
        LinearLayout layout = findViewById(R.id.diceList);
        layout.removeAllViews();

        for (String dice : diceList) {
            Button button = new Button(this);
            button.setBackgroundColor(0xa0a0a0a0);
            button.setText(dice);
            button.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    TextView result = findViewById(R.id.rollResult);
                    result.setText(getString(R.string.diceMessageToStartRolling));
                    Button b = (Button) v;
                    String diceFileName = b.getText().toString();
                    DieConfiguration[] diceConfig = loadFromFile(diceFileName);
                    if (diceConfig == null) {
                        return;
                    }
                    rollTheDice(diceConfig, diceFileName);
                }
            });
            layout.addView(button);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        //joinDrawer();
        logFile.writeFile();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();

        LogFileLoadedResult result = loadFromLog(logItemToLoad);
        if (result != null) {
            drawStoppedDice(result.name, result.diceConfig, result.diceResult);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        joinDrawer();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == DICE_CONFIGURATION_ACTIVITY) {
            resetDiceList(new DiceConfigurationManager(this));
        } else if (requestCode == DICE_THEME_SELECTION_ACTIVITY) {
            if (resultCode == RESULT_OK) {
                String themeName = data.getStringExtra(themeNameConfigValue);
                if (themeName != null && !themeName.isEmpty()) {
                    int resID = getResources().getIdentifier(themeName, "style", getPackageName());
                    setTheme(resID);
                    initGui(new DiceConfigurationManager(this));
                    TypedValue value = new TypedValue();
                    getTheme().resolveAttribute(android.R.attr.windowBackground, value, true);
                    getWindow().setBackgroundDrawableResource(value.resourceId);
                }
            }
        } else if (requestCode == DICE_CUSTOMIZATION_ACTIVITY) {
            resetDiceList(new DiceConfigurationManager(this));
        } else if (requestCode == Constants.DICE_LOG_FILE_ACTIVITY) {
            if (resultCode == RESULT_OK) {
                logItemToLoad = data.getIntExtra(Constants.LOG_ITEM_TO_LOAD, -1);
            }
        } else if (requestCode == Constants.DICE_EXPORT_FILE_ACTIVITY) {
            if (resultCode == RESULT_OK) {
                saveDiceToExternalFile(data.getData());
            }
        } else if (requestCode == Constants.DICE_IMPORT_FILE_ACTIVITY) {
            if (resultCode == RESULT_OK) {
                // close the log file so that the data can be overridden
                logFile = null;
                restoreDiceFromExternalFile(data.getData());
                logFile = new LogFile(this);

                resetDiceList(new DiceConfigurationManager(this));
                logItemToLoad = -1;
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_menu, menu);

        int bonus = (new DiceConfigurationManager(this)).bonus();
        MenuItem item = menu.findItem(R.id.addBonusItem).setTitle(
                String.format(Locale.getDefault(), getResources().getString(R.string.bonusReadOut), bonus));
        return true;
    }

    public void onExportClick(MenuItem item) {
        LinearLayout layout = findViewById(R.id.activity_main_top_level);
        LayoutInflater inflater = getLayoutInflater();

        final LinearLayout dialogView = (LinearLayout) inflater.inflate(
                R.layout.message_ok_cancel, layout, false);

        TextView message = dialogView.findViewById(R.id.message);
        message.setText(R.string.exportOK);
        final Dialog dialog = new AlertDialog.Builder(this)
                .setTitle(R.string.exportDice).setView(dialogView).show();

        Button button = dialogView.findViewById(R.id.cancelButton);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });

        button = dialogView.findViewById(R.id.okButton);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Choose a file to save the dice data to.
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);

                // Filter to only show results that can be "opened", such as a
                // file (as opposed to a list of contacts or timezones)
                intent.addCategory(Intent.CATEGORY_OPENABLE);

                // Search for all documents available via installed storage providers.
                // There is no document type for json.
                intent.setType("*/*");

                intent.putExtra(Intent.EXTRA_TITLE, Constants.EXPORT_FILE_NAME);
                startActivityForResult(intent, Constants.DICE_EXPORT_FILE_ACTIVITY);

                dialog.dismiss();
            }
        });
    }

    public void onImportClick(MenuItem item) {
        LinearLayout layout = findViewById(R.id.activity_main_top_level);
        LayoutInflater inflater = getLayoutInflater();

        final LinearLayout dialogView = (LinearLayout) inflater.inflate(
                R.layout.message_ok_cancel, layout, false);

        TextView message = dialogView.findViewById(R.id.message);
        message.setText(R.string.importOK);
        final Dialog dialog = new AlertDialog.Builder(this)
                .setTitle(R.string.importDice).setView(dialogView).show();

        Button button = dialogView.findViewById(R.id.cancelButton);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });

        button = dialogView.findViewById(R.id.okButton);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // Choose a file to save the dice data to.
                Intent intent = new Intent(Intent.ACTION_GET_CONTENT);

                // Filter to only show results that can be "opened", such as a
                // file (as opposed to a list of contacts or timezones)
                intent.addCategory(Intent.CATEGORY_OPENABLE);

                // Search for all documents available via installed storage providers.
                // There is no document type for json.
                intent.setType("*/*");

                intent.putExtra(Intent.EXTRA_TITLE, Constants.EXPORT_FILE_NAME);
                startActivityForResult(intent, Constants.DICE_IMPORT_FILE_ACTIVITY);
                dialog.dismiss();
            }
        });
    }

    public void onBonusClick(MenuItem item) {
        LinearLayout layout = findViewById(R.id.activity_main_top_level);
        LayoutInflater inflater = getLayoutInflater();

        final LinearLayout bonusView = (LinearLayout) inflater.inflate(
                R.layout.add_bonus_dialog, layout, false);

        final DiceConfigurationManager config = new DiceConfigurationManager(MainActivity.this);
        int bonus = config.bonus();

        EditText bonusEdit = bonusView.findViewById(R.id.bonusValue);
        bonusEdit.setText(String.format(Locale.getDefault(), "%d", bonus));
        final Dialog bonusDialog = new AlertDialog.Builder(this)
                .setTitle(R.string.bonusDialogTitle).setView(bonusView).show();

        Button button = bonusView.findViewById(R.id.cancelBonus);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                bonusDialog.dismiss();
            }
        });

        button = bonusView.findViewById(R.id.okBonus);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                CheckBox ck = bonusView.findViewById(R.id.currentRoll);
                boolean currentRoll = ck.isChecked();
                ck = bonusView.findViewById(R.id.futureRoll);
                boolean futureRolls = ck.isChecked();

                // Ignore ok button click if no options are clicked
                if (!currentRoll && !futureRolls) {
                    return;
                }

                EditText bonusEdit = bonusView.findViewById(R.id.bonusValue);

                // Ignore ok button click if the bonus text value is empty.
                String bonusString = bonusEdit.getText().toString();
                if (bonusString.isEmpty()) {
                    return;
                }

                int bonusValue = Integer.valueOf(bonusString);

                if (bonusValue != 0 && currentRoll) {
                    LogFile.LogItem logitem = null;
                    int logFileSize = logFile.size();
                    int index = logItemToLoad;
                    if (index < 0) {
                        index = logFileSize - 1;
                    }
                    if (index > 0 && logFileSize > index) {
                        logitem = logFile.getCopy(index);
                    }

                    if (logitem != null && logitem.getVersion() >= 1) {
                        DiceResult result = logitem.getRollResults();
                        result.addBonus(bonusValue);

                        DieConfiguration[] diceConfig = logitem.getDiceConfiguration();
                        String diceName = logitem.getDiceName();
                        TextView text = findViewById(R.id.rollResult);
                        text.setText(result.generateResultsString(diceConfig, diceName, MainActivity.this));

                        logFile.addRoll(diceName, result, diceConfig);
                        logItemToLoad = -1;
                    }
                }

                if (futureRolls) {
                    config.setBonus(bonusValue);
                    config.save();
                    invalidateOptionsMenu();
                }

                bonusDialog.dismiss();
            }
        });

        class OnToggleCheckboxListenerCreator {
            public OnToggleCheckboxListenerCreator() { }

            public View.OnClickListener getOnToggleCheckboxListener() {
                return new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        CheckBox ck = bonusView.findViewById(R.id.currentRoll);
                        boolean currentRoll = ck.isChecked();
                        ck = bonusView.findViewById(R.id.futureRoll);
                        boolean futureRolls = ck.isChecked();

                        Button button = bonusView.findViewById(R.id.okBonus);

                        // Ignore ok button click if no options are clicked
                        if (!currentRoll && !futureRolls) {
                            button.setEnabled(false);
                            button.setClickable(false);

                            int[] attrs = {R.attr.textDisabledColor};
                            TypedArray ta = obtainStyledAttributes(attrs);
                            button.setTextColor(ta.getColor(0, 0xff505050));
                            ta.recycle();
                        } else {
                            button.setEnabled(true);
                            button.setClickable(true);

                            int[] attrs = {android.R.attr.textColor};
                            TypedArray ta = obtainStyledAttributes(attrs);
                            button.setTextColor(ta.getColor(0, 0xff505050));
                            ta.recycle();
                        }
                    }
                };
            }
        }

        OnToggleCheckboxListenerCreator cr = new OnToggleCheckboxListenerCreator();
        CheckBox ck = bonusView.findViewById(R.id.currentRoll);
        ck.setOnClickListener(cr.getOnToggleCheckboxListener());

        ck = bonusView.findViewById(R.id.futureRoll);
        ck.setOnClickListener(cr.getOnToggleCheckboxListener());
    }

    public void onConfigure(MenuItem item) {
        Intent intent = new Intent(this, DiceConfigurationActivity.class);
        startActivityForResult(intent, DICE_CONFIGURATION_ACTIVITY);
    }

    public void onOpenLogFile(MenuItem item) {
        Intent intent = new Intent(this, DiceLogActivity.class);
        startActivityForResult(intent, Constants.DICE_LOG_FILE_ACTIVITY);
    }

    public void onSelectTheme(MenuItem item) {
        Intent intent = new Intent(this, ActivityThemeSelector.class);
        if (graphicsDescription != null) {
            intent.putExtra(Constants.SENSOR_HAS_LINEAR_ACCELERATION, graphicsDescription.hasLinearAccelerationSensor);
            intent.putExtra(Constants.SENSOR_HAS_GRAVITY, graphicsDescription.hasGravitySensor);
            intent.putExtra(Constants.SENSOR_HAS_ACCELEROMETER, graphicsDescription.hasAccelerometer);
            intent.putExtra(Constants.GRAPHICS_API_NAME, graphicsDescription.apiName);
            intent.putExtra(Constants.GRAPHICS_IS_VULKAN, graphicsDescription.isVulkan);
            if (graphicsDescription.apiVersion != null) {
                intent.putExtra(Constants.GRAPHICS_API_VERSION, graphicsDescription.apiVersion);
            }
            if (graphicsDescription.deviceName != null) {
                intent.putExtra(Constants.GRAPHICS_DEVICE_NAME, graphicsDescription.deviceName);
            }
        }
        startActivityForResult(intent, DICE_THEME_SELECTION_ACTIVITY);
    }

    public void onNewDice(MenuItem item) {
        Intent intent = new Intent(this, DiceCustomizationActivity.class);
        startActivityForResult(intent, DICE_CUSTOMIZATION_ACTIVITY);
    }

    public void onReroll(View view) {
        Draw.rerollSelected();
        logItemToLoad = -1;
    }

    public void onPlusReroll(View view) {
        Draw.addRerollSelected();
        logItemToLoad = -1;
    }

    public void onDeleteDiceFromRoll(View view) {
        Draw.deleteSelected();
        logItemToLoad = -1;
    }

    public  void onResetView(View view) {
        view.setEnabled(false);
        view.setBackground(getDrawable(R.drawable.drop_changes_grayscale));
        Draw.resetView();
    }

    private void displayMessageDialog(int titleID, int messageID) {
        LinearLayout layout = findViewById(R.id.activity_main_top_level);
        LayoutInflater inflater = getLayoutInflater();

        final LinearLayout dialogView = (LinearLayout) inflater.inflate(
                R.layout.message_dialog, layout, false);

        TextView message = dialogView.findViewById(R.id.message);
        message.setText(messageID);
        final Dialog dialog = new AlertDialog.Builder(this)
                .setTitle(titleID).setView(dialogView).show();

        Button button = dialogView.findViewById(R.id.okButton);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
    }

    private void saveDiceToExternalFile(Uri file) {
        boolean succeeded = true;
        try {
            ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(file, "w");
            if (pfd == null) {
                return;
            }
            FileOutputStream fileOutputStream = new FileOutputStream(pfd.getFileDescriptor());
            succeeded = DataExporter.exportAllToFile(this, fileOutputStream);

            // Let the document provider know you're done by closing the stream.
            fileOutputStream.close();
            pfd.close();
        } catch (FileNotFoundException e) {
            succeeded = false;
        } catch (IOException e) {
            succeeded = false;
        }

        if (!succeeded) {
            displayMessageDialog(R.string.error, R.string.errorExportFailed);
        }
    }

    private void restoreDiceFromExternalFile(Uri uri) {
        boolean succeeded = true;
        try {
            InputStream inputStream = getContentResolver().openInputStream(uri);
            succeeded = DataExporter.importAllFromFile(this, inputStream);

        } catch (IOException e) {
            succeeded = false;
        }

        if (!succeeded) {
            displayMessageDialog(R.string.error, R.string.errorImportFailed);
        }
    }

    public void drawStoppedDice(String name, DieConfiguration[] diceConfig, DiceResult diceResult) {
        int i = 0;
        int j = 1;
        for (ArrayList<DiceResult.DieResult> dieResults : diceResult.getDiceResults()) {
            for (DiceResult.DieResult dieResult : dieResults) {
                if (dieResult.index() == null) {
                    int index = diceConfig[i].getIndexForSymbol(dieResult.symbol());
                    dieResult.setIndex(index);
                }
            }
            if (j < diceConfig[i].getNumberOfDice()) {
                j++;
            } else {
                j = 1;
                i++;
            }
        }
        String err = Draw.startDrawingStoppedDice(name, diceConfig, diceResult);
        if (err != null && err.length() > 0) {
            TextView text = findViewById(R.id.rollResult);
            text.setText(err);
        }
    }

    public LogFileLoadedResult loadFromLog(int i) {
        TextView text = findViewById(R.id.rollResult);
        LogFile.LogItem item = null;
        int logFileSize = logFile.size();
        if (i >= 0 && i < logFileSize) {
            item = logFile.get(i);
        } else if (logFileSize > 0) {
            item = logFile.get(logFileSize - 1);
        }

        if (item == null || item.getVersion() != 1) {
            text.setText(R.string.diceMessageStartup);
            return null;
        }

        LogFileLoadedResult result = new LogFileLoadedResult();
        result.diceResult = item.getRollResults();
        result.name = item.getDiceName();
        result.diceConfig = item.getDiceConfiguration();

        text.setText(result.diceResult.generateResultsString(result.diceConfig,
                result.name, this));
        return result;
    }

    private DieConfiguration[] loadFromFile(String filename) {
        try {
            FileInputStream inputStream = openFileInput(filename);
            DiceGroup diceGroup = DiceGroup.loadFromFile(inputStream);
            inputStream.close();

            if (diceGroup == null) {
                return null;
            }

            return diceGroup.dieConfigurations();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
            return null;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
            return null;
        }
    }

    private void createDefaults(DiceConfigurationManager configurationFile) {
        int count = configurationFile.getDiceList().size();

        if (count == 0) {
            String filename;
            // There are no favorites or dice configuration defaults.  Add a few example dice
            // configurations.
            DiceGroup diceGroup;
            DieConfiguration[] dice = new DieConfiguration[1];
            dice[0] = new DieConfiguration(1, 6, 1, 1,
                    null, true, null);
            diceGroup = new DiceGroup(dice);
            filename = "1D6";
            configurationFile.addDice(filename, diceGroup);

            dice[0] = new DieConfiguration(2, 6, 1, 1,
                    null, true, null);
            diceGroup = new DiceGroup(dice);
            filename = "2D6";
            configurationFile.addDice(filename, diceGroup);

            dice[0] = new DieConfiguration(1, 20, 1, 1,
                    null, true, null);
            diceGroup = new DiceGroup(dice);
            filename = "1D20";
            configurationFile.addDice(filename, diceGroup);

            dice = DieConfiguration.createFudge();
            diceGroup = new DiceGroup(dice);
            filename = "Fudge Dice";
            configurationFile.addDice(filename, diceGroup);

            dice = new DieConfiguration[1];
            dice[0] = new DieConfiguration(12, 6, 1, 1,
                    null, true, null);
            diceGroup = new DiceGroup(dice);
            filename = "12D6";
            configurationFile.addDice(filename, diceGroup);

            dice = DieConfiguration.createPercentile();
            diceGroup = new DiceGroup(dice);
            filename = "Percentile";
            configurationFile.addDice(filename, diceGroup);

            dice = new DieConfiguration[2];
            int[] rerollOn = new int[1];
            rerollOn[0] = 6;
            dice[0] = new DieConfiguration(1, 6, 1, 1, rerollOn,
                    true, Constants.WHITE);
            dice[1] = new DieConfiguration(1, 6, 1, 1, rerollOn,
                    false, Constants.BLACK);
            diceGroup = new DiceGroup(dice);
            filename = "d6-d6(reroll 6's)";
            configurationFile.addDice(filename, diceGroup);
            configurationFile.save();
        }
    }

    public void createWorker() {
        if (drawer != null) {
            return;
        }

        Button resetView = findViewById(R.id.reset_view);
        resetView.setBackground(getDrawable(R.drawable.drop_changes_grayscale));
        resetView.setEnabled(false);

        ConfigurationFile configurationFile = new ConfigurationFile(this);
        SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
        SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
        TextView resultView = findViewById(R.id.rollResult);
        Handler notify = new Handler(new ResultHandler(resultView));
        drawer = new Thread(new DiceWorker(notify, drawSurfaceHolder, assetManager,
                configurationFile.useGravity(), configurationFile.drawRollingDice(),
                configurationFile.useLegacy(), configurationFile.reverseGravity()));
        drawer.start();
    }

    private void rollTheDice(DieConfiguration[] diceConfig, String diceFileName) {
        String err = Draw.startDrawingRoll(diceConfig, diceFileName);
        if (err != null && !err.isEmpty()) {
            TextView resultView = findViewById(R.id.rollResult);
            resultView.setText(err);
        }

        // load the latest log item on pause.
        logItemToLoad = -1;
    }

    public void joinDrawer() {
        if (drawer == null) {
            return;
        }

        try {
            Draw.tellDrawerStop();
            drawer.join();
        } catch (InterruptedException e) {
        }
        drawer = null;
    }

    public void publishError(String err) {
        if (err != null && err.length() > 0) {
            TextView view = findViewById(R.id.rollResult);
            view.setText(err);
        }
    }

    private class ResultHandler implements Handler.Callback {
        private TextView text;
        public ResultHandler(TextView textToUpdate) {
            text = textToUpdate;
        }
        public boolean handleMessage(Message message) {
            Bundle data = message.getData();
            if (data.containsKey(DiceDrawerReturnChannel.errorMsg)) {
                String serror = data.getString(DiceDrawerReturnChannel.errorMsg);
                // the result is an error message.  Print the error message to the text view
                text.setText(serror);
                return true;
            } else if (data.containsKey(DiceDrawerReturnChannel.diceSelected)) {
                boolean diceSelected = data.getBoolean(DiceDrawerReturnChannel.diceSelected);
                Button plusRolling = MainActivity.this.findViewById(R.id.plus_rolling_dice);
                Button rolling = MainActivity.this.findViewById(R.id.rolling_dice);
                Button deleteDice = MainActivity.this.findViewById(R.id.delete_dice);

                if (diceSelected) {
                    plusRolling.setBackground(MainActivity.this.getDrawable(R.drawable.plus_rolling_dice));
                    rolling.setBackground(MainActivity.this.getDrawable(R.drawable.rolling_dice));
                    deleteDice.setBackground(MainActivity.this.getDrawable(R.drawable.trash));
                    plusRolling.setEnabled(true);
                    rolling.setEnabled(true);
                    deleteDice.setEnabled(true);
                } else {
                    plusRolling.setBackground(MainActivity.this.getDrawable(R.drawable.plus_rolling_dice_grayscale));
                    rolling.setBackground(MainActivity.this.getDrawable(R.drawable.rolling_dice_grayscale));
                    deleteDice.setBackground(MainActivity.this.getDrawable(R.drawable.trash_grayscale));
                    plusRolling.setEnabled(false);
                    rolling.setEnabled(false);
                    deleteDice.setEnabled(false);
                }

            } else if (data.containsKey(DiceDrawerReturnChannel.isVulkan)) {
                if (graphicsDescription == null) {
                    graphicsDescription = new GraphicsDescription();
                }

                graphicsDescription.hasLinearAccelerationSensor =
                        data.getBoolean(DiceDrawerReturnChannel.hasLinearAccelerationType);
                graphicsDescription.hasGravitySensor =
                        data.getBoolean(DiceDrawerReturnChannel.hasGravityType);
                graphicsDescription.hasAccelerometer =
                        data.getBoolean(DiceDrawerReturnChannel.hasAccelerometerType);
                graphicsDescription.isVulkan = data.getBoolean(DiceDrawerReturnChannel.isVulkan);
                graphicsDescription.apiName = data.getString(DiceDrawerReturnChannel.apiName);
                if (data.containsKey(DiceDrawerReturnChannel.apiVersion)) {
                    graphicsDescription.apiVersion = data.getString(DiceDrawerReturnChannel.apiVersion);
                } else {
                    graphicsDescription.apiVersion = getString(R.string.unknown);
                }
                if (data.containsKey(DiceDrawerReturnChannel.deviceName)) {
                    graphicsDescription.deviceName = data.getString(DiceDrawerReturnChannel.deviceName);
                } else {
                    graphicsDescription.deviceName = getString(R.string.unknown);
                }
            } else if (data.containsKey(DiceDrawerReturnChannel.resultsMsg) &&
                    data.containsKey(DiceDrawerReturnChannel.diceConfigMsg)) {

                DiceDrawerMessage results = data.getParcelable(DiceDrawerReturnChannel.resultsMsg);

                Parcelable[] diceConfigParcels =
                        data.getParcelableArray(DiceDrawerReturnChannel.diceConfigMsg);
                if (diceConfigParcels == null || results == null) {
                    // these values should always be there.
                    return true;
                }
                DieConfiguration[] diceConfig = new DieConfiguration[diceConfigParcels.length];
                for (int i = 0; i < diceConfigParcels.length; i++) {
                    diceConfig[i] = (DieConfiguration)diceConfigParcels[i];
                }

                String filename = "";
                if (data.containsKey(DiceDrawerReturnChannel.fileNameMsg)) {
                    filename = data.getString(DiceDrawerReturnChannel.fileNameMsg);
                }

                boolean isModifiedRoll = false;
                if (data.containsKey(DiceDrawerReturnChannel.isModifiedRollMsg)) {
                    isModifiedRoll = data.getBoolean(DiceDrawerReturnChannel.isModifiedRollMsg);
                }

                DiceConfigurationManager configurationFile = new DiceConfigurationManager(
                        MainActivity.this);

                Integer bonusValue = configurationFile.bonus();
                DiceResult diceResult = new DiceResult(results, diceConfig, isModifiedRoll, bonusValue);

                // no dice need reroll, just update the results text view with the results.
                logFile.addRoll(filename, diceResult, diceConfig);
                text.setText(diceResult.generateResultsString(diceConfig, filename,
                        MainActivity.this));
            }
            return true;
        }
    }
}
