package com.quasar.cerulean.rainbowdice;

import android.content.Context;
import android.content.res.Resources;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

public class LogFile {
    public static final String diceLogFilename = "diceRollResultLogFileName";

    private static final String jsonTime = "time";
    private static final String jsonDiceName = "name";
    private static final String jsonDiceRepresentation = "representation";
    private static final String jsonRollResult = "result";
    private static final int MAX_SIZE = 10;

    private ArrayList<LogItem> logItems;
    private Context ctx;

    public class LogItem {
        String logTime;
        String diceName;
        String diceRepresentation;
        String rollResults;
    }

    public LogFile(Context inCtx) {
        ctx = inCtx;
        logItems = new ArrayList<>();
        loadFile();
    }

    private void loadFile() {
        StringBuilder json = new StringBuilder();

        try {
            byte[] bytes = new byte[1024];
            int len;
            FileInputStream inputStream = ctx.openFileInput(diceLogFilename);
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, "UTF-8"));
                }
            }
            inputStream.close();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + diceLogFilename + " message: " + e.getMessage());
            return;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + diceLogFilename + " message: " + e.getMessage());
            return;
        }

        try {
            JSONArray arr = new JSONArray(json.toString());
            for (int i=0; i < arr.length(); i++) {
                JSONObject obj = arr.getJSONObject(i);
                LogItem item = new LogItem();
                item.logTime = obj.getString(jsonTime);
                item.diceName = obj.getString(jsonDiceName);
                item.diceRepresentation = obj.getString(jsonDiceRepresentation);
                item.rollResults = obj.getString(jsonRollResult);
                logItems.add(item);
            }
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + diceLogFilename + " message: " + e.getMessage());
            return;
        }
    }

    public void writeFile() {
        String json;
        try {
            JSONArray arr = new JSONArray();
            for (LogItem logItem: logItems) {
                JSONObject obj = new JSONObject();
                obj.put(jsonTime, logItem.logTime);
                obj.put(jsonDiceName, logItem.diceName);
                obj.put(jsonDiceRepresentation, logItem.diceRepresentation);
                obj.put(jsonRollResult, logItem.rollResults);
                arr.put(obj);
            }
            json = arr.toString();
        } catch (JSONException e) {
            System.out.println("Exception in writing out JSON: " + e.getMessage());
            return;
        }

        try {
            FileOutputStream outputStream = ctx.openFileOutput(diceLogFilename, Context.MODE_PRIVATE);
            outputStream.write(json.getBytes());
            outputStream.close();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + diceLogFilename + " message: " + e.getMessage());
            return;
        } catch (IOException e) {
            System.out.println("Exception on writing to file: " + diceLogFilename + " message: " + e.getMessage());
            return;
        }

    }

    public void addRoll(String diceName, DiceResult result, DieConfiguration[] diceConfigurations) {
        DateFormat time = SimpleDateFormat.getDateTimeInstance();
        LogItem logItem = new LogItem();
        logItem.logTime = time.format(new Date());

        logItem.diceName = diceName;

        logItem.diceRepresentation = DieConfiguration.arrayToString(diceConfigurations);

        Resources res = ctx.getResources();
        logItem.rollResults = result.generateResultsString(res.getString(R.string.diceMessageResult),
                res.getString(R.string.addition), res.getString(R.string.subtraction));

        logItems.add(logItem);

        if (logItems.size() > MAX_SIZE) {
            logItems.remove(0);
        }
    }

    public LogItem get(int index) {
        return logItems.get(index);
    }

    public int size() {
        return logItems.size();
    }
}
