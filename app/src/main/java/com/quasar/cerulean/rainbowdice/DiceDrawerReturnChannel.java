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

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import java.util.ArrayList;

public class DiceDrawerReturnChannel {
    public static final String errorMsg = "drawerError";

    public static final String resultsMsg = "diceResults";
    public static final String diceConfigMsg = "diceConfig";
    public static final String fileNameMsg = "diceFileName";
    public static final String isModifiedRollMsg = "isModifiedRoll";

    public static final String deviceName = "deviceName";
    public static final String apiVersion = "apiVersion";
    public static final String apiName = "apiName";
    public static final String isVulkan = "isVulkan";

    private DiceDrawerMessage m_msgBeingBuilt;
    private ArrayList<DieConfiguration> m_dice;
    private Handler m_notify;
    private String m_diceName;
    private boolean m_isModifiedRoll;

    public DiceDrawerReturnChannel(Handler inNotify) {
        m_notify = inNotify;
        m_msgBeingBuilt = null;
        m_dice = null;
        m_diceName = null;
        m_isModifiedRoll = false;
    }

    public void addResult(int[] indices) {
        if (m_msgBeingBuilt == null) {
            m_msgBeingBuilt = new DiceDrawerMessage();
        }

        m_msgBeingBuilt.addResult(indices);
    }

    public void addResult(int index) {
        if (m_msgBeingBuilt == null) {
            m_msgBeingBuilt = new DiceDrawerMessage();
        }

        m_msgBeingBuilt.addResult(index);
    }

    public void addDice(int nbrDice, String[] symbols, Integer[] values, int[] rerollIndices,
                        float[] color, boolean isAddOperation) {
        if (m_dice == null) {
            m_dice = new ArrayList<>();
        }

        if (color.length == 4) {
            m_dice.add(new DieConfiguration(nbrDice, symbols, values, rerollIndices, color, isAddOperation));
        } else {
            m_dice.add(new DieConfiguration(nbrDice, symbols, values, rerollIndices, isAddOperation));
        }
    }

    public void sendResults(String diceName, boolean inIsModifiedRoll) {
        m_diceName = diceName;
        m_isModifiedRoll = inIsModifiedRoll;
        sendResults();
    }

    public void sendResults() {
        if (m_msgBeingBuilt == null || m_dice == null) {
            return;
        }

        // tell the main thread, a result has occurred.
        Bundle bundle = new Bundle();
        bundle.putParcelable(resultsMsg, m_msgBeingBuilt);
        DieConfiguration[] dice = new DieConfiguration[m_dice.size()];
        int i = 0;
        for (DieConfiguration die : m_dice) {
            dice[i++] = die;
        }
        bundle.putParcelableArray(diceConfigMsg, dice);
        if (m_diceName != null) {
            bundle.putString(fileNameMsg, m_diceName);
        }
        bundle.putBoolean(isModifiedRollMsg, m_isModifiedRoll);
        Message msg = Message.obtain();
        msg.setData(bundle);
        m_notify.sendMessage(msg);

        m_msgBeingBuilt = null;
        m_dice = null;
        m_diceName = null;
    }

    public void sendError(String error) {
        // tell the main thread, a result has occurred.
        Bundle bundle = new Bundle();
        bundle.putString(errorMsg, error);
        Message msg = Message.obtain();
        msg.setData(bundle);
        m_notify.sendMessage(msg);

        m_msgBeingBuilt = null;
    }

    public void sendGraphicsDescription(boolean isVulkanAPI, String graphicsName, String version,
                                        String graphicsDeviceName) {
        // tell the main thread, a result has occurred.
        Bundle bundle = new Bundle();
        bundle.putBoolean(isVulkan, isVulkanAPI);
        bundle.putString(apiName, graphicsName);
        if (!version.isEmpty()) {
            bundle.putString(apiVersion, version);
        }
        if (!graphicsDeviceName.isEmpty()) {
            bundle.putString(deviceName, graphicsDeviceName);
        }
        Message msg = Message.obtain();
        msg.setData(bundle);
        m_notify.sendMessage(msg);
    }
}
