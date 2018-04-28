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

import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public class ConfigurationFile {
    public static final String configFile = "diceConfigurationFile";
    private static final String favorite1 = "favorite1";
    private static final String favorite2 = "favorite2";
    private static final String favorite3 = "favorite3";
    private static final String theme = "theme";

    private String fav1File;
    private String fav2File;
    private String fav3File;
    private String themeName;

    private Context ctx;

    public ConfigurationFile(Context inCtx) {
        ctx = inCtx;
        StringBuffer json = new StringBuffer();

        try {
            byte[] bytes = new byte[1024];
            int len;
            FileInputStream inputStream = ctx.openFileInput(configFile);
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, "UTF-8"));
                }
            }
            inputStream.close();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + configFile + " message: " + e.getMessage());
            return;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + configFile + " message: " + e.getMessage());
            return;
        }

        try {
            JSONObject obj = new JSONObject(json.toString());
            fav1File = obj.getString(favorite1);
            fav2File = obj.getString(favorite2);
            fav3File = obj.getString(favorite3);
            themeName = obj.getString(theme);
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + configFile + " message: " + e.getMessage());
            return;
        }
    }

    public void writeFile() {
        String json;
        try {
            JSONObject obj = new JSONObject();
            obj.put(favorite1, fav1File);
            obj.put(favorite2, fav2File);
            obj.put(favorite3, fav3File);
            obj.put(theme, themeName);
            json = obj.toString();
        } catch (JSONException e) {
            System.out.println("Exception in writing out JSON: " + e.getMessage());
            return;
        }

        try {
            FileOutputStream outputStream = ctx.openFileOutput(configFile, Context.MODE_PRIVATE);
            outputStream.write(json.getBytes());
            outputStream.close();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + configFile + " message: " + e.getMessage());
            return;
        } catch (IOException e) {
            System.out.println("Exception on writing to file: " + configFile + " message: " + e.getMessage());
            return;
        }

    }

    public String getFavorite1() {
        return fav1File;
    }

    public String getFavorite2() {
        return fav2File;
    }

    public String getFavorite3() {
        return fav3File;
    }

    public String getTheme() {
        return themeName;
    }

    public void setFavorite1(String in) {
        fav1File = in;
    }

    public void setFavorite2(String in) {
        fav2File = in;
    }

    public void setFavorite3(String in) {
        fav3File = in;
    }

    public void setThemeName(String in) {
        themeName = in;
    }
}
