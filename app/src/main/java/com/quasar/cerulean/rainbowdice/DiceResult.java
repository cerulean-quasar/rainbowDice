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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;

public class DiceResult {
    public class DieResult {
        private static final String result_name = "die_result";
        private static final String result_symbol_name = "die_symbol_result";
        private static final String result_value_name = "die_value_result";
        private static final String diceIndex_name = "dice_index_result";
        private static final String isAddOperation_name = "is_add_operation";
        private static final String isConstant_name = "is_constant";

        private Integer m_diceIndex;
        private Integer m_value;

        // What the die rolled
        private String m_symbol;

        // Does this die get added or subtracted?
        private boolean m_isAddOperation;

        // is the result from a constant (D1 dice)?
        private boolean m_isConstant;

        DieResult (String inSymbol, Integer inValue, Integer inDiceIndex, boolean inIsAddOperation, boolean inIsConstant) {
            m_symbol = inSymbol;
            m_value = inValue;
            m_diceIndex = inDiceIndex;
            m_isAddOperation = inIsAddOperation;
            m_isConstant = inIsConstant;
        }

        public JSONObject toJSON() throws JSONException {
            JSONObject obj = new JSONObject();
            if (m_value != null) {
                obj.put(result_value_name, m_value);
            }
            if (!m_isConstant && m_diceIndex != null) {
                obj.put(diceIndex_name, m_diceIndex);
            }
            obj.put(result_symbol_name, m_symbol);
            obj.put(isAddOperation_name, m_isAddOperation);
            obj.put(isConstant_name, m_isConstant);

            return obj;
        }

        public DieResult(JSONObject obj) throws JSONException {
            m_isAddOperation = obj.getBoolean(isAddOperation_name);
            m_isConstant = obj.getBoolean(isConstant_name);
            if (obj.has(result_name)) {
                // an old result.  the symbol and value are the same and both are numbers only
                m_value = obj.getInt(result_name);
                m_symbol = String.format(Locale.getDefault(), "%d", m_value);
                m_diceIndex = null;
            } else {
                m_symbol = obj.getString(result_symbol_name);
                if (obj.has(result_value_name)) {
                    m_value = obj.getInt(result_value_name);
                }
                m_diceIndex = null;
                if (!m_isConstant && obj.has(diceIndex_name)) {
                    m_diceIndex = obj.getInt(diceIndex_name);
                }
            }
        }

        Integer value() { return m_value; }
        String symbol() { return m_symbol; }
        Integer index() { return m_diceIndex; }
    }

    private ArrayList<ArrayList<DieResult>> diceResults;

    public DiceResult(String result, DieConfiguration[] diceConfigurations) {
        diceResults = new ArrayList<>();
        String[] values = result.split("\n");
        int i=0;
        for (DieConfiguration die: diceConfigurations) {
            int numberOfDice = die.getNumberOfDice();
            boolean isAddOperation = die.isAddOperation();
            for (int j = 0; j < numberOfDice; j++) {
                if (die.getNumberOfSides() == 1) {
                    DieSideConfiguration side = die.getSide(0);
                    DieResult dieResult = new DieResult(side.symbol(), side.value(), null,
                            isAddOperation, true);
                    ArrayList<DieResult> dieResults = new ArrayList<>();
                    dieResults.add(dieResult);
                    diceResults.add(dieResults);
                } else {
                    ArrayList<DieResult> dieResults = getResultForString(die, values[i], isAddOperation);
                    diceResults.add(dieResults);
                    i++;
                }
            }
        }
    }

    private ArrayList<DieResult> getResultForString(DieConfiguration die, String dieResultsString, boolean isAddOperation) {
        ArrayList<DieResult> dieResults = new ArrayList<>();
        String[] values = dieResultsString.split("\t");
        for (String value: values) {
            int index = Integer.valueOf(value);
            // Take %die.getNumberOfSides() because C++ returns the dice face index, not the index
            // into the dice sides array.  This is critical for some dice like the 3 sided die, where
            // the actual virtual die has six sides, but the configuration only has three sides.
            // C++ needs to return the face index so that it can be passed back into the function
            // to restore dice from a log entry.  If the face index is not passed but rather the
            // symbol index, then for the three sided die, the face could shift colors for
            // rainbow colored dice.
            DieSideConfiguration side = die.getSide(index%die.getNumberOfSides());
            DieResult dieResult = new DieResult(side.symbol(), side.value(), index, isAddOperation, false);
            dieResults.add(dieResult);
        }
        return dieResults;
    }

    public JSONArray toJSON() throws JSONException {
        JSONArray arrDiceResults = new JSONArray();
        for (ArrayList<DieResult> diceResult : diceResults) {
            JSONArray arrDiceResult = new JSONArray();
            for (DieResult dieResult : diceResult) {
                JSONObject obj = dieResult.toJSON();
                arrDiceResult.put(obj);
            }
            arrDiceResults.put(arrDiceResult);
        }

        return arrDiceResults;
    }

    public DiceResult(JSONArray arr) throws JSONException {
        diceResults = new ArrayList<>();
        for (int i = 0; i < arr.length(); i++) {
            ArrayList<DieResult> diceResult = new ArrayList<>();
            JSONArray arr2 = arr.getJSONArray(i);
            for (int j = 0; j < arr2.length(); j++) {
                JSONObject obj = arr2.getJSONObject(j);
                DieResult result = new DieResult(obj);
                diceResult.add(result);
            }
            diceResults.add(diceResult);
        }
    }

    public String generateResultsString(DieConfiguration[] configs, String resultFormat,
                                        String resultFormat2,
                                        String addition, String subtraction) {
        boolean isBegin = true;
        int i = 0;
        int j = 1;
        TreeMap<String, Integer> totalMap = new TreeMap<>();
        String number = "__cerulean_quasar_numeric_value__";
        StringBuilder resultString = new StringBuilder();
        for (ArrayList<DieResult> dieResults : diceResults) {
            for (DieResult dieResult : dieResults) {
                String symbol = dieResult.m_symbol;
                Integer value = dieResult.m_value;
                boolean isAddOperation = dieResult.m_isAddOperation;
                if (DieSideConfiguration.valueEqualsSymbol(symbol, value) && value < 0) {
                    value = -value;
                    symbol = String.format(Locale.getDefault(), "%d", value);
                    isAddOperation = !isAddOperation;
                }
                if (isBegin) {
                    isBegin = false;
                    if (!isAddOperation) {
                        resultString.append(subtraction);
                    }
                } else {
                    if (isAddOperation) {
                        resultString.append(addition);
                    } else {
                        resultString.append(subtraction);
                    }
                }

                resultString.append(DieSideConfiguration.toString(symbol, value));

                symbol = dieResult.m_symbol;
                value = dieResult.m_value;
                if (value == null) {
                    value = 1;
                } else {
                    symbol = number;
                }

                if (!configs[i].isAddOperation()) {
                    value = -value;
                }

                Integer nbrTimesOccurred = totalMap.remove(symbol);
                if (nbrTimesOccurred != null) {
                    nbrTimesOccurred += value;
                    totalMap.put(symbol, nbrTimesOccurred);
                } else {
                    totalMap.put(symbol, value);
                }
            }
            if (j < configs[i].getNumberOfDice()) {
                j++;
            } else {
                i++;
                j = 1;
            }
        }

        StringBuilder totalString = new StringBuilder();
        isBegin = true;
        for (Map.Entry<String, Integer> entry : totalMap.entrySet()) {
            Integer value = entry.getValue();
            boolean isAddOperation = true;
            if (value == 0) {
                continue;
            }
            if (value < 0) {
                value = -value;
                isAddOperation = false;
            }
            if (isBegin) {
                isBegin = false;
                if (!isAddOperation) {
                    totalString.append(subtraction);
                }
            } else {
                if (isAddOperation) {
                    totalString.append(addition);
                } else {
                    totalString.append(subtraction);
                }
            }
            if (entry.getKey().equals(number)) {
                totalString.append(value);
            } else {
                if (value == 1) {
                    totalString.append(entry.getKey());
                } else {
                    totalString.append(String.format(Locale.getDefault(), "%d*%s",
                            value, entry.getKey()));
                }
            }
        }

        String resultBeforeAdd = resultString.toString();
        String resultAfterAdd = totalString.toString();
        if (resultAfterAdd.isEmpty()) {
            resultAfterAdd = "0";
        }
        if (resultBeforeAdd.equals(resultAfterAdd)) {
            return String.format(Locale.getDefault(), resultFormat2, resultBeforeAdd);
        } else {
            return String.format(Locale.getDefault(), resultFormat, resultBeforeAdd, resultAfterAdd);
        }
    }

    ArrayList<ArrayList<DieResult>> getDiceResults() {
        return diceResults;
    }
}
