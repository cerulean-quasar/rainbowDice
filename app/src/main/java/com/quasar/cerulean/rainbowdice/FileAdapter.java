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

public class FileAdapter extends BaseAdapter {
    private ArrayList<String> filelist;
    private Context ctx;
    public FileAdapter(Context inctx, String[] excludeFiles) {
        ctx = inctx;
        filelist = new ArrayList<>();
        File[] filearr = ctx.getFilesDir().listFiles();
        for (File file : filearr) {
            boolean found = false;
            for (String filename : excludeFiles) {
                if (file.getName().equals(filename)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                filelist.add(file.getName());
            }
        }
    }

    public int getPosition(String filename) {
        int i = 0;
        for (String file : filelist) {
            if (filename.equals(file)) {
                return i;
            }
            i++;
        }
        return -1;
    }

    @Override
    public int getCount() {
        return filelist.size();
    }

    @Override
    public String getItem(int position) {
        return filelist.get(position);
    }

    @Override
    public long getItemId(int position) {
        return filelist.get(position).hashCode();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup container) {
        if (convertView == null) {
            LayoutInflater inflater = (LayoutInflater) ctx.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(R.layout.list_item_dice_configuration, container, false);
        }

        TextView text = convertView.findViewById(R.id.list_item);
        text.setText(getItem(position));

        return text;
    }
}
