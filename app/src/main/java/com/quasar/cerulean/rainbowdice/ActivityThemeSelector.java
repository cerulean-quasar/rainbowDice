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
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

public class ActivityThemeSelector extends AppCompatActivity implements AdapterView.OnItemSelectedListener {
    ConfigurationFile configurationFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        configurationFile = new ConfigurationFile(this);
        int currentThemeId = getApplicationInfo().theme;
        String themeName = configurationFile.getTheme();
        if (themeName != null && !themeName.isEmpty()) {
            currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
            setTheme(currentThemeId);
        }

        super.onCreate(savedInstanceState);
        initializeGui(true);
    }

    private void initializeGui(boolean loadTheme) {
        int currentThemeId = getApplicationInfo().theme;
        String themeName = configurationFile.getTheme();
        if (themeName != null && !themeName.isEmpty()) {
            currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
            if (loadTheme) {
                setTheme(currentThemeId);
            }
        }
        setContentView(R.layout.activity_theme_selector);

        Spinner themeSelector = findViewById(R.id.themeSelector);
        ArrayAdapter<CharSequence> spinAdapter = ArrayAdapter.createFromResource(this,
                R.array.themeArray, android.R.layout.simple_spinner_item);
        spinAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        String[] strings = getResources().getStringArray(R.array.themeArray);
        int pos = 0;
        int i=0;
        for (String string : strings) {
            int resID = getResources().getIdentifier(string, "style", getPackageName());
            if (resID == currentThemeId) {
                pos = i;
                break;
            }
            i++;
        }
        themeSelector.setAdapter(spinAdapter);
        themeSelector.setSelection(pos);
        themeSelector.setOnItemSelectedListener(this);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_theme_selection_menu, menu);
        return true;
    }

    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        TextView text = (TextView) view;

        String themeName = text.getText().toString();
        int currentThemeId = getApplicationInfo().theme;
        String currentThemeName = configurationFile.getTheme();
        if (currentThemeName != null && !currentThemeName.isEmpty()) {
            currentThemeId = getResources().getIdentifier(currentThemeName, "style", getPackageName());
        }
        int themeId = getResources().getIdentifier(themeName, "style", getPackageName());
        if (themeId == currentThemeId) {
            // we're already using this theme.  Just break out.
            return;
        }
        configurationFile.setThemeName(themeName);
        setTheme(themeId);

        TypedValue value = new TypedValue();
        getTheme().resolveAttribute(android.R.attr.windowBackground, value, true);
        getWindow().setBackgroundDrawableResource(value.resourceId);

        initializeGui(false);
    }

    public void onNothingSelected(AdapterView<?> parent) {
        // do nothing
    }

    public void onCancelThemeSelection(MenuItem item) {
        // return from the application without doing anything.
        setResult(RESULT_CANCELED);
        finish();
    }

    public void onSaveThemeSelection(MenuItem item) {
        Spinner spinner = findViewById(R.id.themeSelector);
        TextView text = (TextView)spinner.getSelectedView();
        if (text != null) {
            String theme = text.getText().toString();
            configurationFile.setThemeName(theme);
            configurationFile.writeFile();

            Intent intent = new Intent();
            intent.putExtra(Constants.themeNameConfigValue, theme);

            setResult(RESULT_OK, intent);
            finish();
        } else {
            setResult(RESULT_CANCELED);
            finish();
        }
    }

}

