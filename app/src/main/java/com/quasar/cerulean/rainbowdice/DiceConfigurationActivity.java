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

import android.content.Intent;
import android.content.res.Configuration;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.BaseAdapter;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import java.io.File;

import static com.quasar.cerulean.rainbowdice.Constants.DICE_CUSTOMIZATION_ACTIVITY;
import static com.quasar.cerulean.rainbowdice.Constants.DICE_FILENAME;

public class DiceConfigurationActivity extends AppCompatActivity {
    ConfigurationFile configurationFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        configurationFile = new ConfigurationFile(this);
        String theme = configurationFile.getTheme();
        if (theme != null && !theme.isEmpty()) {
            int resID = getResources().getIdentifier(theme, "style", getPackageName());
            setTheme(resID);
        }

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dice_configuration);

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        }

        resetFileList();
        resetFavoriteSpinners();
    }

    private void resetFileList() {
        LinearLayout diceLayout = findViewById(R.id.dice_layout);
        diceLayout.removeAllViews();
        LayoutInflater inflater = getLayoutInflater();
        File[] filearr = getFilesDir().listFiles();
        for (File file : filearr) {
            if (!file.getName().equals(ConfigurationFile.configFile)) {
                LinearLayout layout = (LinearLayout) inflater.inflate(R.layout.edit_delete_row, diceLayout, false);
                TextView text = layout.findViewById(R.id.filename);
                text.setText(file.getName());
                diceLayout.addView(layout);
            }
        }
    }

    private void resetFavoriteSpinners() {
        String[] excludeFiles = new String[2];
        excludeFiles[0] = ConfigurationFile.configFile;
        excludeFiles[1] = LogFile.diceLogFilename;
        Spinner spinner1 = findViewById(R.id.favorite1);
        FileAdapter spinAdapter1 = new FileAdapter(this, excludeFiles);
        spinner1.setAdapter(spinAdapter1);

        Spinner spinner2 = findViewById(R.id.favorite2);
        FileAdapter spinAdapter2 = new FileAdapter(this, excludeFiles);
        spinner2.setAdapter(spinAdapter2);

        Spinner spinner3 = findViewById(R.id.favorite3);
        FileAdapter spinAdapter3 = new FileAdapter(this, excludeFiles);
        spinner3.setAdapter(spinAdapter3);

        String fav1File = configurationFile.getFavorite1();
        String fav2File = configurationFile.getFavorite2();
        String fav3File = configurationFile.getFavorite3();

        if (fav1File != null && !fav1File.isEmpty()) {
            int pos = spinAdapter1.getPosition(fav1File);
            if (pos != -1) {
                spinner1.setSelection(pos);
            }
        }

        if (fav2File != null && !fav2File.isEmpty()) {
            int pos = spinAdapter2.getPosition(fav2File);
            if (pos != -1) {
                spinner2.setSelection(pos);
            }
        }

        if (fav3File != null && !fav3File.isEmpty()) {
            int pos = spinAdapter3.getPosition(fav3File);
            if (pos != -1) {
                spinner3.setSelection(pos);
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_configuration_menu, menu);
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == DICE_CUSTOMIZATION_ACTIVITY) {
            if (resultCode == RESULT_CANCELED) {
                // The user cancelled editing
                return;
            }

            resetFavoriteSpinners();
            resetFileList();
        }
    }

    public void onDeleteFile(View view) {
        // find the list item that created the delete event
        LinearLayout layout = findViewById(R.id.dice_layout);
        for (int i=0; i < layout.getChildCount(); i++) {
            View item = layout.getChildAt(i);
            ImageButton delete = item.findViewById(R.id.trash);
            if (delete == view) {
                // this is the row that caused the delete event
                TextView text = item.findViewById(R.id.filename);
                String filename = text.getText().toString();

                // delete the file
                if (!filename.isEmpty()) {
                    deleteFile(filename);
                    layout.removeViewAt(i);
                    resetFavoriteSpinners();
                }
            }
        }

    }

    public void onEdit(View view) {
        // find the list item that created the edit event
        LinearLayout layout = findViewById(R.id.dice_layout);
        ViewParent parent = view.getParent();
        for (int i=0; i < layout.getChildCount(); i++) {
            LinearLayout item = (LinearLayout)layout.getChildAt(i);
            ImageButton edit = item.findViewById(R.id.editDice);
            if (edit == view) {
                // this is the row that caused the edit event
                TextView text = item.findViewById(R.id.filename);
                String filename = text.getText().toString();

                // start the customization app with the file
                if (!filename.isEmpty()) {
                    Intent intent = new Intent(this, DiceCustomizationActivity.class);
                    intent.putExtra(DICE_FILENAME, filename);
                    startActivityForResult(intent, DICE_CUSTOMIZATION_ACTIVITY);
                }
            }
        }

    }

    public void onDone(MenuItem item) {
        Spinner spinner = findViewById(R.id.favorite1);
        TextView text = (TextView)spinner.getSelectedView();
        if (text != null) {
            String filename = text.getText().toString();
            configurationFile.setFavorite1(filename);
        }

        spinner = findViewById(R.id.favorite2);
        text = (TextView)spinner.getSelectedView();
        if (text != null) {
            String filename = text.getText().toString();
            configurationFile.setFavorite2(filename);
        }

        spinner = findViewById(R.id.favorite3);
        text = (TextView)spinner.getSelectedView();
        if (text != null) {
            String filename = text.getText().toString();
            configurationFile.setFavorite3(filename);
        }

        configurationFile.writeFile();

        setResult(RESULT_OK, null);
        finish();
    }

    public void onNewDiceConfiguration(MenuItem item) {
        Intent intent = new Intent(this, DiceCustomizationActivity.class);
        startActivityForResult(intent, DICE_CUSTOMIZATION_ACTIVITY);
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

}
