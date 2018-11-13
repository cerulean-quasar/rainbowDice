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

public class DiceResult {
    public class DieResult {
        private static final String result_name = "die_result";
        private static final String needsReRoll_name = "die_needs_reroll";
        private static final String isAddOperation_name = "is_add_operation";
        private static final String isConstant_name = "is_constant";

        // What the die rolled
        public int result;

        // Does it need to be rerolled?
        public boolean needsReRoll;

        // Does this die get added or subtracted?
        public boolean isAddOperation;

        // is the result from a constant (D1 dice)?
        public boolean isConstant;

        DieResult (int inResult, boolean inNeedsReRoll, boolean inIsAddOperation, boolean inIsConstant) {
            result = inResult;
            needsReRoll = inNeedsReRoll;
            isAddOperation = inIsAddOperation;
            isConstant = inIsConstant;
        }

        public JSONObject toJSON() throws JSONException {
            JSONObject obj = new JSONObject();
            obj.put(result_name, result);
            obj.put(needsReRoll_name, needsReRoll);
            obj.put(isAddOperation_name, isAddOperation);
            obj.put(isConstant_name, isConstant);

            return obj;
        }

        public DieResult(JSONObject obj) throws JSONException {
            result = obj.getInt(result_name);
            needsReRoll = obj.getBoolean(needsReRoll_name);
            isAddOperation = obj.getBoolean(isAddOperation_name);
            isConstant = obj.getBoolean(isConstant_name);
        }
    }

    private ArrayList<ArrayList<DieResult>> diceResults;

    private DiceResult() {
        diceResults = null;
    }

    public DiceResult(String result, DieConfiguration[] diceConfigurations) {
        diceResults = new ArrayList<>();
        String[] values = result.split("\n");
        int i=0;
        for (DieConfiguration die: diceConfigurations) {
            int numberOfDice = die.getNumberOfDice();
            boolean isAddOperation = die.isAddOperation();
            int reRollOn = die.getReRollOn();
            for (int j = 0; j < numberOfDice; j++) {
                boolean needsReRoll = false;
                if (die.isRepresentableByTwoTenSided()) {
                    ArrayList<DieResult> dieResults = getResultForString(values[i++], isAddOperation);
                    ArrayList<DieResult> dieResults2 = getResultForString(values[i], isAddOperation);
                    diceResults.add(dieResults);
                    diceResults.add(dieResults2);
                    i++;
                } else if (die.getNumberOfSides() == 1) {
                    DieResult dieResult = new DieResult(die.getStartAt(), false, isAddOperation, true);
                    ArrayList<DieResult> dieResults = new ArrayList<>();
                    dieResults.add(dieResult);
                    diceResults.add(dieResults);
                } else {
                    ArrayList<DieResult> dieResults = getResultForString(values[i], isAddOperation);
                    diceResults.add(dieResults);
                    i++;
                }
            }
        }
    }

    private ArrayList<DieResult> getResultForString(String dieResultsString, boolean isAddOperation) {
        ArrayList<DieResult> dieResults = new ArrayList<>();
        String[] values = dieResultsString.split("\t");
        for (String value: values) {
            int valueInt = Integer.valueOf(value);
            DieResult dieResult = new DieResult(valueInt, false, isAddOperation, false);
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

    public void updateWithReRolls(String result) {
        String[] values = result.split("\n");
        int i = 0;
        for (ArrayList<DieResult> prevResult : diceResults) {
            DieResult prev = prevResult.get(prevResult.size()-1);
            if (prev.needsReRoll) {
                boolean needsReRoll = false;
                int value = Integer.valueOf(values[i]);
                if (prev.result == value) {
                    needsReRoll = true;
                }

                DieResult dieResult = new DieResult(value, needsReRoll, prev.isAddOperation, false);
                prevResult.add(dieResult);
            }
            if (!prev.isConstant) {
                i++;
            }
        }
    }

    public int[] reRollRequiredOnIndices() {
        ArrayList<Integer> indicesNeedReRoll = new ArrayList<>();
        int i=0;
        for (ArrayList<DieResult> dieResults : diceResults) {
            if (dieResults.get(dieResults.size()-1).needsReRoll) {
                indicesNeedReRoll.add(i);
            }
            if (!dieResults.get(dieResults.size()-1).isConstant) {
                i++;
            }
        }

        if (indicesNeedReRoll.isEmpty()) {
            return null;
        }

        int[] indices = new int[indicesNeedReRoll.size()];
        i = 0;
        for (Integer index : indicesNeedReRoll) {
            indices[i++] = index;
        }

        return indices;
    }

    public String generateResultsString(String resultFormat, String addition, String subtraction) {
        boolean isBegin = true;
        int total = 0;
        StringBuilder resultString = new StringBuilder();
        for (ArrayList<DieResult> dieResults : diceResults) {
            for (DieResult dieResult : dieResults) {
                int value = dieResult.result;
                if (isBegin) {
                    isBegin = false;
                    if (dieResult.isAddOperation) {
                        total += value;
                    } else {
                        total -= value;
                        value = -1*value;
                    }
                } else {
                    if (dieResult.isAddOperation) {
                        total += value;
                        resultString.append(addition);
                    } else {
                        total-= value;
                        resultString.append(subtraction);
                    }
                }
                resultString.append(value);
            }
        }
        return String.format(resultFormat, resultString.toString(), total);
    }

    ArrayList<ArrayList<DieResult>> getDiceResults() {
        return diceResults;
    }

    String[] getResultSymbolList() {
        int len = 0;
        for (ArrayList<DieResult> dieResults : diceResults) {
            len += dieResults.size();
        }

        String[] symbols = new String[len];

        int i = 0;
        for (ArrayList<DieResult> dieResults : diceResults) {
            for (DieResult dieResult : dieResults) {
                symbols[i++] = Integer.toString(dieResult.result, 10);
            }
        }

        return symbols;
    }
}
