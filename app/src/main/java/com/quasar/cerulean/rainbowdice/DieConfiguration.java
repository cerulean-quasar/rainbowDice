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
import android.content.res.Resources;
import android.os.Parcel;
import android.os.Parcelable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Locale;

public class DieConfiguration implements Parcelable {
    private int numberOfDice;
    private int numberOfSides;
    private int startAt;
    private int increment;
    private int reRollOn;

    // This operator applies to the current dice set.
    private boolean isAdditionOperation;

    public static DieConfiguration[] loadFromFile(FileInputStream inputStream) {
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

        try {
            JSONArray jsonArray = new JSONArray(json.toString());
            int length = jsonArray.length();
            DieConfiguration[] configs = new DieConfiguration[length];
            for (int i = 0; i < length; i++) {
                JSONObject jsonDieConfig = jsonArray.getJSONObject(i);
                configs[i] = DieConfiguration.fromJson(jsonDieConfig);
            }
            return configs;
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + e.getMessage());
            return null;
        }
    }

    public static void saveToFile(FileOutputStream outputStream, DieConfiguration[] diceConfigs) {
        String json;
        try {
            JSONArray jsonArray = new JSONArray();
            for (DieConfiguration diceConfig : diceConfigs) {
                JSONObject obj = diceConfig.toJSON();
                jsonArray.put(obj);
            }
            json = jsonArray.toString();
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


    public static DieConfiguration fromJson(JSONObject obj) throws org.json.JSONException {
        int inNumberOfDice = obj.getInt("NumberOfDice");
        int inNumberOfSides = obj.getInt("NumberOfSides");
        int inStartAt = obj.getInt("StartAt");
        int inIncrement = obj.getInt("Increment");
        int inReRollOn = obj.getInt("ReRollOn");
        boolean inIsAdditionOperation = obj.getBoolean("IsAddOperation");
        return new DieConfiguration(inNumberOfDice, inNumberOfSides, inStartAt, inIncrement,
                inReRollOn, inIsAdditionOperation);
    }

    public JSONObject toJSON() throws org.json.JSONException {
        JSONObject obj = new JSONObject();
        obj.put("NumberOfDice", numberOfDice);
        obj.put("NumberOfSides", numberOfSides);
        obj.put("StartAt", startAt);
        obj.put("Increment", increment);
        obj.put("ReRollOn", reRollOn);
        obj.put("IsAddOperation", isAdditionOperation);
        return obj;
    }

    public int describeContents() {
        // not sending a file descriptor so return 0.
        return 0;
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(numberOfDice);
        out.writeInt(numberOfSides);
        out.writeInt(startAt);
        out.writeInt(increment);
        out.writeInt(reRollOn);
        out.writeInt(isAdditionOperation?1:0);
    }

    public static final Parcelable.Creator<DieConfiguration> CREATOR = new Parcelable.Creator<DieConfiguration>() {
        public DieConfiguration createFromParcel(Parcel in) {
            return new DieConfiguration(in);
        }
        public DieConfiguration[] newArray(int size) {
            return new DieConfiguration[size];
        }
    };

    public DieConfiguration(
            int inNbrDice,
            int inNbrSides,
            int inStartAt,
            int inIncrement,
            int inReRollOn,
            boolean inIsAdditionOperation)
    {
        numberOfDice = inNbrDice;
        numberOfSides = inNbrSides;
        startAt = inStartAt;
        increment = inIncrement;
        reRollOn = inReRollOn;
        isAdditionOperation = inIsAdditionOperation;
    }

    private DieConfiguration(Parcel in) {
        numberOfDice = in.readInt();
        numberOfSides = in.readInt();
        startAt = in.readInt();
        increment = in.readInt();
        reRollOn = in.readInt();
        isAdditionOperation = (in.readInt() == 1);
    }

    public Integer getNumberOfDice() {
        return numberOfDice;
    }

    public Integer getNumberOfSides() {
        return numberOfSides;
    }

    public Integer getStartAt() {
        return startAt;
    }

    public Integer getIncrement() {
        return increment;
    }

    public Integer getReRollOn() {
        return reRollOn;
    }

    public boolean isAddOperation() {
        return isAdditionOperation;
    }

    public void setNumberOfDice(int in) {
        numberOfDice = in;
    }

    public void setNumberOfSides(int in) {
        numberOfSides = in;
    }

    public void setStartAt(int in) {
        startAt = in;
    }

    public void setIncrement(int in) {
        increment = in;
    }

    public void setReRollOn(int in) {
        reRollOn = in;
    }

    public void setIsAddOperation(boolean in) {
        isAdditionOperation = in;
    }

    public boolean isRepresentableByTwoTenSided() { return numberOfSides == 100 && startAt <= 1 && startAt >= 0 && increment == 1;}

    public String toString() {
        if (reRollOn >= startAt) {
            return String.format(Locale.getDefault(), "%dD%d (%d, %d,...,%d reroll on %d)",
                    numberOfDice, numberOfSides, startAt, startAt + increment, startAt + (numberOfSides - 1) * increment, reRollOn);
        } else {
            return String.format(Locale.getDefault(), "%dD%d (%d, %d,...,%d)",
                    numberOfDice, numberOfSides, startAt, startAt + increment, startAt + (numberOfSides - 1) * increment);
        }
    }

    public static String arrayToString(DieConfiguration[] arr) {
        StringBuilder stringRepresentation = new StringBuilder();
        for (int i = 0; i < arr.length; i++) {
            if (i != 0) {
                if (arr[i].isAddOperation()) {
                    stringRepresentation.append(" + ");
                } else {
                    stringRepresentation.append(" - ");
                }
            }
            stringRepresentation.append(arr[i].toString());
        }

        return stringRepresentation.toString();
    }
}
