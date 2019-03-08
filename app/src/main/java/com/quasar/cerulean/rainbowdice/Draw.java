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

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.TreeSet;

import static android.graphics.Bitmap.Config.ALPHA_8;
import static android.graphics.Bitmap.Config.ARGB_4444;
import static android.graphics.Bitmap.Config.ARGB_8888;

public class Draw {
    private static final int MAX_TEXTURE_DIMENSION = 10000;
    private static final int TEXWIDTH = 128;
    private static final int TEX_PADDING = 30;
    private static final int TEX_BLANK_HEIGHT = 128;

    static final private class DiceTexture {
        public int width;
        public int height;
        public float[] textureCoordLeft;
        public float[] textureCoordRight;
        public float[] textureCoordTop;
        public float[] textureCoordBottom;
        public byte[] bytes;
        public DiceTexture(int inWidth, int inTextureHeight,
                           float[] inTextureCoordLeft, float[] inTextureCoordRight,
                           float[] inTextureCoordTop, float[] inTextureCoordBottom,
                           byte[] inBytes) {
            width = inWidth;
            height = inTextureHeight;
            textureCoordLeft = inTextureCoordLeft;
            textureCoordRight = inTextureCoordRight;
            textureCoordTop = inTextureCoordTop;
            textureCoordBottom = inTextureCoordBottom;
            bytes = inBytes;
        }
    }

    public Draw() {
    }

    public static String startDrawingRoll(DieConfiguration[] diceConfig, String diceName) {
        if (diceConfig == null || diceConfig.length == 0) {
            // somehow someone managed to crash the code by having dice config null.  I don't know
            // how they did this, but returning here avoids the crash.
            return "No dice configurations exist, please choose dice.";
        }

        Collection<String> symbolSet = getSymbols(diceConfig);

        DiceTexture texture = createTexture(symbolSet, changeAspectRatio(diceConfig));
        if (texture == null) {
            return "error: Could not create texture.";
        }

        String err = rollDice(diceName, diceConfig,
                symbolSet.toArray(new String[symbolSet.size()]),
                texture.width, texture.height, texture.textureCoordLeft, texture.textureCoordRight,
                texture.textureCoordTop, texture.textureCoordBottom, texture.bytes);
        if (err != null && err.length() != 0) {
            return err;
        }

        return null;
    }

    private static boolean changeAspectRatio(DieConfiguration[] diceConfig) {
        boolean change = true;
        for (DieConfiguration dieConfig : diceConfig) {
            int nbrSides = dieConfig.getNumberOfSides();
            if (nbrSides == 12 || nbrSides == 6 || nbrSides ==  3 || nbrSides == 2) {
                change = false;
                break;
            }
        }

        return change;
    }

    public static String startDrawingStoppedDice(DieConfiguration[] diceConfig, int[] faceIndicesUp) {
        if (diceConfig == null || diceConfig.length == 0) {
            // somehow someone managed to crash the code by having dice config null.  I don't know
            // how they did this, but returning here avoids the crash.
            return "No dice configurations exist, please choose dice.";
        }

        Collection<String> symbolSet = getSymbols(diceConfig);

        DiceTexture texture = createTexture(symbolSet, changeAspectRatio(diceConfig));
        if (texture == null) {
            return "error: Could not create texture.";
        }

        String err = drawStoppedDice(faceIndicesUp, diceConfig,
                symbolSet.toArray(new String[symbolSet.size()]),
                texture.width, texture.height,
                texture.textureCoordLeft, texture.textureCoordRight, texture.textureCoordTop,
                texture.textureCoordBottom, texture.bytes);
        if (err != null && err.length() != 0) {
            return err;
        }

        return null;
    }

    private static Collection<String> getSymbols(DieConfiguration[] diceConfig) {
        TreeSet<String> symbolSet = new TreeSet<>();

        // First we need to load all the textures in the texture Atlas (in cpp), then
        // we can load the models.  Since the model depends on the size of the textures.
        for (DieConfiguration dieConfig : diceConfig) {
            for (int i = 0; i < dieConfig.getNumberOfSides(); i++) {
                symbolSet.add(dieConfig.getSide(i).symbol());
            }
        }

        return symbolSet;
    }

    private static DiceTexture createTexture(Collection<String> symbolSet, boolean changeAspectRatio) {
        int totalNbrImages = symbolSet.size();
        int textureHeight = 0;
        int textureWidth = 0;
        int totalTextureHeight = 0;
        Paint paint = new Paint(/*Paint.ANTI_ALIAS_FLAG*/);
        paint.setTextAlign(Paint.Align.CENTER);
        int i = 0;
        float[] textSizes = new float[symbolSet.size()];
        int maxHeight = 0;
        int maxWidth = 0;
        int[] textureCoordTop = new int[symbolSet.size()];
        int[] textureCoordBottom = new int[symbolSet.size()];
        int[] textureCoordLeft = new int[symbolSet.size()];
        int[] textureCoordRight = new int[symbolSet.size()];
        int[] textNumberOfLines = new int[symbolSet.size()];
        int totalNbrTexturesDown = (int)Math.ceil(Math.sqrt(totalNbrImages));
        int nbrTexturesDown = 0;
        for (String symbol : symbolSet) {
            // find the longest line of text in a sequence of lines
            int length = symbol.length();
            int nbrLines = 0;
            int start = 0;
            int end = symbol.indexOf("\n", 0);
            if (end == -1) {
                end = length;
            }
            int longestLineStart = 0;
            int longestLineEnd = end;

            float maxLineWidth = 0;
            do {
                nbrLines++;
                float width = paint.measureText(symbol, start, end);
                if (width > maxLineWidth) {
                    maxLineWidth = width;
                    longestLineStart = start;
                    longestLineEnd = end;
                }

                if (end == length) {
                    break;
                }

                start = end+1;
                end = symbol.indexOf("\n", end + 1);
                if (end == -1) {
                    end = length;
                }
            } while (true);
            textNumberOfLines[i] = nbrLines;

            // find the max size for the text.
            float maxTextSize = TEXWIDTH;
            float minTextSize = 5.0f;
            float textSize = (maxTextSize + minTextSize)/2;
            paint.setTextSize(textSize);
            do {
                paint.setTextSize(textSize);
                if (paint.measureText(symbol, longestLineStart, longestLineEnd) >= TEXWIDTH) {
                    maxTextSize = textSize;
                } else {
                    minTextSize = textSize;
                }
                textSize = (maxTextSize + minTextSize)/2;
            } while (maxTextSize - minTextSize > 0.5f);
            textSizes[i] = minTextSize - 0.5f;

            paint.setTextSize(textSizes[i]);
            int height = (int) Math.ceil((paint.descent() - paint.ascent())*nbrLines + TEX_PADDING);
            int width = TEXWIDTH + TEX_PADDING;
            if (!changeAspectRatio) {
                if (height < width) {
                    height = width;
                } else {
                    width = height;
                }
            }

            if (maxWidth < width) {
                maxWidth = width;
            }

            if (textureHeight + height + TEX_BLANK_HEIGHT > MAX_TEXTURE_DIMENSION ||
                    nbrTexturesDown >= totalNbrTexturesDown) {
                if (totalTextureHeight < textureHeight) {
                    totalTextureHeight = textureHeight;
                }
                nbrTexturesDown = 0;
                textureHeight = 0;
                textureWidth += maxWidth + TEX_BLANK_HEIGHT;
                maxWidth = width;

                if (textureWidth > MAX_TEXTURE_DIMENSION) {
                    return null;
                }
            }

            if (textureHeight > 0) {
                textureHeight += TEX_BLANK_HEIGHT;
            }

            textureCoordTop[i] = textureHeight;
            textureHeight += height;
            textureCoordBottom[i] = textureHeight;
            textureCoordLeft[i] = textureWidth;
            textureCoordRight[i] = textureWidth + width;
            if (height > maxHeight) {
                maxHeight = height;
            }

            i++;
            nbrTexturesDown++;
        }

        if (totalTextureHeight < textureHeight) {
            totalTextureHeight = textureHeight;
        }
        textureWidth += maxWidth;

        Bitmap bitmap;
        try {
            bitmap = Bitmap.createBitmap(textureWidth, totalTextureHeight, ARGB_8888);
        } catch (Exception e) {
            return null;
        }
        Canvas canvas = new Canvas(bitmap);

        // fill the image with blank including a zero alpha.  The shader uses this to tell if we
        // are in text or open space.
        canvas.drawARGB(0,0,0,0);

        float textureCoordTopNorm[] = new float[totalNbrImages];
        float textureCoordBottomNorm[] = new float[totalNbrImages];
        float textureCoordLeftNorm[] = new float[totalNbrImages];
        float textureCoordRightNorm[] = new float[totalNbrImages];
        i = 0;
        for (String symbol : symbolSet) {
            paint.setTextSize(textSizes[i]);
            int length = symbol.length();
            int nbrLines = 0;
            int start = 0;
            int end = symbol.indexOf("\n", 0);
            if (end == -1) {
                end = length;
            }
            int middle = (textureCoordBottom[i] - textureCoordTop[i] -
                    (int)Math.ceil((paint.descent() - paint.ascent())*textNumberOfLines[i]))/2;
            do {
                nbrLines++;
                canvas.drawText(symbol,
                        start,
                        end,
                        (textureCoordRight[i] + textureCoordLeft[i])/2,
                        textureCoordTop[i] - paint.ascent()*nbrLines + middle,
                        paint);
                if (end == length) {
                    break;
                }

                start = end+1;
                end = symbol.indexOf("\n", end + 1);
                if (end == -1) {
                    end = length;
                }
            } while (true);
            textureCoordTopNorm[i] = textureCoordTop[i]/(float)totalTextureHeight;
            textureCoordBottomNorm[i] = textureCoordBottom[i]/(float)totalTextureHeight;
            textureCoordLeftNorm[i] = textureCoordLeft[i]/(float)textureWidth;
            textureCoordRightNorm[i] = textureCoordRight[i]/(float)textureWidth;
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

        return new DiceTexture(textureWidth, totalTextureHeight,
                textureCoordLeftNorm, textureCoordRightNorm,
                textureCoordTopNorm, textureCoordBottomNorm, bytes);
    }

    private static native String rollDice(String diceName, DieConfiguration[] diceConfig,
                                   String[] symbols, int width, int height,
                                   float[] textureCoordLeft, float[] textureCoordRight,
                                   float[] textureCoordTop, float[] textureCoordBottom,  byte[] bitmap);
    private static native String drawStoppedDice(int[] symbolIndicesUp, DieConfiguration[] diceConfig,
                                          String[] symbolsTexture, int width, int height,
                                          float[] textureCoordLeft, float[] textureCoordRight,
                                          float[] textureCoordTop, float[] textureCoordBottom,  byte[] bitmap);
    public static native void tellDrawerStop();

    public static native String tellDrawerSurfaceDirty(Surface jsurface);

    public static native String tellDrawerSurfaceChanged(int width, int height);

    public static native void scroll(float distanceX, float distanceY);

    public static native void scale(float scaleFactor);

    public static native void tapDice(float x, float y);
}
