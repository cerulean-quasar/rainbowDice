/**
 * Copyright 2018 Cerulean Quasar. All Rights Reserved.
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

import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;

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

    private boolean surfaceReady = false;
    private Thread drawer = null;

    public class LogFileLoadedResult {
        public DieConfiguration[] diceConfig = null;
        public DiceResult diceResult = null;
    }

    private DiceConfigurationManager configurationFile = null;
    private LogFile logFile = null;
    private boolean drawingStarted = false;
    private AssetManager assetManager = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        configurationFile = new DiceConfigurationManager(this);

        String themeName = configurationFile.getTheme();
        if (themeName == null || themeName.isEmpty()) {
            themeName = "Space";
        }
        int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
        setTheme(currentThemeId);

        super.onCreate(savedInstanceState);

        assetManager = getAssets();
        createDefaults();
        logFile = new LogFile(this);
        initGui();
    }

    void initGui() {
        setContentView(R.layout.activity_main);

        SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
        drawSurfaceView.setZOrderOnTop(true);
        SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
        drawSurfaceHolder.setFormat(PixelFormat.TRANSPARENT);
        drawSurfaceHolder.addCallback(new MySurfaceCallback(this));

        resetDiceList();
    }

    void resetDiceList() {
        configurationFile = new DiceConfigurationManager(this);
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
                    drawingStarted = true;
                    TextView result = findViewById(R.id.rollResult);
                    result.setText(getString(R.string.diceMessageToStartRolling));
                    Button b = (Button) v;
                    joinDrawer();
                    String diceFileName = b.getText().toString();
                    DieConfiguration[] diceConfig = loadFromFile(diceFileName);
                    rollTheDice(diceConfig, diceFileName);
                }
            });
            layout.addView(button);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        joinDrawer();
        logFile.writeFile();
    }

    @Override
    protected void onStop() {
        super.onStop();
        joinDrawer();
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        joinDrawer();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == DICE_CONFIGURATION_ACTIVITY) {
            resetDiceList();
        } else if (requestCode == DICE_THEME_SELECTION_ACTIVITY) {
            if (resultCode != RESULT_OK) {
                // The user cancelled theme selection
                return;
            }
            String themeName = data.getStringExtra(themeNameConfigValue);
            if (themeName != null && !themeName.isEmpty()) {
                int resID = getResources().getIdentifier(themeName, "style", getPackageName());
                setTheme(resID);
                initGui();
                TypedValue value = new TypedValue();
                getTheme().resolveAttribute(android.R.attr.windowBackground, value, true);
                getWindow().setBackgroundDrawableResource(value.resourceId);
            }
        } else if (requestCode == DICE_CUSTOMIZATION_ACTIVITY) {
            resetDiceList();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_menu, menu);
        return true;
    }

    public void onConfigure(MenuItem item) {
        joinDrawer();
        Intent intent = new Intent(this, DiceConfigurationActivity.class);
        startActivityForResult(intent, DICE_CONFIGURATION_ACTIVITY);
    }

    public void onOpenLogFile(MenuItem item) {
        joinDrawer();
        Intent intent = new Intent(this, DiceLogActivity.class);
        startActivityForResult(intent, Constants.DICE_LOG_FILE_ACTIVITY);
    }

    public void onSelectTheme(MenuItem item) {
        joinDrawer();
        Intent intent = new Intent(this, ActivityThemeSelector.class);
        startActivityForResult(intent, DICE_THEME_SELECTION_ACTIVITY);
    }

    public void onNewDice(MenuItem item) {
        joinDrawer();
        Intent intent = new Intent(this, DiceCustomizationActivity.class);
        startActivityForResult(intent, DICE_CUSTOMIZATION_ACTIVITY);
    }

    public void setSurfaceReady(boolean inIsReady) {
        surfaceReady = inIsReady;
    }

    public void drawStoppedDice(DieConfiguration[] diceConfig, DiceResult diceResult) {
        ArrayList<ArrayList<DiceResult.DieResult>> diceResultList = diceResult.getDiceResults();
        int len = 0;
        for (ArrayList<DiceResult.DieResult> dieResults : diceResultList) {
            len += dieResults.size();
        }

        String[] symbols = new String[len];

        int i = 0;
        for (ArrayList<DiceResult.DieResult> dieResults : diceResultList) {
            for (DiceResult.DieResult dieResult : dieResults) {
                symbols[i++] = Integer.toString(dieResult.result, 10);
            }
        }

        SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
        SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
        Draw draw = new Draw(null, drawSurfaceHolder, diceConfig, assetManager, null);
        String err = draw.startDrawingStoppedDice(symbols);
        if (err != null && err.length() > 0) {
            TextView text = findViewById(R.id.rollResult);
            text.setText(err);
        }
    }

    public LogFileLoadedResult loadFromLog() {
        TextView text = findViewById(R.id.rollResult);
        LogFile.LogItem item = null;
        int logFileSize = logFile.size();
        if (logFileSize > 0) {
            item = logFile.get(logFileSize - 1);
        }

        if (item == null || item.getVersion() != 1) {
            text.setText(R.string.diceMessageStartup);
            return null;
        }

        LogFileLoadedResult result = new LogFileLoadedResult();
        result.diceResult = item.getRollResults();
        ArrayList<ArrayList<DiceResult.DieResult>> diceResultList = result.diceResult.getDiceResults();
        int len = 0;
        for (ArrayList<DiceResult.DieResult> dieResults : diceResultList) {
            len += dieResults.size();
        }

        DieConfiguration[] configs = item.getDiceConfiguration();
        result.diceConfig = new DieConfiguration[len];

        int k = 0;
        int l = 0;
        int m = 0;
        for (int j = 0; j < diceResultList.size(); j++) {
            ArrayList<DiceResult.DieResult> dieResults = diceResultList.get(j);
            for (int i=0; i < dieResults.size(); i++) {
                DiceResult.DieResult dieResult = dieResults.get(i);
                if (configs[l].isRepresentableByTwoTenSided()) {
                    if (configs[l].getStartAt() == 0) {
                        result.diceConfig[k++] = new DieConfiguration(1, 10, 0, 10,
                                configs[l].getReRollOn(), configs[l].isAddOperation());
                        result.diceConfig[k++] = new DieConfiguration(1, 10, 0, 1,
                                configs[l].getReRollOn(), configs[l].isAddOperation());
                    } else {
                        result.diceConfig[k++] = new DieConfiguration(1, 10, 0, 10,
                                configs[l].getReRollOn(), configs[l].isAddOperation());
                        result.diceConfig[k++] = new DieConfiguration(1, 10, 1, 1,
                                configs[l].getReRollOn(), configs[l].isAddOperation());
                    }
                    j++;
                } else {
                    result.diceConfig[k++] = new DieConfiguration(configs[l], 1);
                }
            }
            if (m == configs[l].getNumberOfDice()-1) {
                l++;
                m = 0;
            } else {
                m++;
            }
        }

        text.setText(result.diceResult.generateResultsString(getString(R.string.diceMessageResult),
                getString(R.string.addition), getString(R.string.subtraction)));
        return result;
    }

    private DieConfiguration[] loadFromFile(String filename) {
        StringBuilder json = new StringBuilder();

        try {
            byte[] bytes = new byte[1024];
            int len;
            FileInputStream inputStream = openFileInput(filename);
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, "UTF-8"));
                }
            }
            inputStream.close();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
            return null;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
            return null;
        }

        try {
            JSONArray jsonArray = new JSONArray(json.toString());
            return DieConfiguration.fromJsonArray(jsonArray);
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + filename + " message: " + e.getMessage());
            return null;
        }
    }

    private void createDefaults() {
        int count = configurationFile.getDiceList().size();

        if (count == 0) {
            String[] jsonStrings = new String[5];
            String filename;
            // There are no favorites or dice configuration defaults.  Add a few example dice
            // configurations.
            DieConfiguration[] dice = new DieConfiguration[1];
            dice[0] = new DieConfiguration(1, 6, 1, 1, 0, true);
            filename = "1D6";
            configurationFile.addDice(filename, dice);


            dice[0] = new DieConfiguration(2, 6, 1, 1, 0, true);
            filename = "2D6";
            configurationFile.addDice(filename, dice);

            dice[0] = new DieConfiguration(1, 20, 1, 1, 0, true);
            filename = "1D20";
            configurationFile.addDice(filename, dice);

            dice[0] = new DieConfiguration(1, 100, 0, 1, -1, true);
            filename = "Percentile";
            configurationFile.addDice(filename, dice);

            dice[0] = new DieConfiguration(4, 3, -1, 1, -2, true);
            filename = "Fudge Dice";
            configurationFile.addDice(filename, dice);

            dice[0] = new DieConfiguration(12, 6, 1, 1, 0, true);
            filename = "12D6";
            configurationFile.addDice(filename, dice);

            dice = new DieConfiguration[2];
            dice[0] = new DieConfiguration(1, 6, 1, 1, 6, true);
            dice[1] = new DieConfiguration(1, 6, 1, 1, 6, false);
            filename = "d6-d6(reroll 6's)";
            configurationFile.addDice(filename, dice);
            configurationFile.save();
        }
    }

    private void rollTheDice(DieConfiguration[] diceConfig, String diceFileName) {
        if (!surfaceReady) {
            // shouldn't happen.
            return;
        }

        SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
        SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
        joinDrawer();
        TextView resultView = findViewById(R.id.rollResult);
        Handler notify = new Handler(new ResultHandler(resultView, diceConfig, diceFileName));
        drawer = new Thread(new Draw(notify, drawSurfaceHolder, diceConfig, assetManager, null));
        drawer.start();
    }

    public void joinDrawer() {
        boolean done = false;
        while (!done && drawer != null) {
            tellDrawerStop();
            try {
                drawer.join();
                done = true;
            } catch (InterruptedException e) {
            }
        }
        drawer = null;
    }

    public void publishError(String err) {
        if (err != null && err.length() > 0) {
            TextView view = findViewById(R.id.rollResult);
            view.setText(err);
        }
    }

    public boolean isDrawing() {
        return drawingStarted;
    }

    private class ResultHandler implements Handler.Callback {
        private TextView text;
        private DieConfiguration[] diceConfig;
        private String diceFileLoaded;
        public ResultHandler(TextView textToUpdate, DieConfiguration[] inDiceConfig, String inDiceFileName) {
            text = textToUpdate;
            diceConfig = inDiceConfig;
            diceFileLoaded = inDiceFileName;
        }
        public boolean handleMessage(Message message) {
            Bundle data = message.getData();
            String svalue = data.getString("results");
            joinDrawer();
            if (svalue.contains("error: ")) {
                // the result is an error message.  Print the error message to the text view
                text.setText(svalue);
                return true;
            }

            DiceResult diceResult = new DiceResult(svalue, diceConfig);

            // no dice need reroll, just update the results text view with the results.
            logFile.addRoll(diceFileLoaded, diceResult, diceConfig);
            drawingStarted = false;
            text.setText(diceResult.generateResultsString(getString(R.string.diceMessageResult),
                    getString(R.string.addition), getString(R.string.subtraction)));
            return true;
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private native void tellDrawerStop();
}
