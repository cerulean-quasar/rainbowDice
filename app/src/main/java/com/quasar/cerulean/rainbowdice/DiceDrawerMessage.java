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

import java.util.LinkedList;

public class DiceDrawerMessage implements Parcelable {
    LinkedList<LinkedList<Integer>> m_results;

    public DiceDrawerMessage() {
        m_results = new LinkedList<>();
    }

    public DiceDrawerMessage(Parcel in) {
        m_results = new LinkedList<>();
        int nbrResults = in.readInt();
        for (int i = 0; i < nbrResults; i++) {
            int nbrIndices = in.readInt();
            LinkedList<Integer> result = new LinkedList<>();
            for (int j = 0; j < nbrIndices; j++) {
                result.add(in.readInt());
            }
            m_results.add(result);
        }
    }

    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(m_results.size());
        for (LinkedList<Integer> result : m_results) {
            out.writeInt(result.size());
            for (Integer index : result) {
                out.writeInt(index);
            }
        }
    }

    public int describeContents() {
        // not sending a file descriptor so return 0.
        return 0;
    }

    public static final Parcelable.Creator<DiceDrawerMessage> CREATOR = new Parcelable.Creator<DiceDrawerMessage>() {
        public DiceDrawerMessage createFromParcel(Parcel in) {
            return new DiceDrawerMessage(in);
        }
        public DiceDrawerMessage[] newArray(int size) {
            return new DiceDrawerMessage[size];
        }
    };

    public void addResult(int[] indices) {
        LinkedList<Integer> result = new LinkedList<>();
        for (int index : indices) {
            result.add(index);
        }

        m_results.add(result);
    }

    public void addResult(int index) {
        LinkedList<Integer> result = new LinkedList<>();
        result.add(index);
        m_results.add(result);
    }

    public LinkedList<LinkedList<Integer>> results() {
        return m_results;
    }
}
