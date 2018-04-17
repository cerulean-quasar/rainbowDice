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
import android.content.Intent;
import android.os.Parcelable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Locale;

public class DiceConfigurationActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dice_configuration);

        Spinner spinner = findViewById(R.id.filename);
        FileAdapter spinAdapter = new FileAdapter();
        spinner.setAdapter(spinAdapter);

        Intent intent = getIntent();
        String filename = intent.getStringExtra(MainActivity.DICE_FILENAME);
        if (filename != null) {
            int position = spinAdapter.getPosition(filename);
            if (position >= 0) {
                // the file exists, load it.
                loadFromFile(filename);
                EditText text = findViewById(R.id.newFilename);
                text.setText(filename);
                spinner.setSelection(position);
            }
        } else {
            Parcelable[] dice = intent.getParcelableArrayExtra(MainActivity.DICE_CONFIGURATION);
            LinearLayout layout = findViewById(R.id.dice_layout);
            int index = 0;
            for (Parcelable dieParcel : dice) {
                DieConfiguration die = (DieConfiguration) dieParcel;
                addDiceConfigToLayout(die, layout, index, index==dice.length-1);
                index++;
            }
        }

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_configuration_menu, menu);
        return true;
    }

    public void onSave(View view) {
        DieConfiguration[] dice = getDiceConfigFromScreen();
        EditText text = findViewById(R.id.newFilename);
        String filename = text.getText().toString();
        if (!filename.isEmpty()) {
            String json;
            try {
                JSONArray jsonArray = new JSONArray();
                for (DieConfiguration die : dice) {
                    JSONObject obj = die.toJSON();
                    jsonArray.put(obj);
                }
                json = jsonArray.toString();
            } catch (JSONException e) {
                System.out.println("Exception in writing out JSON: " + e.getMessage());
                return;
            }

            try {
                FileOutputStream outputStream = openFileOutput(filename, Context.MODE_PRIVATE);
                outputStream.write(json.getBytes());
                outputStream.close();
            } catch (FileNotFoundException e) {
                System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
                return;
            } catch (IOException e) {
                System.out.println("Exception on writing to file: " + filename + " message: " + e.getMessage());
                return;
            }
        }

        Spinner spinner = findViewById(R.id.filename);
        spinner.setAdapter(new FileAdapter());
    }

    public void onDeleteFile(View view) {
        Spinner spinner = findViewById(R.id.filename);
        Object text = spinner.getAdapter().getItem(spinner.getSelectedItemPosition());
        String filename = text.toString();
        deleteFile(filename);
        spinner.setAdapter(new FileAdapter());
    }

    public void onLoad(View view) {
        Spinner spinner = findViewById(R.id.filename);
        Object text = spinner.getAdapter().getItem(spinner.getSelectedItemPosition());
        String filename = text.toString();
        if (filename.isEmpty()) {
            return;
        }
        loadFromFile(filename);
    }

    public void onDone(MenuItem item) {
        DieConfiguration[] dice = getDiceConfigFromScreen();
        Intent result = new Intent();
        result.putExtra(MainActivity.DICE_CONFIGURATION, dice);
        TextView filenameView = findViewById(R.id.newFilename);
        Spinner spinner = findViewById(R.id.filename);
        int pos = ((FileAdapter)(spinner.getAdapter())).getPosition(filenameView.getText().toString());
        if (pos >= 0) {
            result.putExtra(MainActivity.DICE_FILENAME, filenameView.getText().toString());
        }
        setResult(RESULT_OK, result);
        finish();
    }

    public void onDelete(View view) {
        LinearLayout layout = findViewById(R.id.dice_layout);
        int nbrDice = layout.getChildCount();
        if (nbrDice == 1) {
            // Don't delete the dice if there is only one row!
            return;
        }
        LinearLayout prev = (LinearLayout)layout.getChildAt(0);
        for (int i = 0; i < nbrDice; i++) {
            LinearLayout layoutDice = (LinearLayout)layout.getChildAt(i);
            View delete_button = layoutDice.findViewById(R.id.delete_button);
            if (view == delete_button) {
                // delete this row.
                layout.removeView(layoutDice);
                if (i == nbrDice-1) {
                    // we removed the last row.  Remove the operation spinner from the new last row
                    LinearLayout item = prev.findViewById(R.id.item_dice_configuration);
                    item.removeViewAt(item.getChildCount()-1);
                }
                return;
            }
            prev = layoutDice;
        }
        TextView filename = findViewById(R.id.newFilename);
        filename.setText("");
    }

    public void onAddRow(View view) {
        LinearLayout layout = findViewById(R.id.dice_layout);
        int nbrDice = layout.getChildCount();
        for (int i = 0; i < nbrDice; i++) {
            LinearLayout layoutDice = (LinearLayout)layout.getChildAt(i);
            View addRow = layoutDice.findViewById(R.id.add_row);
            if (view == addRow) {
                LayoutInflater layoutInflater = getLayoutInflater();
                LinearLayout layoutDiceAdded = (LinearLayout) layoutInflater.inflate(
                        R.layout.row_dice_configuration, layout, false);
                layout.addView(layoutDiceAdded, i+1);
                Spinner operatorSpinner = (Spinner)layoutInflater.inflate(
                        R.layout.operator_view, layoutDice, false);
                StringArrayAdapter adapter = new StringArrayAdapter(R.array.operationArray);
                operatorSpinner.setAdapter(adapter);
                if (i == nbrDice - 1) {
                    // we are on the last row, we need to add an operator spinner on the end row.
                    LinearLayout item = layoutDice.findViewById(R.id.item_dice_configuration);
                    item.addView(operatorSpinner);
                } else {
                    // we are not on the last row, the spinner needs to be added to the layout
                    // we just added.
                    LinearLayout item = layoutDiceAdded.findViewById(R.id.item_dice_configuration);
                    item.addView(operatorSpinner);
                }

                TextView filename = findViewById(R.id.newFilename);
                filename.setText("");
                return;
            }
        }
    }

    private void loadFromFile(String filename) {
        StringBuffer json = new StringBuffer();

        try {
            byte[] bytes = new byte[1024];
            int len;
            FileInputStream inputStream = openFileInput(filename);
            while ((len = inputStream.read(bytes)) >= 0) {
                if (len > 0) {
                    json.append(new String(bytes, 0, len, "UTF-8"));
                }
            }
            inputStream.close();
        } catch (FileNotFoundException e) {
            System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
            return;
        } catch (IOException e) {
            System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
            return;
        }

        LinearLayout layout = findViewById(R.id.dice_layout);
        layout.removeViews(0, layout.getChildCount());
        try {
            JSONArray jsonArray = new JSONArray(json.toString());
            int length = jsonArray.length();
            for (int i = 0; i < length; i++) {
                JSONObject jsonDieConfig = jsonArray.getJSONObject(i);
                DieConfiguration dieConfig = DieConfiguration.fromJson(jsonDieConfig);
                addDiceConfigToLayout(dieConfig, layout, i, i==length-1);
            }
        } catch (JSONException e) {
            System.out.println("Exception on reading JSON from file: " + filename + " message: " + e.getMessage());
            return;
        }

        TextView saveFile = findViewById(R.id.newFilename);
        saveFile.setText(filename);
    }

    private DieConfiguration[] getDiceConfigFromScreen() {
        LinearLayout layout = findViewById(R.id.dice_layout);
        int nbrDice = layout.getChildCount();
        DieConfiguration[] dice = new DieConfiguration[nbrDice];
        for (int i=0; i < nbrDice; i++) {
            LinearLayout layoutDice = (LinearLayout)layout.getChildAt(i);
            EditText text = layoutDice.findViewById(R.id.number_of_dice);
            int nbrDiceOfThisType = Integer.parseInt(text.getText().toString(), 10);
            text = layoutDice.findViewById(R.id.die_sides);
            int nbrSides = Integer.parseInt(text.getText().toString(), 10);
            text = layoutDice.findViewById(R.id.die_start);
            int startAt = Integer.parseInt(text.getText().toString(), 10);
            text = layoutDice.findViewById(R.id.die_increment);
            int increment = Integer.parseInt(text.getText().toString(), 10);
            text = layoutDice.findViewById(R.id.die_reroll);
            int reRollOn = startAt - 1;
            if (!text.getText().toString().isEmpty()) {
                reRollOn = Integer.parseInt(text.getText().toString(), 10);
            }
            boolean isAdd = false;
            Spinner operationSpinner = layoutDice.findViewById(R.id.operator_view);
            if (operationSpinner==null) {
                isAdd = true;
            } else {
                TextView operation = (TextView)operationSpinner.getSelectedView();
                if (operation.getText().toString().equals(getString(R.string.addition))) {
                    isAdd = true;
                }
            }
            dice[i] = new DieConfiguration(
                    nbrDiceOfThisType, nbrSides, startAt, increment, reRollOn, isAdd);
        }

        return dice;
    }

    private void addDiceConfigToLayout(DieConfiguration die, LinearLayout layout, int index, boolean isEnd) {
        LayoutInflater layoutInflater = getLayoutInflater();
        LinearLayout layoutDiceAdded = (LinearLayout) layoutInflater.inflate(
                R.layout.row_dice_configuration, layout, false);
        EditText text = layoutDiceAdded.findViewById(R.id.number_of_dice);
        text.setText(String.format(Locale.getDefault(), "%d", die.getNumberOfDice()));
        text = layoutDiceAdded.findViewById(R.id.die_sides);
        text.setText(String.format(Locale.getDefault(), "%d", die.getNumberOfSides()));
        text = layoutDiceAdded.findViewById(R.id.die_start);
        text.setText(String.format(Locale.getDefault(), "%d", die.getStartAt()));
        text = layoutDiceAdded.findViewById(R.id.die_increment);
        text.setText(String.format(Locale.getDefault(), "%d", die.getIncrement()));
        text = layoutDiceAdded.findViewById(R.id.die_reroll);
        text.setText(String.format(Locale.getDefault(), "%d", die.getReRollOn()));
        if (!isEnd) {
            StringArrayAdapter spinAdapter = new StringArrayAdapter(R.array.operationArray);
            Spinner spinner = (Spinner) layoutInflater.inflate(R.layout.operator_view, layoutDiceAdded, false);
            spinner.setAdapter(spinAdapter);
            if (die.isAddOperation()) {
                int position = spinAdapter.getPosition(getString(R.string.addition));
                if (position >= 0) {
                    spinner.setSelection(position);
                }
            } else {
                int position = spinAdapter.getPosition(getString(R.string.subtraction));
                if (position >= 0) {
                    spinner.setSelection(position);
                }
            }
            LinearLayout item = layoutDiceAdded.findViewById(R.id.item_dice_configuration);
            item.addView(spinner);
        }
        layout.addView(layoutDiceAdded, index);
    }

    private class StringArrayAdapter extends BaseAdapter {
        private String[] strings;
        public StringArrayAdapter(int resId) {
            strings = getResources().getStringArray(resId);
        }

        public int getPosition(String inString) {
            int i = 0;
            for (String string : strings) {
                if (inString.equals(string)) {
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
                convertView = getLayoutInflater().inflate(R.layout.list_item_dice_configuration, container, false);
            }

            TextView text = convertView.findViewById(R.id.list_item);
            text.setText(getItem(position));

            return text;
        }

    }

    private class FileAdapter extends BaseAdapter {
        private String[] filelist;
        public FileAdapter() {
            File[] filearr = getFilesDir().listFiles();
            filelist = new String[filearr.length];
            int i = 0;
            for (File file : filearr) {
                filelist[i++] = file.getName();
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
            return filelist.length;
        }

        @Override
        public String getItem(int position) {
            return filelist[position];
        }

        @Override
        public long getItemId(int position) {
            return filelist[position].hashCode();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup container) {
            if (convertView == null) {
                convertView = getLayoutInflater().inflate(R.layout.list_item_dice_configuration, container, false);
            }

            TextView text = convertView.findViewById(R.id.list_item);
            text.setText(getItem(position));

            return text;
        }
    }
}
