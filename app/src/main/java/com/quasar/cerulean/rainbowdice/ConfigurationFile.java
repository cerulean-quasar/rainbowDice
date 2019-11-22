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

import android.content.Context;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.LinkedList;

public class ConfigurationFile {
    public static final String configFile = "diceConfigurationFile";
    private static final String version = "version";

    // for version 0.
    private static final String favorite1 = "favorite1";
    private static final String favorite2 = "favorite2";
    private static final String favorite3 = "favorite3";

    // for version 1
    private static final String diceList = "diceList";
    private static final String theme = "theme";
    // for version 1, but not present in older code
    private static final String useLegacy = "useLegacy";
    private static final String useGravity = "useGravity";
    private static final String drawRollingDice = "drawRollingDice";
    private static final String bonusValue = "bonusValue";
    private static final String reverseGravityStr = "reverseGravity";

    private LinkedList<String> diceConfigList;
    private String themeName;
    private boolean m_useLegacy;
    private boolean m_useGravity;
    private boolean m_drawRollingDice;
    private int m_bonus;
    private boolean m_reverseGravity;

    private Context ctx;

    public ConfigurationFile(Context inCtx) {
        ctx = inCtx;
        diceConfigList = new LinkedList<>();
        themeName = "Space";
        m_useGravity = true;
        m_useLegacy = false;
        m_drawRollingDice = true;
        m_bonus = 0;
        m_reverseGravity = false;

        StringBuilder json = new StringBuilder();
        try {
            byte[] bytes = new byte[1024];
            int len;
            FileInputStream inputStream = ctx.openFileInput(configFile);
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, StandardCharsets.UTF_8));
                }
            }
            inputStream.close();
        } catch (FileNotFoundException e) {
            return;
        } catch (IOException e) {
            return;
        }

        try {
            JSONObject obj = new JSONObject(json.toString());
            int versionNbr;
            try {
                versionNbr = obj.getInt(version);
            } catch (JSONException e) {
                // if there is no version, then we are using version 0.
                versionNbr = 0;
            }

            if (versionNbr == 0) {
                loadVersion0(obj);
            } else if (versionNbr == 1) {
                loadVersion1(obj);
            }
        } catch (JSONException e) {
        }
    }

    private void loadVersion0(JSONObject obj) throws JSONException {
        String fav1File = obj.getString(favorite1);
        String fav2File = obj.getString(favorite2);
        String fav3File = obj.getString(favorite3);

        diceConfigList.add(fav1File);
        diceConfigList.add(fav2File);
        diceConfigList.add(fav3File);
        themeName = obj.getString(theme);
    }

    private void loadVersion1(JSONObject obj) throws JSONException {
        JSONArray arr = obj.getJSONArray(diceList);
        for (int i = 0; i < arr.length(); i ++) {
            String dice = arr.getString(i);
            diceConfigList.add(dice);
        }

        if (obj.has(theme)) {
            String tmp = obj.getString(theme);
            if (!tmp.isEmpty()) {
                themeName = tmp;
            }
        }

        if (obj.has(useLegacy)) {
            m_useLegacy = obj.getBoolean(useLegacy);
        }

        if (obj.has(useGravity)) {
            m_useGravity = obj.getBoolean(useGravity);
        }

        if (obj.has(drawRollingDice)) {
            m_drawRollingDice = obj.getBoolean(drawRollingDice);
        }

        if (obj.has(bonusValue)) {
            m_bonus = obj.getInt(bonusValue);
        }

        if (obj.has(reverseGravityStr)) {
            m_reverseGravity = obj.getBoolean(reverseGravityStr);
        } else {
            // If the file exists, this is a previous install... keep the gravity reversed for
            // them since that was the way it used to function.
            m_reverseGravity = true;
        }
    }

    public void writeFile() {
        String json;
        try {
            JSONObject obj = new JSONObject();

            obj.put(version, 1);

            JSONArray arr = new JSONArray();
            for (String dice : diceConfigList) {
                arr.put(dice);
            }

            obj.put(diceList, arr);
            obj.put(theme, themeName);
            obj.put(useLegacy, m_useLegacy);
            obj.put(useGravity, m_useGravity);
            obj.put(drawRollingDice, m_drawRollingDice);
            obj.put(bonusValue, m_bonus);
            obj.put(reverseGravityStr, m_reverseGravity);

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

    public LinkedList<String> getDiceList() {
        return diceConfigList;
    }

    public void add(String name) {
        if (!diceList.contains(name)) {
            diceConfigList.add(name);
        }
    }

    public void remove(String name) {
        diceConfigList.remove(name);
    }

    public void rename(String oldName, String newName) {
        int i = diceConfigList.indexOf(oldName);
        diceConfigList.remove(i);
        diceConfigList.add(i, newName);
    }

    public void moveDice(String item, int to) {
        int i = diceConfigList.indexOf(item);
        if (i == -1) {
            return;
        }
        diceConfigList.remove(i);
        if (i < to) {
            to -= 1;
        }
        diceConfigList.add(to, item);
    }

    public String getTheme() {
        return themeName;
    }

    public boolean useLegacy() {
        return m_useLegacy;
    }

    public boolean useGravity() {
        return m_useGravity;
    }

    public boolean drawRollingDice() {
        return m_drawRollingDice;
    }

    public int bonus() {
        return m_bonus;
    }

    public boolean reverseGravity() {
        return m_reverseGravity;
    }

    public void setThemeName(String in) {
        themeName = in;
    }

    public void setUseLegacy(boolean in) {
        m_useLegacy = in;
    }

    public void setUseGravity(boolean in) {
        m_useGravity = in;
    }

    public void setDrawRollingDice(boolean in) {
        m_drawRollingDice = in;
    }

    public void setBonus(int in) {
        m_bonus = in;
    }

    public void setReverseGravity(boolean in) {
        m_reverseGravity = in;
    }
}
