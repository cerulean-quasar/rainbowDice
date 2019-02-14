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

import android.os.Parcel;
import android.os.Parcelable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Locale;

public class DieSideConfiguration implements Parcelable {
    private static final String symbolAttributeName = "symbol";
    private static final String valueAttributeName = "value";
    private static final String reRollOnAttributeName = "reRoll";

    private String m_symbol;

    // is null if we are only grouping
    private Integer m_value;

    private boolean m_reRollOn;

    public DieSideConfiguration(String inSymbol, Integer inValue, boolean inReRollOn) {
        m_symbol = inSymbol;
        m_value = inValue;
        m_reRollOn = inReRollOn;
    }

    public DieSideConfiguration(DieSideConfiguration other) {
        m_symbol = other.m_symbol;
        m_value = other.m_value;
        m_reRollOn = other.m_reRollOn;
    }

    public DieSideConfiguration(Parcel in) {
        m_symbol = in.readString();
        m_reRollOn = in.readInt() != 0;
        m_value = null;
        boolean hasValue = in.readInt() != 0;
        if (hasValue) {
            m_value = in.readInt();
        }
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeString(m_symbol);
        out.writeInt(m_reRollOn ? 1 : 0);
        if (m_value != null) {
            out.writeInt(1);
            out.writeInt(m_value);
        } else {
            out.writeInt(0);
        }
    }

    public int describeContents() {
        // not sending a file descriptor so return 0.
        return 0;
    }

    public static final Parcelable.Creator<DieSideConfiguration> CREATOR = new Parcelable.Creator<DieSideConfiguration>() {
        public DieSideConfiguration createFromParcel(Parcel in) {
            return new DieSideConfiguration(in);
        }
        public DieSideConfiguration[] newArray(int size) {
            return new DieSideConfiguration[size];
        }
    };

    public static DieSideConfiguration[] fromJsonArray(JSONArray arr) throws JSONException {
        int length = arr.length();
        DieSideConfiguration[] sides = new DieSideConfiguration[length];
        for (int i = 0; i < length; i++) {
            JSONObject obj = arr.getJSONObject(i);
            sides[i] = DieSideConfiguration.fromJson(obj);
        }

        return sides;
    }

    public static DieSideConfiguration fromJson(JSONObject obj) throws JSONException {
        String symbol = obj.getString(symbolAttributeName);
        Integer value = null;
        if (obj.has(valueAttributeName)) {
            value = obj.getInt(valueAttributeName);
        }
        boolean reRollOn = obj.getBoolean(reRollOnAttributeName);

        return new DieSideConfiguration(symbol, value, reRollOn);
    }

    public JSONObject toJson() throws JSONException {
        JSONObject obj = new JSONObject();
        obj.put(symbolAttributeName, m_symbol);
        if (m_value != null) {
            obj.put(valueAttributeName, m_value);
        }
        obj.put(reRollOnAttributeName, m_reRollOn);

        return obj;
    }

    public static JSONArray toJsonArray(DieSideConfiguration[] sides) throws JSONException {
        JSONArray arr = new JSONArray();
        for (DieSideConfiguration side : sides) {
            arr.put(side.toJson());
        }

        return arr;
    }

    public String symbol() { return m_symbol; }
    public Integer value() { return m_value; }
    public boolean reRollOn() { return m_reRollOn; }

    public void setValue(Integer value) {
        m_value = value;
    }

    public void setSymbol(String symbol) {
        m_symbol = symbol;
    }

    public void setRerollOn(boolean rerollOn) {
        m_reRollOn = rerollOn;
    }

    public boolean isSymbolNumeric() {
        return isNumeric(m_symbol);
    }

    public boolean valueEqualsSymbol() {
        return valueEqualsSymbol(m_symbol, m_value);
    }

    public static boolean valueEqualsSymbol(String symbol, Integer value) {
        if (value == null || !isNumeric(symbol)) {
            return false;
        }

        return value.equals(Integer.valueOf(symbol));
    }

    public static boolean isNumeric(String symbol) {
        if (symbol == null) {
            return false;
        }
        if (symbol.length() > 9) {
            return false;
        }
        return symbol.matches("-?\\d+");
    }

    public String toShortString() {
        String symbol = m_symbol;
        if (symbol.length() > 4 && !valueEqualsSymbol()) {
            symbol = String.format(Locale.getDefault(), "%s.",
                    symbol.substring(0, 3));
        }
        return toString(symbol, m_value);
    }

    public static String toString(String symbol, Integer value) {
        if (symbol == null) {
            symbol = "";
        }

        if (value == null) {
            return symbol;
        }

        // If symbol is the same number as value then no need to put value
        // in the toString result.
        if (valueEqualsSymbol(symbol, value)) {
            return symbol;
        }

        // otherwise, m_symbol is different from m_value and we want to display m_value as well.
        return String.format(Locale.getDefault(), "(%s)%d", symbol, value);
    }

    public String toString() {
        return toString(m_symbol, m_value);
    }

    public boolean equals(DieSideConfiguration other) {
        if (m_symbol.equals(other.symbol())) {
            if (m_reRollOn != other.m_reRollOn) {
                return false;
            }
            if (m_value == null && other.value() == null) {
                return true;
            } else if (m_value != null && other.value() != null && m_value.equals(other.value())) {
                return true;
            }
        }

        return false;
    }
}
