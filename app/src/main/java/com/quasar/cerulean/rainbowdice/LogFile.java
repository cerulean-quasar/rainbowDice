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
    public static final String version_name = "version";
    private static final int MAX_SIZE = 10;

    private ArrayList<LogItem> logItems;
    private Context ctx;

    public abstract class LogItem {
        protected static final String jsonTime = "time";
        protected static final String jsonDiceName = "name";
        protected String logTime;
        protected String diceName;

        public String getLogTime() {
            return logTime;
        }

        public String getDiceName() {
            return diceName;
        }

        public abstract JSONObject toJSON() throws JSONException;
        public abstract int getVersion();
        public abstract String getDiceRepresentation();
        public abstract String getRollResultsString();
        public abstract DiceResult getRollResults();
        public abstract DieConfiguration[] getDiceConfiguration();
    }

    private class LogItemV0 extends LogItem {
        private static final String jsonDiceRepresentation = "representation";
        private static final String jsonRollResultString = "result";
        private String diceRepresentation;
        private String rollResults;

        public LogItemV0(LogItemV0 other) {
            logTime = other.logTime;
            diceName = other.diceName;
            diceRepresentation = other.diceRepresentation;
            rollResults = other.rollResults;
        }

        public LogItemV0(JSONObject obj) throws JSONException {
            logTime = obj.getString(jsonTime);
            diceName = obj.getString(jsonDiceName);
            diceRepresentation = obj.getString(jsonDiceRepresentation);
            rollResults = obj.getString(jsonRollResultString);
        }

        public JSONObject toJSON() throws JSONException {
            JSONObject obj = new JSONObject();
            obj.put(version_name, 0);
            obj.put(jsonTime, logTime);
            obj.put(jsonDiceName, diceName);
            obj.put(jsonDiceRepresentation, diceRepresentation);
            obj.put(jsonRollResultString, rollResults);
            return obj;
        }

        @Override
        public String getDiceRepresentation() {
            return diceRepresentation;
        }

        @Override
        public String getRollResultsString() {
            return rollResults;
        }

        @Override
        public DieConfiguration[] getDiceConfiguration() {
            return null;
        }

        @Override
        public DiceResult getRollResults() {
            return null;
        }

        @Override
        public int getVersion() { return 0; }
    }

    private class LogItemV1 extends LogItem {
        private static final String diceList_name = "diceConfigurationList";
        private static final String diceGroup_name = "diceGroup";
        private static final String diceResults_name = "diceResults";
        private static final String diceResult_name = "diceResult";
        private DiceGroup dice;
        private DiceResult result;

        public LogItemV1(LogItemV1 other) {
            logTime = other.logTime;
            diceName = other.diceName;
            dice = new DiceGroup(other.dice.dieConfigurations());
            result = new DiceResult(other.result);
        }

        public LogItemV1(String inLogTime, String inDiceName, DieConfiguration[] inDice, DiceResult inResult) {
            logTime = inLogTime;
            diceName = inDiceName;
            dice = new DiceGroup(inDice);
            result = inResult;
        }

        public LogItemV1(JSONObject obj) throws JSONException {
            logTime = obj.getString(jsonTime);
            diceName = obj.getString(jsonDiceName);
            if (obj.has(diceList_name)) {
                dice = DiceGroup.fromDiceConfigurationV0JsonArray(obj.getJSONArray(diceList_name));
            } else if (obj.has(diceGroup_name)) {
                dice = DiceGroup.fromJson(obj.getJSONObject(diceGroup_name));
            }
            if (obj.has(diceResults_name)) {
                JSONArray arr2 = obj.getJSONArray(diceResults_name);
                result = new DiceResult(arr2);
            } else {
                JSONObject obj2 = obj.getJSONObject(diceResult_name);
                result = new DiceResult(obj2);
            }
        }

        public JSONObject toJSON() throws JSONException {
            JSONObject obj = new JSONObject();
            obj.put(version_name, 1);
            obj.put(jsonTime, logTime);
            obj.put(jsonDiceName, diceName);
            JSONObject diceObj = dice.toJson();
            obj.put(diceGroup_name, diceObj);
            JSONObject resultObj = result.toJSON();
            obj.put(diceResult_name, resultObj);
            return obj;
        }

        @Override
        public String getDiceRepresentation() {
            return DieConfiguration.arrayToString(dice.dieConfigurations());
        }

        @Override
        public String getRollResultsString() {
            Resources res = ctx.getResources();
            return result.generateResultsString(dice.dieConfigurations(),
                    diceName, ctx);
        }

        @Override
        public DieConfiguration[] getDiceConfiguration() {
            return dice.dieConfigurations();
        }

        @Override
        public DiceResult getRollResults() {
            return result;
        }

        @Override
        public int getVersion() { return 1; }
    }

    public LogFile(Context inCtx) {
        ctx = inCtx;
        logItems = new ArrayList<>();
        loadFile();
    }

    public LogFile(Context inCtx, JSONArray arr) throws JSONException {
        ctx = inCtx;
        logItems = new ArrayList<>();
        loadJSON(arr);
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
            loadJSON(arr);
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + diceLogFilename + " message: " + e.getMessage());
            return;
        }
    }

    private void loadJSON(JSONArray arr) throws JSONException {
        for (int i=0; i < arr.length(); i++) {
            JSONObject obj = arr.getJSONObject(i);
            int version = 0;
            if (obj.has(version_name)) {
                version = obj.getInt(version_name);
            }
            LogItem item;
            if (version == 0) {
                item = new LogItemV0(obj);
            } else if (version == 1) {
                item = new LogItemV1(obj);
            } else {
                // not a valid entry...
                continue;
            }
            logItems.add(item);
        }
    }

    public JSONArray toJSON() throws JSONException {
        JSONArray arr = new JSONArray();
        for (LogItem logItem: logItems) {
            JSONObject obj = logItem.toJSON();
            arr.put(obj);
        }

        return arr;
    }

    public void writeFile() {
        String json;
        try {
            JSONArray arr = toJSON();
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
        LogItem logItem = new LogItemV1(time.format(new Date()), diceName, diceConfigurations, result);
        logItems.add(logItem);

        if (logItems.size() > MAX_SIZE) {
            logItems.remove(0);
        }
    }

    public LogItem get(int index) {
        return logItems.get(index);
    }

    public LogItem getCopy(int index) {
        LogItem item = logItems.get(index);
        int version = item.getVersion();
        if (item.getVersion() == 1) {
            return new LogItemV1((LogItemV1)item);
        } else if (version == 0) {
            return new LogItemV0((LogItemV0)item);
        } else {
            return null;
        }
    }

    public int size() {
        return logItems.size();
    }
}
