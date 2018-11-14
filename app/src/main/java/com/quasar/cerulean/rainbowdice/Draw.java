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

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collection;
import java.util.TreeSet;

import static android.graphics.Bitmap.Config.ALPHA_8;

public class Draw implements Runnable {
    private static final int TEXWIDTH = 64;
    private static final int TEXHEIGHT = 64;
    private static final int TEX_BLANK_HEIGHT = 56;

    private Handler m_notify;
    private SurfaceHolder m_surfaceHolder;
    private DieConfiguration[] m_diceConfig;
    private AssetManager m_assetManager;
    private int[] m_indicesNeedsReroll;

    public Draw(Handler inNotify, SurfaceHolder inSurfaceHolder, DieConfiguration[] inDiceConfig,
                AssetManager inAssetManager, int[] inIndicesNeedsReroll) {
        m_notify = inNotify;
        m_surfaceHolder = inSurfaceHolder;
        m_diceConfig = inDiceConfig;
        m_assetManager = inAssetManager;
        m_indicesNeedsReroll = inIndicesNeedsReroll;
    }

    public void run() {
        String result;
        result = startDrawingRoll();
        if (result == null || result.length() == 0) {
            // no result and we exited, something must be wrong.  Just return, we are done.
            return;
        }
        // tell the main thread, a result has occurred.
        Bundle bundle = new Bundle();
        bundle.putString("results", result);
        Message msg = Message.obtain();
        msg.setData(bundle);
        m_notify.sendMessage(msg);
    }

    private String startDrawingRoll() {
        if (m_diceConfig == null || m_diceConfig.length == 0) {
            // somehow someone managed to crash the code by having dice config null.  I don't know
            // how they did this, but returning here avoids the crash.
            return "No dice configurations exist, please choose dice.";
        }

        Collection<String> symbolSet = getSymbols();

        byte[] texture = createTexture(symbolSet);
        if (texture == null) {
            return "Could not create texture.";
        }

        Surface drawSurface = m_surfaceHolder.getSurface();
        String err = rollDice(drawSurface, m_assetManager, m_diceConfig,
                symbolSet.toArray(new String[symbolSet.size()]),
                TEXWIDTH, (TEXHEIGHT+TEX_BLANK_HEIGHT)*symbolSet.size(), TEXHEIGHT,
                TEX_BLANK_HEIGHT, texture);
        if (err != null && err.length() != 0) {
            return err;
        }

        return null;
    }

    public String startDrawingStoppedDice(String[] symbolsUp) {
        if (m_diceConfig == null || m_diceConfig.length == 0) {
            // somehow someone managed to crash the code by having dice config null.  I don't know
            // how they did this, but returning here avoids the crash.
            return "No dice configurations exist, please choose dice.";
        }

        Collection<String> symbolSet = getSymbols();

        byte[] texture = createTexture(symbolSet);
        if (texture == null) {
            return "Could not create texture.";
        }

        Surface drawSurface = m_surfaceHolder.getSurface();
        String err = drawStoppedDice(symbolsUp, drawSurface, m_assetManager, m_diceConfig,
                symbolSet.toArray(new String[symbolSet.size()]),
                TEXWIDTH, (TEXHEIGHT+TEX_BLANK_HEIGHT)*symbolSet.size(), TEXHEIGHT,
                TEX_BLANK_HEIGHT, texture);
        if (err != null && err.length() != 0) {
            return err;
        }

        return null;
    }

    private Collection<String> getSymbols() {
        TreeSet<String> symbolSet = new TreeSet<>();

        // First we need to load all the textures in the texture Atlas (in cpp), then
        // we can load the models.  Since the model depends on the size of the textures.
        for (DieConfiguration dieConfig : m_diceConfig) {
            for (int i=0; i < dieConfig.getNumberDiceInRepresentation(); i++) {
                String[] symbols = dieConfig.getSymbols(i);
                symbolSet.addAll(Arrays.asList(symbols));
            }
        }

        return symbolSet;
    }

    private byte[] createTexture(Collection<String> symbolSet) {
        Bitmap bitmap;
        try {
            bitmap = Bitmap.createBitmap(TEXWIDTH, (TEXHEIGHT+TEX_BLANK_HEIGHT) * symbolSet.size(), ALPHA_8);
        } catch (Exception e) {
            return null;
        }
        Canvas canvas = new Canvas(bitmap);
        Paint paint = new Paint();
        paint.setUnderlineText(true);
        paint.setFakeBoldText(true);
        paint.setTextAlign(Paint.Align.CENTER);
        canvas.drawARGB(0,0,0,0);
        int i = 0;
        for (String symbol : symbolSet) {
            if (symbol.length() < 3) {
                paint.setTextSize(48.0f);
            } else {
                paint.setTextSize(25.0f);
            }
            canvas.drawText(symbol, 28, i*(TEXHEIGHT+TEX_BLANK_HEIGHT) + 47, paint);
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
            return null;
        }

        return bytes;
    }

    private native String rollDice(Surface surface, AssetManager manager, DieConfiguration[] diceConfig,
                                   String[] symbols, int width, int height, int heightImage,
                                   int heightBlankSpace,  byte[] bitmap);
    private native String drawStoppedDice(String[] symbolsUp, Surface surface, AssetManager manager, DieConfiguration[] diceConfig,
                                          String[] symbolsTexture, int width, int height, int heightImage,
                                          int heightBlankSpace,  byte[] bitmap);
}
