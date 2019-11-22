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

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;

public class OperationAdapter extends BaseAdapter {
    public static final String plus = "+";
    public static final String minus = "-";
    public static final int plusPosition = 0;
    public static final int minusPosition = 1;
    private Context ctx;
    public OperationAdapter(Context inctx){
        ctx = inctx;
    }

    @Override
    public int getCount() {
        return 2;
    }

    @Override
    public String getItem(int position) {
        if (position == plusPosition) {
            return plus;
        } else {
            return minus;
        }
    }

    @Override
    public long getItemId(int position) {
        if (position == plusPosition) {
            return plus.hashCode();
        } else {
            return minus.hashCode();
        }
    }

    @Override
    public View getView(int position, View convertView, ViewGroup container) {
        if (convertView == null) {
            LayoutInflater inflater = (LayoutInflater) ctx.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(R.layout.list_item_operation, container, false);
        }

        ImageView img = convertView.findViewById(R.id.list_item_operation);
        if (position == plusPosition) {
            img.setImageDrawable(ctx.getDrawable(R.drawable.plus));
        } else {
            img.setImageDrawable(ctx.getDrawable(R.drawable.minus));
        }

        return img;
    }
}
