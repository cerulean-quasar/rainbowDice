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

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class Draw implements Runnable {
    private static String NATIVE = "Rainbow Dice Native";
    private Handler notify;
    public Draw(Handler inNotify) { notify = inNotify; }
    public void run() {
        String result = draw();
        if (result == null || result.length() == 0) {
            // no result and we exited, something must be wrong.  Just return, we are done.
            return;
        }
        // tell the main thread, a result has occurred.
        Bundle bundle = new Bundle();
        bundle.putString("results", result);
        Message msg = Message.obtain();
        msg.setData(bundle);
        notify.sendMessage(msg);
    }

    public native String draw();
}
