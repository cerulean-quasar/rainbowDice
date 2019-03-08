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

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Locale;

public class DieConfiguration implements Parcelable {
    private static final String numberOfDiceAttributeName = "NumberOfDice";
    private static final String sidesAttributeName = "sides";
    private static final String colorAttributeName = "color";
    private static final String useRainbowAttributeName = "rainbow";
    private static final String isAddOperationAttributeName = "IsAddOperation";

    private int m_numberOfDice;

    // if color is null, then we use the rainbow coloration.
    private float[] m_color;
    private boolean m_rainbow;

    private DieSideConfiguration[] m_sides;

    // This operator applies to the current dice set.
    private boolean m_isAdditionOperation;

    public static DieConfiguration[] fromJsonArray(JSONArray arr,
                                                   int version) throws org.json.JSONException {
        ArrayList<DieConfiguration> dice = new ArrayList<>();
        for (int i = 0; i < arr.length(); i ++) {
            JSONObject obj = arr.getJSONObject(i);
            if (version == 0) {
                DieConfiguration[] diceArray = fromJsonV0(obj);
                for (DieConfiguration die : diceArray) {
                    dice.add(die);
                }
            } else {
                DieConfiguration die = fromJson(obj);
                dice.add(die);
            }
        }

        int length = dice.size();
        DieConfiguration[] diceArray = new DieConfiguration[length];
        for (int i = 0; i < length; i++) {
            diceArray[i] = dice.get(i);
        }

        return diceArray;
    }

    public static DieConfiguration[] fromJsonV0(JSONObject obj) throws org.json.JSONException {
        int inNumberOfDice = obj.getInt(numberOfDiceAttributeName);
        boolean inIsAdditionOperation = obj.getBoolean(isAddOperationAttributeName);
        int inNumberOfSides = obj.getInt("NumberOfSides");
        int inStartAt = obj.getInt("StartAt");
        int inIncrement = obj.getInt("Increment");
        int inReRollOn = obj.getInt("ReRollOn");

        if (inNumberOfSides == 100 && (inStartAt == 0 || inStartAt == 1) && inIncrement == 1) {
            return DieConfiguration.createPercentile(inNumberOfDice, inStartAt);
        } else {
            DieConfiguration[] dice = new DieConfiguration[1];
            DieSideConfiguration[] sides = new DieSideConfiguration[inNumberOfSides];
            for (int i = 0; i < inNumberOfSides; i++) {
                int value = inStartAt + i * inIncrement;
                String symbol = String.format(Locale.getDefault(), "%d", value);
                boolean reroll = inReRollOn == value;
                sides[i] = new DieSideConfiguration(symbol, value, reroll);
            }
            dice[0] = new DieConfiguration(inNumberOfDice, sides, inIsAdditionOperation);
            return dice;
        }
    }

    public static DieConfiguration fromJson(JSONObject obj) throws org.json.JSONException {
        int inNumberOfDice = obj.getInt(numberOfDiceAttributeName);
        boolean inIsAdditionOperation = obj.getBoolean(isAddOperationAttributeName);

        JSONArray jsonArray = obj.getJSONArray(sidesAttributeName);
        int length = jsonArray.length();
        DieSideConfiguration[] sides = new DieSideConfiguration[length];
        for (int i = 0; i < length; i++) {
            JSONObject sideObj = jsonArray.getJSONObject(i);
            sides[i] = DieSideConfiguration.fromJson(sideObj);
        }

        float[] color = null;
        boolean rainbow = true;
        if (obj.has(useRainbowAttributeName) && obj.getBoolean(useRainbowAttributeName)) {
            rainbow = true;
        } else if (obj.has(colorAttributeName)) {
            jsonArray = obj.getJSONArray(colorAttributeName);
            length = jsonArray.length();
            if (length == 4) {
                rainbow = false;
                color = new float[length];
                for (int i = 0; i < length; i++) {
                    color[i] = (float)jsonArray.getDouble(i);
                }
            }
        }

        if (rainbow) {
            return new DieConfiguration(inNumberOfDice, sides, inIsAdditionOperation);
        } else {
            return new DieConfiguration(inNumberOfDice, sides, inIsAdditionOperation, color);
        }
    }

    public static JSONArray toJSON(DieConfiguration[] dice) throws JSONException {
        JSONArray arr = new JSONArray();
        for (DieConfiguration die : dice) {
            JSONObject obj = die.toJSON();
            arr.put(obj);
        }
        return arr;
    }

    public JSONObject toJSON() throws org.json.JSONException {
        JSONObject obj = new JSONObject();
        obj.put(numberOfDiceAttributeName, m_numberOfDice);
        obj.put(isAddOperationAttributeName, m_isAdditionOperation);
        if (m_rainbow) {
            obj.put(useRainbowAttributeName, m_rainbow);
        } else {
            JSONArray colorArray = new JSONArray();
            for (int i = 0; i < m_color.length; i++) {
                colorArray.put(m_color[i]);
            }
            obj.put(colorAttributeName, colorArray);
        }
        JSONArray sidesJsonArray = DieSideConfiguration.toJsonArray(m_sides);
        obj.put(sidesAttributeName, sidesJsonArray);
        return obj;
    }

    public int describeContents() {
        // not sending a file descriptor so return 0.
        return 0;
    }

    public DieConfiguration(
            int inNbrDice,
            String[] inSymbols,
            Integer[] inValues,
            int[] inRerollIndices,
            float[] inColor,
            boolean inIsAddOperation)
    {
        m_color = inColor;
        m_numberOfDice = inNbrDice;
        m_isAdditionOperation = inIsAddOperation;
        m_rainbow = false;
        init(inSymbols, inValues, inRerollIndices);
    }

    public DieConfiguration(
            int inNbrDice,
            String[] inSymbols,
            Integer[] inValues,
            int[] inRerollIndices,
            boolean inIsAddOperation)
    {
        m_color = null;
        m_numberOfDice = inNbrDice;
        m_isAdditionOperation = inIsAddOperation;
        m_rainbow = true;
        init(inSymbols, inValues, inRerollIndices);
    }

    private void init(
            String[] inSymbols,
            Integer[] inValues,
            int[] inRerollIndices) {
        m_sides = new DieSideConfiguration[inSymbols.length];
        for (int i = 0; i < inSymbols.length; i++) {
            boolean reroll = false;
            for (int rerollIndex : inRerollIndices) {
                if (rerollIndex == i) {
                    reroll = true;
                    break;
                }
            }
            m_sides[i] = new DieSideConfiguration(inSymbols[i], inValues[i], reroll);
        }
    }

    // Constructor for using a specified color for the dice color
    public DieConfiguration(
            int inNbrDice,
            int inNbrSides,
            int first,
            int increment,
            int[] reRollOnNumbers,
            boolean inIsAdditionOperation,
            float[] inColor)
    {
        m_numberOfDice = inNbrDice;
        m_sides = new DieSideConfiguration[inNbrSides];
        for (int i = 0; i < inNbrSides; i++) {
            int value = first + i*increment;
            boolean reRollOnSide = false;
            if (reRollOnNumbers != null) {
                for (int reroll : reRollOnNumbers) {
                    if (reroll == value) {
                        reRollOnSide = true;
                        break;
                    }
                }
            }
            m_sides[i] = new DieSideConfiguration(String.format(
                    Locale.getDefault(), "%d", value), value, reRollOnSide);
        }

        m_color = inColor;
        if (inColor == null) {
            m_rainbow = true;
        } else {
            m_rainbow = false;
        }
        m_isAdditionOperation = inIsAdditionOperation;
    }

    // Constructor for using a specified color for the dice color
    public DieConfiguration(
            int inNbrDice,
            DieSideConfiguration[] inSides,
            boolean inIsAdditionOperation,
            float[] inColor)
    {
        m_numberOfDice = inNbrDice;
        m_sides = new DieSideConfiguration[inSides.length];
        for (int i = 0; i < inSides.length; i++) {
            m_sides[i] = new DieSideConfiguration(inSides[i]);
        }
        m_color = inColor;
        m_rainbow = false;
        m_isAdditionOperation = inIsAdditionOperation;
    }

    // Constructor for using the rainbow dice coloration
    public DieConfiguration(
            int inNbrDice,
            DieSideConfiguration[] inSides,
            boolean inIsAdditionOperation)
    {
        m_numberOfDice = inNbrDice;
        m_sides = new DieSideConfiguration[inSides.length];
        for (int i = 0; i < inSides.length; i++) {
            m_sides[i] = new DieSideConfiguration(inSides[i]);
        }
        m_color = null;
        m_rainbow = true;
        m_isAdditionOperation = inIsAdditionOperation;
    }

    public DieConfiguration(DieConfiguration other, int inNumberOfDice) {
        m_numberOfDice = inNumberOfDice;
        m_sides = new DieSideConfiguration[other.m_sides.length];
        for (int i = 0; i < other.m_sides.length; i++) {
            m_sides[i] = new DieSideConfiguration(other.m_sides[i]);
        }

        if (other.m_color == null) {
            m_color = null;
        } else {
            m_color = new float[other.m_color.length];
            System.arraycopy(other.m_color, 0, m_color, 0, other.m_color.length);
        }

        m_isAdditionOperation = other.m_isAdditionOperation;
    }

    public DieConfiguration(DieConfiguration other) {
        m_numberOfDice = other.m_numberOfDice;
        m_sides = new DieSideConfiguration[other.m_sides.length];
        for (int i = 0; i < other.m_sides.length; i++) {
            m_sides[i] = new DieSideConfiguration(other.m_sides[i]);
        }

        if (other.m_color == null) {
            m_color = null;
        } else {
            m_color = new float[other.m_color.length];
            System.arraycopy(other.m_color, 0, m_color, 0, other.m_color.length);
        }

        m_isAdditionOperation = other.m_isAdditionOperation;
    }

    public static DieConfiguration[] createPercentile() {
        return createPercentile(1, 0);
    }

    public static DieConfiguration[] createPercentile(int inNumberOfDice, int inStartAt) {
        DieConfiguration[] dice = new DieConfiguration[2];
        dice[0] = new DieConfiguration(inNumberOfDice, 10, 0, 10,
                null, true, null);
        dice[0].m_sides[0].setSymbol("00");
        dice[1] = new DieConfiguration(inNumberOfDice, 10, inStartAt, 1,
                null, true, null);
        return dice;
    }

    public static DieConfiguration[] createFudge() {
        DieConfiguration[] dice = new DieConfiguration[1];
        DieSideConfiguration[] sides = new DieSideConfiguration[3];
        sides[0] = new DieSideConfiguration("-", -1, false);
        sides[1] = new DieSideConfiguration("", 0, false);
        sides[2] = new DieSideConfiguration("+", 1, false);
        dice[0] = new DieConfiguration(4, sides, true);
        return dice;
    }

    public static final Parcelable.Creator<DieConfiguration> CREATOR = new Parcelable.Creator<DieConfiguration>() {
        public DieConfiguration createFromParcel(Parcel in) {
            return new DieConfiguration(in);
        }
        public DieConfiguration[] newArray(int size) {
            return new DieConfiguration[size];
        }
    };

    private DieConfiguration(Parcel in) {
        m_numberOfDice = in.readInt();
        m_isAdditionOperation = in.readInt() != 0;
        m_rainbow = in.readInt() != 0;
        if (!m_rainbow) {
            m_color = new float[4];
            for (int i = 0; i < m_color.length; i++) {
                m_color[i] = in.readFloat();
            }
        } else {
            m_color = null;
        }

        int numberOfSides = in.readInt();
        m_sides = new DieSideConfiguration[numberOfSides];
        for (int i = 0; i < numberOfSides; i++) {
            m_sides[i] = new DieSideConfiguration(in);
        }
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(m_numberOfDice);
        out.writeInt(m_isAdditionOperation ? 1 : 0);
        out.writeInt(m_rainbow ? 1 : 0);
        if (!m_rainbow) {
            for (int i = 0; i < m_color.length; i++) {
                out.writeFloat(m_color[i]);
            }
        }
        out.writeInt(m_sides.length);
        for (DieSideConfiguration side : m_sides) {
            side.writeToParcel(out, flags);
        }
    }

    public boolean isAddOperation() {
        return m_isAdditionOperation;
    }

    public int getNumberOfDice() {
        return m_numberOfDice;
    }

    public DieSideConfiguration getSide(int index) {
        return m_sides[index];
    }

    public DieSideConfiguration[] getSides() {
        DieSideConfiguration[] sides = new DieSideConfiguration[m_sides.length];
        for (int i = 0; i < m_sides.length; i++) {
            sides[i] = new DieSideConfiguration(m_sides[i]);
        }

        return sides;
    }

    public int getNbrIndicesReRollOn() {
        int count = 0;
        for (DieSideConfiguration side : m_sides) {
            if (side.reRollOn()) {
                count++;
            }
        }

        return count;
    }

    public void getReRollOn(int[] rerollIndices) {
        ArrayList<Integer> alRerollIndices = new ArrayList<>();
        for (int i = 0; i < m_sides.length; i++) {
            if (m_sides[i].reRollOn()) {
                alRerollIndices.add(i);
            }
        }

        int length = alRerollIndices.size();
        for (int i = 0; i < length; i++) {
            rerollIndices[i] = alRerollIndices.get(i);
        }
    }

    public int getNumberOfSides() {
        return m_sides.length;
    }

    public float[] getColor() {
        if (m_color == null) {
            return null;
        }

        float[] retColor = new float[m_color.length];
        System.arraycopy(m_color, 0, retColor, 0, m_color.length);

        return retColor;
    }

    public boolean isRainbow() {
        return m_rainbow;
    }

    public boolean getColor(float[] retColor) {
        if (m_color == null) {
            return false;
        }

        System.arraycopy(m_color, 0, retColor, 0, m_color.length);

        return true;
    }

    public void setNumberOfDice(int in) {
        m_numberOfDice = in;
    }

    public void setSides(DieSideConfiguration[] sides) {
        m_sides = new DieSideConfiguration[sides.length];
        for (int i = 0; i < sides.length; i++) {
            m_sides[i] = new DieSideConfiguration(sides[i]);
        }
    }

    @NonNull
    public String toString() {
        if (m_sides.length == 1) {
            return m_sides[0].toString();
        } else {
            StringBuilder rerollString = new StringBuilder();
            for (DieSideConfiguration side : m_sides) {
                if (side.reRollOn()) {
                    rerollString.append(side.symbol());
                    rerollString.append(',');
                }
            }

            int length = rerollString.length();
            if (length > 0) {
                rerollString.deleteCharAt(length - 1);
            }

            if (rerollString.length() > 0) {
                return String.format(Locale.getDefault(), "%dD%d (%s, %s,...,%s reroll on %s)",
                        m_numberOfDice, m_sides.length, m_sides[0].toString(), m_sides[1].toString(),
                        m_sides[m_sides.length-1].toString(), rerollString.toString());
            } else {
                return String.format(Locale.getDefault(), "%dD%d (%s, %s,...,%s)",
                        m_numberOfDice, m_sides.length, m_sides[0].toString(), m_sides[1].toString(),
                        m_sides[m_sides.length-1].toString());
            }
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

    public String[] getSymbols() {
        String[] symbols = new String[m_sides.length];
        for (int i = 0; i < m_sides.length; i++) {
            symbols[i] = m_sides[i].symbol();
        }

        return symbols;
    }

    public void getValues(Integer[] values) {
        if (values.length != m_sides.length) {
            // should not happen
            return;
        }

        for (int i = 0; i < m_sides.length; i++) {
            values[i] = m_sides[i].value();
        }
    }

    public String getSymbolsString(int i) {
        if (i >= m_sides.length) {
            return "";
        }

        return m_sides[i].symbol();
    }

    public void setNumberOfSides(int nbrSides) {
        DieSideConfiguration[] sides = new DieSideConfiguration[nbrSides];
        for (int i = 0; i < nbrSides && i < m_sides.length; i++) {
            sides[i] = m_sides[i];
        }

        if (nbrSides > m_sides.length) {
            Integer value = m_sides[m_sides.length - 1].value();
            if (value != null) {
                Integer increment = 1;
                if (m_sides.length > 1) {
                    Integer value0 = m_sides[0].value();
                    Integer value1 = m_sides[1].value();
                    if (value0 != null && value1 != null) {
                        increment = value1 - value0;
                    }
                }
                for (int i = m_sides.length; i < nbrSides; i++) {
                    value += increment;
                    sides[i] = new DieSideConfiguration(
                            String.format(Locale.getDefault(), "%d", value),
                            value, false);
                }
            } else {
                String symbol = m_sides[m_sides.length-1].symbol();
                for (int i = m_sides.length; i < nbrSides; i++) {
                    sides[i] = new DieSideConfiguration(symbol, null, false);
                }
            }
        } else {
            int count = 0;
            for (DieSideConfiguration side : sides) {
                if (side.reRollOn()) {
                    count++;
                }
            }
            if (count > sides.length/2) {
                int i = sides.length - 1;
                while (count > sides.length/2 && i >= 0) {
                    if (sides[i].reRollOn()) {
                        sides[i].setRerollOn(false);
                        count --;
                    }
                    i--;
                }
            }
        }

        m_sides = sides;
    }

    public void setIsAddOperation(boolean inIsAddOperation) {
        m_isAdditionOperation = inIsAddOperation;
    }

    public void setColor(float[] inColor) {
        int length = 4;
        if (inColor == null || inColor.length != length) {
            m_color = null;
            m_rainbow = true;
        } else {
            m_rainbow = false;
            m_color = new float[length];
            System.arraycopy(inColor, 0, m_color, 0, length);
        }
    }

    public int getIndexForSymbol(String symbol) {
        int i = 0;
        for (DieSideConfiguration side : m_sides) {
            if (symbol.equals(side.symbol())) {
                return i;
            }
            i++;
        }

        // should not happen!
        return 0;
    }
}
