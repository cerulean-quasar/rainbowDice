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
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Locale;
import java.util.TreeSet;

import static android.graphics.Bitmap.Config.ALPHA_8;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public static final String DICE_CONFIGURATION = "DiceConfiguration";
    public static final String DICE_FILENAME = "DiceFileName";
    private static final String NATIVE = "Rainbow Dice Native";
    private static final int DICE_CONFIGURATION_ACTIVITY = 1;

    private boolean surfaceReady = false;
    private Thread drawer;
    private DieConfiguration[] diceConfig;
    private DiceResult diceResult = null;
    private String diceConfigFilename = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        diceConfig = new DieConfiguration[1];
        diceConfig[0] = new DieConfiguration(4, 6,1, 1,0,true);
        setContentView(R.layout.activity_main);

        SurfaceView drawSurfaceView = findViewById(R.id.drawingSurface);
        SurfaceHolder drawSurfaceHolder = drawSurfaceView.getHolder();
        drawSurfaceHolder.addCallback(new MySurfaceCallback(this));
    }

    @Override
    protected void onPause() {
        super.onPause();
        joinDrawer();
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
        rollTheDice();
        startDrawer();
    }

    @Override
    protected void onResume() {
        super.onResume();
        diceResult = null;
        rollTheDice();
        startDrawer();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        joinDrawer();
        destroySurface();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == DICE_CONFIGURATION_ACTIVITY) {
            if (resultCode == RESULT_CANCELED) {
                // The user cancelled editing
                return;
            }
            diceConfigFilename = data.getStringExtra(DICE_FILENAME);
            Parcelable[] parcels = data.getParcelableArrayExtra(DICE_CONFIGURATION);
            diceConfig = new DieConfiguration[parcels.length];
            int i = 0;
            for (Parcelable parcel : parcels) {
                diceConfig[i++] = (DieConfiguration)parcel;
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
        intent.putExtra(DICE_CONFIGURATION, diceConfig);
        if (diceConfigFilename != null) {
            intent.putExtra(DICE_FILENAME, diceConfigFilename);
        }
        startActivityForResult(intent, DICE_CONFIGURATION_ACTIVITY);
    }

    public void onRoll(MenuItem item) {
        rollTheDice();
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

    private void loadModelsAndTextures() {
        int TEXWIDTH = 50;
        int TEXHEIGHT = 25;

        TreeSet<String> symbolSet = new TreeSet<String>();

        // First we need to load all the textures in the texture Atlas (in cpp), then
        // we can load the models.  Since the model depends on the size of the textures.
        for (DieConfiguration dieConfig : diceConfig) {
            int nbrSides = dieConfig.getNumberOfSides();
            int startOn = dieConfig.getStartAt();
            int increment = dieConfig.getIncrement();
            for (int j = 0; j < nbrSides; j++) {
                String symbol = String.format(Locale.getDefault(), "%d", startOn + increment*j);
                symbolSet.add(symbol);
            }
        }

        for (String symbol : symbolSet) {
            Bitmap bitmap = Bitmap.createBitmap(TEXWIDTH, TEXHEIGHT, ALPHA_8);
            Canvas canvas = new Canvas(bitmap);
            Paint paint = new Paint();
            paint.setUnderlineText(true);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTextSize(18.0f);
            canvas.drawText(symbol, 20, 15, paint);
            int bitmapSize = bitmap.getAllocationByteCount();
            ByteBuffer imageBuffer = ByteBuffer.allocate(bitmapSize);
            bitmap.copyPixelsToBuffer(imageBuffer);
            byte[] bytes = new byte[bitmapSize];
            try {
                imageBuffer.position(0);
                imageBuffer.get(bytes);
            } catch (Exception e) {
                publishError(e.getMessage() != null ? e.getMessage() : e.getClass().toString());
                return;
            }
            String err = addSymbol(symbol, TEXWIDTH, TEXHEIGHT, bytes);
            if (err != null && err.length() != 0) {
                publishError(err);
                return;
            }
        }

        // now load the models. Some of the vertices depend on the size of the texture image.
        for (DieConfiguration dieConfig : diceConfig) {
            int nbrDice = dieConfig.getNumberOfDice();
            int nbrSides = dieConfig.getNumberOfSides();
            int startOn = dieConfig.getStartAt();
            int increment = dieConfig.getIncrement();
            String[] symbols = new String[nbrSides];
            for (int j = 0; j < nbrSides; j++) {
                symbols[j] = String.format(Locale.getDefault(), "%d", startOn + increment*j);
            }
            for (int j = 0; j < nbrDice; j++){
                loadModel(symbols);
            }
        }

    }

    public void publishError(String err) {
        if (err != null && err.length() > 0) {
            TextView view = findViewById(R.id.rollResult);
            view.setText(err);
        }
    }

    public void startDrawing(SurfaceHolder holder) {
        byte[] vertShader = readAssetFile("shaders/shader.vert.spv");
        byte[] fragShader = readAssetFile("shaders/shader.frag.spv");
        Surface drawSurface = holder.getSurface();
        String err = initWindow(drawSurface);
        if (err != null && err.length() != 0) {
            publishError(err);
            return;
        }

        loadModelsAndTextures();

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
        startDrawer();
    }

    private byte[] readAssetFile(String filename) {
        try {
            InputStream inputStream = getAssets().open(filename);
            ByteBufferBuilder bytes = new ByteBufferBuilder(1024);
            bytes.readToBuffer(inputStream);
            return bytes.getBytes();
        } catch (IOException e) {
            System.out.println("Error in opening asset file: " + filename + " message: " + e.getMessage());
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
    private native String initWindow(Surface surface);
    private native String initPipeline();
    private native void destroyVulkan();
    private native void tellDrawerStop();
    private native void destroyModels();
    private native void recreateModels();
    private native void recreateSwapChain();
    private native void loadModel(String[] symbols);
    private native String addSymbol(String symbol, int width, int height, byte[] bitmap);
    private native void roll();
    private native void reRoll(int[] indices);
    private native String initSensors();
}