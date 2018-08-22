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

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.opengl.GLUtils;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.Locale;
import java.util.TreeSet;

import static android.graphics.Bitmap.Config.ALPHA_8;
import static android.graphics.Bitmap.Config.ARGB_4444;
import static android.graphics.Bitmap.Config.ARGB_8888;
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

    private static final int DICE_CONFIGURATION_ACTIVITY = 1;

    private boolean surfaceReady = false;
    private Thread drawer = null;
    private DieConfiguration[] diceConfig = null;
    private DiceResult diceResult = null;
    private DiceConfigurationManager configurationFile = null;
    private LogFile logFile = null;
    private String diceFileLoaded = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        configurationFile = new DiceConfigurationManager(this);

        String themeName = configurationFile.getTheme();
        if (themeName != null && !themeName.isEmpty()) {
            int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
            setTheme(currentThemeId);
        }

        super.onCreate(savedInstanceState);
        createDefaults();
        initGui();

        logFile = new LogFile(this);
        TextView text = findViewById(R.id.rollResult);
        text.setText(R.string.diceMessageStartup);
        LinkedList<String> diceList = configurationFile.getDiceList();
        if (diceList != null && diceList.size() > 0) {
            loadFromFile(diceList.getFirst());
        } else {
            diceConfig = new DieConfiguration[1];
            diceConfig[0] = new DieConfiguration(1, 6, 1, 1, 0, true);
        }
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
            button.setBackgroundColor(0);
            button.setText(dice);
            button.setPadding(50,0,50,0);
            button.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Button b = (Button) v;
                    joinDrawer();
                    destroySurface();
                    loadFromFile(b.getText().toString());
                    SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
                    SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
                    startDrawing(drawSurfaceHolder);
                    rollTheDice();
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
        diceResult = null;
    }

    @Override
    protected void onResume() {
        super.onResume();
        diceResult = null;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        joinDrawer();
        destroySurface();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode != RESULT_OK) {
            // The user cancelled editing
            return;
        }
        if (requestCode == DICE_CONFIGURATION_ACTIVITY) {
            resetDiceList();
        } else if (requestCode == DICE_THEME_SELECTION_ACTIVITY) {
            String themeName = data.getStringExtra(themeNameConfigValue);
            if (themeName != null && !themeName.isEmpty()) {
                int resID = getResources().getIdentifier(themeName, "style", getPackageName());
                setTheme(resID);
                initGui();
                TypedValue value = new TypedValue();
                getTheme().resolveAttribute(android.R.attr.windowBackground, value, true);
                getWindow().setBackgroundDrawableResource(value.resourceId);
            }
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

    private void loadFromFile(String filename) {
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
            return;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
            return;
        }

        try {
            JSONArray jsonArray = new JSONArray(json.toString());
            int length = jsonArray.length();
            diceConfig = new DieConfiguration[length];
            for (int i = 0; i < length; i++) {
                JSONObject jsonDieConfig = jsonArray.getJSONObject(i);
                DieConfiguration dieConfig = DieConfiguration.fromJson(jsonDieConfig);
                diceConfig[i] = dieConfig;
            }
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + filename + " message: " + e.getMessage());
            return;
        }
        diceFileLoaded = filename;
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

    private void rollTheDice() {
        if (surfaceReady) {
            joinDrawer();
            diceResult = null;
            TextView result = findViewById(R.id.rollResult);
            result.setText(getString(R.string.diceMessageToStartRolling));
            roll();
            startDrawer();
        }
    }

    public void destroySurface() {
        if (surfaceReady) {
            surfaceReady = false;
            destroyVulkan();
        }
    }

    private void startDrawer() {
        if (drawer == null && surfaceReady) {
            TextView resultView = findViewById(R.id.rollResult);
            Handler notify = new Handler(new ResultHandler(resultView));
            drawer = new Thread(new Draw(notify));
            drawer.start();
        }
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

    private boolean loadModelsAndTextures() {
        if (diceConfig == null || diceConfig.length == 0) {
            // somehow someone managed to crash the code by having dice config null.  I don't know
            // how they did this, but returning here avoids the crash.
            publishError("No dice configurations exist, please choose dice.");
            return false;
        }
        int TEXWIDTH = 64;
        int TEXHEIGHT = 64;

        TreeSet<String> symbolSet = new TreeSet<>();

        // First we need to load all the textures in the texture Atlas (in cpp), then
        // we can load the models.  Since the model depends on the size of the textures.
        for (DieConfiguration dieConfig : diceConfig) {
            int nbrSides = dieConfig.getNumberOfSides();
            int startOn = dieConfig.getStartAt();
            int increment = dieConfig.getIncrement();
            if (dieConfig.isRepresentableByTwoTenSided()) {
                // special case a d100 die to be represented by two 10 sided dice, one for the ten's
                // place and one for the one's place.
                for (int j = 0; j < 10; j++) {
                    String symbol = String.format(Locale.getDefault(), "%d", startOn + increment * j);
                    symbolSet.add(symbol);
                }

                for (int j = 0; j < 10; j++) {
                    String symbol = String.format(Locale.getDefault(), "%d", 10 * j);
                    symbolSet.add(symbol);
                }

            } else if (nbrSides > 1) {
                for (int j = 0; j < nbrSides; j++) {
                    String symbol = String.format(Locale.getDefault(), "%d", startOn + increment * j);
                    symbolSet.add(symbol);
                }
            }
        }

        Bitmap bitmap;
        try {
            bitmap = Bitmap.createBitmap(TEXWIDTH, TEXHEIGHT * symbolSet.size(), ALPHA_8);
        } catch (Exception e) {
            if (e.getMessage() != null) {
                publishError(e.getMessage());
            } else {
                publishError("Could not generate dice textures.");
            }
            return false;
        }
        Canvas canvas = new Canvas(bitmap);
        Paint paint = new Paint();
        paint.setUnderlineText(true);
        paint.setTextAlign(Paint.Align.CENTER);
        canvas.drawARGB(0,0,0,0);
        int i = 0;
        for (String symbol : symbolSet) {
            if (symbol.length() < 3) {
                paint.setTextSize(50.0f);
            } else {
                paint.setTextSize(25.0f);
            }
            canvas.drawText(symbol, 40, i*TEXHEIGHT + 40, paint);
            i++;
        }
        int bitmapSize = bitmap.getAllocationByteCount();
        ByteBuffer imageBuffer = ByteBuffer.allocate(bitmapSize);
        bitmap.copyPixelsToBuffer(imageBuffer);
        byte[] bytes = new byte[bitmapSize];
        try {
            imageBuffer.position(0);
            imageBuffer.get(bytes);
        } catch (Exception e) {
            publishError(e.getMessage() != null ? e.getMessage() : e.getClass().toString());
            return false;
        }
        String err = addSymbols(symbolSet.toArray(new String[symbolSet.size()]), symbolSet.size(),
                TEXWIDTH, TEXHEIGHT*symbolSet.size(), TEXHEIGHT, bitmapSize, bytes);
        if (err != null && err.length() != 0) {
            publishError(err);
            return false;
        }

        // now load the models. Some of the vertices depend on the size of the texture image.
        for (DieConfiguration dieConfig : diceConfig) {
            int nbrDice = dieConfig.getNumberOfDice();
            int nbrSides = dieConfig.getNumberOfSides();
            int startOn = dieConfig.getStartAt();
            int increment = dieConfig.getIncrement();
            if (dieConfig.isRepresentableByTwoTenSided()) {
                // special case a d100 die to be represented by two 10 sided dice, one for the ten's
                // place and one for the one's place.
                String[] symbolsOnes = new String[10];
                String[] symbolsTens = new String[10];
                for (int j = 0; j < 10; j++) {
                    symbolsOnes[j] = String.format(Locale.getDefault(), "%d", startOn + increment * j);
                }

                for (int j = 0; j < 10; j++) {
                    symbolsTens[j] = String.format(Locale.getDefault(), "%d", 10 * j);
                }

                for (int j = 0; j < nbrDice; j++) {
                    loadModel(symbolsTens);
                    loadModel(symbolsOnes);
                }
            } else if (nbrSides > 1){
                String[] symbols = new String[nbrSides];
                for (int j = 0; j < nbrSides; j++) {
                    symbols[j] = String.format(Locale.getDefault(), "%d", startOn + increment * j);
                }
                for (int j = 0; j < nbrDice; j++){
                    loadModel(symbols);
                }
            }
        }

        return true;
    }

    public void publishError(String err) {
        if (err != null && err.length() > 0) {
            TextView view = findViewById(R.id.rollResult);
            view.setText(err);
        }
    }

    public void startDrawing(SurfaceHolder holder) {
        boolean usingVulkan = true;
        Surface drawSurface = holder.getSurface();
        String err = initWindow(usingVulkan, drawSurface);
        if (err != null && err.length() != 0) {
            usingVulkan = false;
            // Vulkan failed, try with OpenGL instead
            err = initWindow(false, drawSurface);
            if (err != null && err.length() != 0) {
                publishError(err);
                return;
            }
        }

        if (!loadModelsAndTextures()) {
            return;
        }

        byte[] vertShader;
        byte[] fragShader;
        if (usingVulkan) {
            vertShader = readAssetFile(false, "shaders/shader.vert.spv");
            fragShader = readAssetFile(false, "shaders/shader.frag.spv");
        } else {
            vertShader = readAssetFile(true, "shaderGL.vert");
            fragShader = readAssetFile(true, "shaderGL.frag");
        }
        err = sendVertexShader(vertShader, vertShader.length);
        if (err != null && err.length() != 0) {
            publishError(err);
            return;
        }

        err = sendFragmentShader(fragShader, fragShader.length);
        if (err != null && err.length() != 0) {
            publishError(err);
            return;
        }

        err = initPipeline();
        if (err != null && err.length() != 0) {
            publishError(err);
            return;
        }

        err = initSensors();
        if (err != null && err.length() != 0) {
            publishError(err);
            return;
        }

        surfaceReady = true;
    }

    private byte[] readAssetFile(boolean nullAppend, String filename) {
        try {
            InputStream inputStream = getAssets().open(filename);
            ByteBufferBuilder bytes = new ByteBufferBuilder(1024);
            bytes.readToBuffer(inputStream, nullAppend);
            return bytes.getBytes();
        } catch (IOException e) {
            publishError("Error in opening asset file: " + filename + " message: " + e.getMessage());
            return null;
        }
    }

    private class ResultHandler implements Handler.Callback {
        private TextView text;
        public ResultHandler(TextView textToUpdate) { text = textToUpdate; }
        public boolean handleMessage(Message message) {
            Bundle data = message.getData();
            String svalue = data.getString("results");
            joinDrawer();
            if (svalue.contains("error: ")) {
                // the result is an error message.  Print the error message to the text view
                text.setText(svalue);
                return true;
            }
            if (diceResult != null) {
                diceResult.updateWithReRolls(svalue);
            } else {
                diceResult = new DiceResult(svalue, diceConfig);
            }

            int[] indicesNeedReRoll = diceResult.reRollRequiredOnIndices();
            if (indicesNeedReRoll == null) {
                // no dice need reroll, just update the results text view with the results.
                text.setText(diceResult.generateResultsString(getString(R.string.diceMessageResult),
                        getString(R.string.addition), getString(R.string.subtraction)));
                logFile.addRoll(diceFileLoaded, diceResult, diceConfig);
                diceResult = null;
            } else {
                text.setText(getString(R.string.diceMessageReRoll));
                reRoll(indicesNeedReRoll);
                startDrawer();
            }
            return true;
        }
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private native String sendVertexShader(byte[] shader, int length);
    private native String sendFragmentShader(byte[] shader, int length);
    private native String initWindow(boolean useVulkan, Surface surface);
    private native String initPipeline();
    private native void destroyVulkan();
    private native void tellDrawerStop();
    private native void destroyModels();
    private native void recreateModels();
    private native void recreateSwapChain();
    private native void loadModel(String[] symbols);
    private native String addSymbols(String[] symbols, int nbrSymbols, int width, int height, int heightImage, int bitmapSize, byte[] bitmap);
    private native void roll();
    private native void reRoll(int[] indices);
    private native String initSensors();
}
