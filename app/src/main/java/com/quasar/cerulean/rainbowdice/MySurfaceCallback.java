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

import android.view.SurfaceHolder;

public class MySurfaceCallback implements SurfaceHolder.Callback {
    private MainActivity app;

    public MySurfaceCallback(MainActivity inApp) {
        app = inApp;
    }

    public void surfaceChanged(SurfaceHolder holder,
                int format,
                int width,
                int height)
    {
        // If the app is drawing, let the drawer detect the change and fix it.  Otherwise we need
        // to draw once here to avoid the dice looking squashed after the dice are done rolling
        // and the result is delivered (the drawer is stopped if this is the case).
        if (!app.isDrawing() && app.loadFromLog()) {
            app.drawStoppedDice();
        }
    }


    public void surfaceCreated(SurfaceHolder holder) {
        app.setSurfaceReady(true);
        if (app.loadFromLog()) {
            app.drawStoppedDice();
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        app.joinDrawer();
        app.setSurfaceReady(false);
    }
}
