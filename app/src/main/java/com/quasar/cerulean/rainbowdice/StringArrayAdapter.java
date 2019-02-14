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

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import java.io.File;
import java.util.ArrayList;

public class StringArrayAdapter extends BaseAdapter {
    private String[] strings;
    private Context ctx;
    private int layout;
    private int textItem;

    public StringArrayAdapter(Context inctx, int arrayName, int inLayout, int inTextItem) {
        ctx = inctx;
        strings = ctx.getResources().getStringArray(arrayName);
        layout = inLayout;
        textItem = inTextItem;
    }

    public int getPosition(String selected) {
        int i = 0;
        for (String string : strings) {
            if (selected.equals(string)) {
                return i;
            }
            i++;
        }
        return -1;
    }

    @Override
    public int getCount() {
        return strings.length;
    }

    @Override
    public String getItem(int position) {
        return strings[position];
    }

    @Override
    public long getItemId(int position) {
        return strings[position].hashCode();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup container) {
        if (convertView == null) {
            LayoutInflater inflater = (LayoutInflater) ctx.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(layout, container, false);
        }

        TextView text = convertView.findViewById(textItem);
        text.setText(getItem(position));

        return text;
    }
}