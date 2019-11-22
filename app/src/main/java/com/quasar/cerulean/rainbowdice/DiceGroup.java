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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

public class DiceGroup {
    private final static String versionAttributeName = "version";
    private final static String diceArrayAttributeName = "dice";
    private DieConfiguration[] dice;

    public DiceGroup(DieConfiguration[] inDice) {
        dice = inDice;
    }

    public static DiceGroup fromJson(JSONObject jsonObj) throws JSONException {
        int version = jsonObj.getInt(versionAttributeName);
        JSONArray jsonArray = jsonObj.getJSONArray(diceArrayAttributeName);
        DieConfiguration[] configs = DieConfiguration.fromJsonArray(jsonArray, version);

        return new DiceGroup(configs);
    }

    public static DiceGroup fromDiceConfigurationV0JsonArray(JSONArray jsonArray) throws JSONException {
        DieConfiguration[] configs = DieConfiguration.fromJsonArray(jsonArray, 0);

        return new DiceGroup(configs);
    }

    public static DiceGroup loadFromFile(FileInputStream inputStream) {
        StringBuffer json = new StringBuffer();

        try {
            byte[] bytes = new byte[1024];
            int len;
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, "UTF-8"));
                }
            }
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + e.getMessage());
            return null;
        }

        // in version 1, we have an enclosing JSON object.
        JSONObject jsonObj = null;
        try {
            jsonObj = new JSONObject(json.toString());
            return DiceGroup.fromJson(jsonObj);
        } catch (JSONException e) {
        }

        // in version 0, there was only a dice configuration array and no other attributes
        // including no version number.
        try {
            JSONArray jsonArray = new JSONArray(json.toString());
            return DiceGroup.fromDiceConfigurationV0JsonArray(jsonArray);
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + e.getMessage());
            return null;
        }
    }

    public JSONObject toJson() throws JSONException {
        JSONObject jsonObj = new JSONObject();
        JSONArray jsonArray = new JSONArray();
        for (DieConfiguration die : dice) {
            JSONObject obj = die.toJSON();
            jsonArray.put(obj);
        }
        jsonObj.put(versionAttributeName, 1);
        jsonObj.put(diceArrayAttributeName, jsonArray);
        return jsonObj;
    }

    public void saveToFile(FileOutputStream outputStream) {
        String json;
        try {
            JSONObject jsonObj = toJson();
            json = jsonObj.toString();
        } catch (JSONException e) {
            System.out.println("Exception in writing out JSON: " + e.getMessage());
            return;
        }

        try {
            outputStream.write(json.getBytes());
        } catch (IOException e) {
            System.out.println("Exception on writing to file.  Message: " + e.getMessage());
            return;
        }
    }

    public DieConfiguration[] dieConfigurations() {
        return dice;
    }
}
