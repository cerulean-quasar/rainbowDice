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

import android.content.res.AssetManager;
import android.os.Handler;
import android.view.Surface;
import android.view.SurfaceHolder;

public class DiceWorker implements Runnable {
    private DiceDrawerReturnChannel m_notify;
    private SurfaceHolder m_surfaceHolder;
    private AssetManager m_assetManager;

    public DiceWorker(Handler inNotify, SurfaceHolder inSurfaceHolder, AssetManager inAssetManager) {
        m_notify = new DiceDrawerReturnChannel(inNotify);
        m_surfaceHolder = inSurfaceHolder;
        m_assetManager = inAssetManager;
    }

    public void run() {
        String error = startWorker(m_surfaceHolder.getSurface(), m_assetManager, m_notify);
        if (error == null || error.length() == 0) {
            // we are done with no error occurring.
            return;
        }
        m_notify.sendError(error);
    }

    private native String startWorker(Surface jsurface, AssetManager jmanager, DiceDrawerReturnChannel jnotify);
}
