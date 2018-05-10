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

import java.util.ArrayList;

public class DiceResult {
    private class DieResult {
        // What the die rolled
        public int result;

        // Does it need to be rerolled?
        public boolean needsReRoll;

        // Does this die get added or subtracted?
        public boolean isAddOperation;

        DieResult (int inResult, boolean inNeedsReRoll, boolean inIsAddOperation) {
            result = inResult;
            needsReRoll = inNeedsReRoll;
            isAddOperation = inIsAddOperation;
        }
    }

    private ArrayList<ArrayList<DieResult>> diceResults;

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
                    ArrayList<DieResult> dieResults = new ArrayList<>();
                    ArrayList<DieResult> dieResults2 = new ArrayList<>();
                    int value = Integer.valueOf(values[i++]);
                    int value2 = Integer.valueOf(values[i]);
                    if (value + value2 == reRollOn) {
                        needsReRoll = true;
                    }
                    DieResult dieResult = new DieResult(value, needsReRoll, isAddOperation);
                    dieResults.add(dieResult);
                    diceResults.add(dieResults);
                    dieResult = new DieResult(value2, needsReRoll, isAddOperation);
                    dieResults2.add(dieResult);
                    diceResults.add(dieResults2);
                } else {
                    ArrayList<DieResult> dieResults = new ArrayList<>();
                    int value = Integer.valueOf(values[i]);
                    if (value == reRollOn) {
                        needsReRoll = true;
                    }
                    DieResult dieResult = new DieResult(value, needsReRoll, isAddOperation);
                    dieResults.add(dieResult);
                    diceResults.add(dieResults);
                }
                i++;
            }
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

                DieResult dieResult = new DieResult(value, needsReRoll, prev.isAddOperation);
                prevResult.add(dieResult);
            }
            i++;
        }
    }

    public int[] reRollRequiredOnIndices() {
        ArrayList<Integer> indicesNeedReRoll = new ArrayList<>();
        int i=0;
        for (ArrayList<DieResult> dieResults : diceResults) {
            if (dieResults.get(dieResults.size()-1).needsReRoll) {
                indicesNeedReRoll.add(i);
            }
            i++;
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
}
