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

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.LinkedList;

public class DiceConfigurationManager {
    private Context ctx;
    private ConfigurationFile config;
    private LinkedList<String> diceFileList;
    public DiceConfigurationManager(Context inCtx) {
        ctx = inCtx;
        config = new ConfigurationFile(ctx);
        diceFileList = new LinkedList<>();
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
                    diceFileList.add(file.getName());
                }
            }
        }

        LinkedList<String> diceConfigList = config.getDiceList();
        if (diceConfigList != null && diceConfigList.size() > 0) {
            LinkedList<String> needsRemove = new LinkedList<>();
            for (String dice : diceConfigList) {
                if (!diceFileList.contains(dice)) {
                    needsRemove.add(dice);
                }
            }

            for (String dice : needsRemove) {
                config.remove(dice);
            }

            for (String dice : diceFileList) {
                if (!diceConfigList.contains(dice)) {
                    config.add(dice);
                }
            }
        }
    }

    public void addDice(String name, DiceGroup configs) {
        try {
            FileOutputStream outputStream = ctx.openFileOutput(name, Context.MODE_PRIVATE);
            configs.saveToFile(outputStream);
            outputStream.close();
        } catch (IOException e) {
            System.out.println("Exception on writing to file: " + name + " message: " + e.getMessage());
            return;
        }
        config.add(name);
    }

    public void deleteDice(String name) {
        config.remove(name);
        ctx.deleteFile(name);
    }

    public boolean renameDice(String oldName, String newName) {
        File oldFile = new File(ctx.getFilesDir().getPath().concat("/").concat(oldName));
        File newFile = new File(ctx.getFilesDir().getPath().concat("/").concat(newName));

        boolean result = oldFile.renameTo(newFile);
        if (result) {
            config.rename(oldName, newName);
        }

        return result;
    }

    public void moveDice(String item, int to) {
        config.moveDice(item, to);
    }

    public LinkedList<String> getDiceList() {
        return config.getDiceList();
    }

    public String getTheme() {
        return config.getTheme();
    }

    public int bonus() { return config.bonus(); }

    public void setBonus(int inBonus) { config.setBonus(inBonus); }

    public void save() {
        config.writeFile();
    }

    public JSONObject getConfigJSON() throws JSONException {
        return config.toJSON();
    }
}
