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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.LinkedList;

public class DataExporter {
    private static final String configFileType = "ConfigFile";
    private static final String logFileType = "LogFile";

    public static boolean exportAllToFile(Context ctx, FileOutputStream fout) {
        try {
            JSONObject topLevel = new JSONObject();
            DiceConfigurationManager mgr = new DiceConfigurationManager(ctx);
            topLevel.put(configFileType, mgr.getConfigJSON());

            LinkedList<String> diceConfigNames = mgr.getDiceList();
            for (String diceConfigName : diceConfigNames) {
                DiceGroup dice = readDiceGroupFromFile(ctx, diceConfigName);
                if (dice != null) {
                    topLevel.put(diceConfigName, dice.toJson());
                }
            }

            LogFile logFile = new LogFile(ctx);
            topLevel.put(logFileType, logFile.toJSON());

            String jsonString = topLevel.toString();
            fout.write((new String("# Written by Rainbow Dice\n")).getBytes());
            fout.write(jsonString.getBytes());
        } catch (JSONException e) {
            return false;
        } catch (IOException e) {
            return false;
        }

        return true;
    }

    public static boolean importAllFromFile(Context ctx, InputStream fin) {
        String json = readDataFromFile(fin);
        if (json == null) {
            return false;
        }

        try {
            JSONObject obj = new JSONObject(json);

            // store the config file read in from the input file
            JSONObject configFileJSON = obj.getJSONObject(configFileType);
            ConfigurationFile configFile = new ConfigurationFile(ctx, configFileJSON);
            configFile.writeFile();

            // store the log file read in from the input file
            JSONArray logFileJSON = obj.getJSONArray(logFileType);
            LogFile logFile = new LogFile(ctx, logFileJSON);
            logFile.writeFile();

            // Delete the dice files already there.
            String[] excludeFiles = new String[2];
            excludeFiles[0] = ConfigurationFile.configFile;
            excludeFiles[1] = LogFile.diceLogFilename;
            File[] filearr = ctx.getFilesDir().listFiles();
            if (filearr != null) {
                for (File file : filearr) {
                    boolean found = false;
                    for (String filename : excludeFiles) {
                        if (file.getName().equals(filename)) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        ctx.deleteFile(file.getName());
                    }
                }
            }

            for (String filename : configFile.getDiceList()) {
                try {
                    JSONObject diceConfigJSON = obj.getJSONObject(filename);
                    FileOutputStream fout = ctx.openFileOutput(filename, Context.MODE_PRIVATE);
                    DiceGroup.fromJson(diceConfigJSON).saveToFile(fout);
                } catch (JSONException e) {
                    // ignore
                } catch (IOException e) {
                    // ignore
                }
            }
        } catch (JSONException e) {
            return false;
        }

        return true;
    }

    public static String readDataFromFile(InputStream inputStream) {
        try {
            StringBuilder json = new StringBuilder();
            byte[] bytes = new byte[1024];
            int len;
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, "UTF-8"));
                }
            }

            return json.toString();
        } catch (FileNotFoundException e) {
            return null;
        } catch (IOException e) {
            return null;
        }

    }

    private static DiceGroup readDiceGroupFromFile(Context ctx, String filename) {
        try {
            FileInputStream inputStream = ctx.openFileInput(filename);
            DiceGroup diceGroup = DiceGroup.loadFromFile(inputStream);
            inputStream.close();

            return diceGroup;
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
            return null;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
            return null;
        }
    }
}
