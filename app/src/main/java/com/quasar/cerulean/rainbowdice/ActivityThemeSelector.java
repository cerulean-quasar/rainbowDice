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
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import androidx.appcompat.app.AppCompatActivity;

import android.content.res.Resources;
import android.os.Build;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;

public class ActivityThemeSelector extends AppCompatActivity implements AdapterView.OnItemSelectedListener {
    ConfigurationFile configurationFile;
    String appVersionName;
    String graphicsAPIName;
    String graphicsAPIVersion;
    String graphicsDeviceName;
    boolean hasLinearAcceleration;
    boolean hasGravity;
    boolean hasAccelerometer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        configurationFile = new ConfigurationFile(this);
        String themeName = configurationFile.getTheme();
        int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
        setTheme(currentThemeId);

        super.onCreate(savedInstanceState);

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        }

        try {
            PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
            appVersionName = info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            appVersionName = getString(R.string.unknown);
        }

        Intent intent = getIntent();
        if (intent.hasExtra(Constants.GRAPHICS_API_NAME)) {
            graphicsAPIName = intent.getStringExtra(Constants.GRAPHICS_API_NAME);
        } else {
            graphicsAPIName = null;
        }
        if (intent.hasExtra(Constants.GRAPHICS_API_VERSION)) {
            graphicsAPIVersion = intent.getStringExtra(Constants.GRAPHICS_API_VERSION);
        } else {
            graphicsAPIVersion = null;
        }
        if (intent.hasExtra(Constants.GRAPHICS_DEVICE_NAME)) {
            graphicsDeviceName = intent.getStringExtra(Constants.GRAPHICS_DEVICE_NAME);
        } else {
            graphicsDeviceName = null;
        }
        hasLinearAcceleration = intent.getBooleanExtra(Constants.SENSOR_HAS_LINEAR_ACCELERATION, false);
        hasGravity = intent.getBooleanExtra(Constants.SENSOR_HAS_GRAVITY, false);
        hasAccelerometer = intent.getBooleanExtra(Constants.SENSOR_HAS_ACCELEROMETER, false);

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

        CheckBox ck = findViewById(R.id.useLegacy);
        ck.setChecked(configurationFile.useLegacy());

        ck = findViewById(R.id.useGravity);
        if (hasAccelerometer && !hasLinearAcceleration) {
            configurationFile.setUseGravity(true);
            ck.setEnabled(false);
        } else if (!hasAccelerometer && (!hasGravity || !hasLinearAcceleration)) {
            configurationFile.setUseGravity(false);
            ck.setEnabled(false);
        }
        ck.setChecked(configurationFile.useGravity());

        ck = findViewById(R.id.drawRollingDice);
        if (!hasAccelerometer && !hasLinearAcceleration) {
            configurationFile.setDrawRollingDice(false);
            ck.setEnabled(false);
        }
        ck.setChecked(configurationFile.drawRollingDice());

        ck = findViewById(R.id.reverseGravity);
        ck.setChecked(configurationFile.reverseGravity());

        TextView view = findViewById(R.id.appVersionName);
        view.setText(appVersionName);

        if (graphicsAPIName != null) {
            view = findViewById(R.id.graphicsAPIName);
            view.setText(graphicsAPIName);
        }

        if (graphicsAPIVersion != null) {
            view = findViewById(R.id.graphicsAPIVersion);
            view.setText(graphicsAPIVersion);
        }

        if (graphicsDeviceName != null) {
            view = findViewById(R.id.graphicsDeviceName);
            view.setText(graphicsDeviceName);
        }

        ArrayList<String> sensors = new ArrayList<>();
        if (hasAccelerometer) {
            sensors.add(getString(R.string.accelerometerSensor));
        }
        if (hasLinearAcceleration) {
            sensors.add(getString(R.string.linearAccelerationSensor));
        }
        if (hasGravity) {
            sensors.add(getString(R.string.gravitySensor));
        }
        StringBuilder sensorsString = new StringBuilder();
        int length = sensors.size();
        for (i = 0; i < length; i++) {
            sensorsString.append(sensors.get(i));
            if (i != length - 1) {
                sensorsString.append(",\n");
            }
        }
        TextView sensorText = findViewById(R.id.sensorsList);
        sensorText.setText(sensorsString.toString());
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_theme_selection_menu, menu);
        return true;
    }

    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        if (view == null) {
            return;
        }
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

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        } else {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(android.R.attr.windowBackground, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        }

        initializeGui(false);
    }

    public void onCheckboxClicked(View view) {
        boolean checked = ((CheckBox) view).isChecked();

        switch (view.getId()) {
            case R.id.useGravity:
                configurationFile.setUseGravity(checked);
                break;
            case R.id.drawRollingDice:
                configurationFile.setDrawRollingDice(checked);
                break;
            case R.id.useLegacy:
                configurationFile.setUseLegacy(checked);
                break;
            case R.id.reverseGravity:
                configurationFile.setReverseGravity(checked);
        }
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
        String theme = configurationFile.getTheme();
        if (text != null) {
            theme = text.getText().toString();
            configurationFile.setThemeName(theme);
        }

        configurationFile.writeFile();

        Intent intent = new Intent();
        intent.putExtra(Constants.themeNameConfigValue, theme);

        setResult(RESULT_OK, intent);
        finish();
    }
}