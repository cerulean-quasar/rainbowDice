package com.quasar.cerulean.rainbowdice;

import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.LinkedList;

public class DiceConfigurationManager {
    Context ctx;
    ConfigurationFile config;
    LinkedList<String> diceFileList;
    public DiceConfigurationManager(Context inCtx) {
        ctx = inCtx;
        config = new ConfigurationFile(ctx);
        diceFileList = new LinkedList<>();
        String[] excludeFiles = new String[2];
        excludeFiles[0] = ConfigurationFile.configFile;
        excludeFiles[1] = LogFile.diceLogFilename;

        File[] filearr = ctx.getFilesDir().listFiles();
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

        LinkedList<String> diceConfigList = config.getDiceList();
        if (diceConfigList != null && diceConfigList.size() > 0) {
            for (String dice : diceConfigList) {
                if (!diceFileList.contains(dice)) {
                    config.remove(dice);
                }
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

    public void renameDice(String oldName, String newName) {
        config.rename(oldName, newName);
        File oldFile = new File(ctx.getFilesDir().getPath().concat("/").concat(oldName));
        File newFile = new File(ctx.getFilesDir().getPath().concat("/").concat(newName));

        oldFile.renameTo(newFile);
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

    public void save() {
        config.writeFile();
    }

}
