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

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Parcelable;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Layout;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewParent;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Locale;

public class DiceCustomizationActivity extends AppCompatActivity implements AdapterView.OnItemSelectedListener {
    private static final int DEFAULT_START = 1;
    private static final int DEFAULT_INCREMENT = 1;
    private static final int DEFAULT_NBR_DICE = 1;
    private static final int DEFAULT_NBR_SIDES = 6;
    private String saveFileName = null;
    private Dialog saveDialog = null;

    private class DiceGuiConfig {
        public DieConfiguration config;
        public Button button;

        public DiceGuiConfig(DieConfiguration inDiceConfig, Button inDiceConfigButton) {
            config = inDiceConfig;
            button = inDiceConfigButton;
        }
    }

    ArrayList<DiceGuiConfig> diceConfigs;
    Button[] diceSidesButtons;
    int configBeingEdited = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        ConfigurationFile configurationFile = new ConfigurationFile(this);
        String theme = configurationFile.getTheme();
        if (theme != null && !theme.isEmpty()) {
            int resID = getResources().getIdentifier(theme, "style", getPackageName());
            setTheme(resID);
        }

        super.onCreate(savedInstanceState);

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            setContentView(R.layout.activity_dice_customization_landscape);
            LinearLayout layout = findViewById(R.id.dice_list);
            layout.setOrientation(LinearLayout.VERTICAL);
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        } else {
            setContentView(R.layout.activity_dice_customization);
        }

        diceSidesButtons = new Button[11];
        int i = 0;
        diceSidesButtons[i++] = findViewById(R.id.d2Button);
        diceSidesButtons[i++] = findViewById(R.id.d3Button);
        diceSidesButtons[i++] = findViewById(R.id.d4Button);
        diceSidesButtons[i++] = findViewById(R.id.d6Button);
        diceSidesButtons[i++] = findViewById(R.id.d8Button);
        diceSidesButtons[i++] = findViewById(R.id.d10Button);
        diceSidesButtons[i++] = findViewById(R.id.d12Button);
        diceSidesButtons[i++] = findViewById(R.id.d20Button);
        diceSidesButtons[i++] = findViewById(R.id.d30Button);
        diceSidesButtons[i++] = findViewById(R.id.d100Button);
        diceSidesButtons[i++] = findViewById(R.id.otherButton);

        DieConfiguration[] configs = null;
        Parcelable[] parcelableConfigs = null;
        if (savedInstanceState != null) {
            parcelableConfigs = savedInstanceState.getParcelableArray(Constants.DICE_CONFIGURATION);
        }
        if (parcelableConfigs != null) {
            i = 0;
            configs = new DieConfiguration[parcelableConfigs.length];
            for (Parcelable parcelableConfig : parcelableConfigs) {
                configs[i++] = (DieConfiguration) parcelableConfig;
            }
            saveFileName = savedInstanceState.getString(Constants.DICE_FILENAME);
        } else {
            Intent intent = getIntent();
            String filename = intent.getStringExtra(Constants.DICE_FILENAME);
            if (filename != null && !filename.isEmpty()) {
                try {
                    FileInputStream inputStream = openFileInput(filename);
                    configs = DieConfiguration.loadFromFile(inputStream);
                    inputStream.close();
                    saveFileName = filename;
                } catch (FileNotFoundException e) {
                    System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
                    filename = null;
                } catch (IOException e) {
                    System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
                    filename = null;
                }

            }
        }

        if (configs == null) {
            // load the default configuration
            configs = new DieConfiguration[1];
            configs[0] = new DieConfiguration(1, 6, 1, 1, -1, true);
        }

        configsToView(configs);
        TypedValue value = new TypedValue();
        getTheme().resolveAttribute(R.attr.rounded_rectangle_clicked, value, true);
        ((LinearLayout)diceConfigs.get(0).button.getParent()).setBackground(getResources().getDrawable(value.resourceId,null));

        TextView text = findViewById(R.id.die_start);
        text.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    TextView text = (TextView) v;
                    String string = text.getText().toString();
                    boolean reroll = true;
                    if (cfg.getStartAt() > cfg.getReRollOn()) {
                        reroll = false;
                    }

                    if (string.isEmpty()) {
                        // set to the default!
                        text.setText(String.format(Locale.getDefault(), "%d", DEFAULT_START));
                        cfg.setStartAt(DEFAULT_START);
                    } else {
                        cfg.setStartAt(Integer.decode(string));
                    }

                    if (!reroll) {
                        cfg.setReRollOn(cfg.getStartAt() - 1);
                    }

                    repopulateCurrentListItemFromConfig();
                }
            }
        });

        text = findViewById(R.id.die_next);
        text.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    TextView text = (TextView) v;
                    String string = text.getText().toString();
                    if (string.isEmpty()) {
                        // set to the default!
                        text.setText(String.format(Locale.getDefault(), "%d", cfg.getStartAt() + DEFAULT_INCREMENT));
                        cfg.setIncrement(DEFAULT_INCREMENT);
                    } else {
                        cfg.setIncrement(Integer.decode(string) - cfg.getStartAt());
                    }

                    repopulateCurrentListItemFromConfig();
                }
            }
        });

        text = findViewById(R.id.number_of_dice);
        text.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    TextView text = (TextView) v;
                    String string = text.getText().toString();
                    if (string.isEmpty()) {
                        // set to the default!
                        text.setText(String.format(Locale.getDefault(), "%d", DEFAULT_NBR_DICE));
                        cfg.setNumberOfDice(DEFAULT_NBR_DICE);
                    } else {
                        int nbrDice = Integer.decode(string);
                        cfg.setNumberOfDice(nbrDice);
                    }

                    repopulateCurrentListItemFromConfig();
                }
            }
        });

        text = findViewById(R.id.die_reroll);
        text.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    TextView text = (TextView) v;
                    String string = text.getText().toString();
                    if (string.isEmpty()) {
                        // Empty means no re-roll.  Set the re-roll value to the start at value - 1
                        // thus, no re-rolls will occur.
                        cfg.setReRollOn(cfg.getStartAt() - 1);
                    } else {
                        cfg.setReRollOn(Integer.decode(string));
                    }

                    repopulateCurrentListItemFromConfig();
                }
            }
        });

        text = findViewById(R.id.otherText);
        text.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    TextView text = (TextView) v;
                    String string = text.getText().toString();
                    if (string.isEmpty()) {
                        // set to the default!
                        text.setText(String.format(Locale.getDefault(), "%d", DEFAULT_NBR_SIDES));
                        cfg.setNumberOfSides(DEFAULT_NBR_SIDES);
                    } else {
                        cfg.setNumberOfSides(Integer.decode(string));
                    }

                    repopulateCurrentListItemFromConfig();
                }
            }
        });
    }

    @Override
    public void onSaveInstanceState(Bundle instanceState) {
        super.onSaveInstanceState(instanceState);

        //  First gather up the data on the screen and update the config being edited with it.
        updateConfigFromScreen();

        // save all the configs to the instance state.
        DieConfiguration[] dice = new DieConfiguration[diceConfigs.size()];
        int i=0;
        for (DiceGuiConfig die : diceConfigs) {
            dice[i++] = die.config;
        }

        instanceState.putParcelableArray(Constants.DICE_CONFIGURATION, dice);

        if (saveFileName != null) {
            instanceState.putString(Constants.DICE_FILENAME, saveFileName);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_customization_menu, menu);
        return true;
    }

    private void repopulateCurrentListItemFromConfig() {
        DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

        // Set the text that specifies the numbers on the die in the format: 1,2,...,6
        LinearLayout layout = findViewById(R.id.dice_list);
        View item = layout.getChildAt(configBeingEdited * 2);
        TextView nbrsOnDice = item.findViewById(R.id.numbers_on_die);
        nbrsOnDice.setText(String.format(Locale.getDefault(),
                getString(R.string.diceNumbersString),
                cfg.getStartAt(), cfg.getStartAt() + cfg.getIncrement(),
                cfg.getStartAt() + cfg.getIncrement() * (cfg.getNumberOfSides()-1)));

        // Set the text that specifies the dice being rolled in the format: 1D6
        Button dieConfigButton = item.findViewById(R.id.die_config_button);
        dieConfigButton.setText(String.format(Locale.getDefault(),
                getString(R.string.diceConfigStringRepresentation),
                cfg.getNumberOfDice(), cfg.getNumberOfSides()));

        TextView reroll = item.findViewById(R.id.die_reroll_text);
        if (cfg.getReRollOn() > cfg.getStartAt()) {
            // update the re-roll text
            reroll.setText(String.format(Locale.getDefault(), getString(R.string.rerollText),
                    cfg.getReRollOn()));
        } else {
            reroll.setText("");
        }
    }

    public void dxClicked(View view) {
        int i=0;
        for (Button button : diceSidesButtons) {
            if (view == button) {
                TypedValue value = new TypedValue();
                getTheme().resolveAttribute(R.attr.round_button_clicked, value, true);
                button.setBackground(getResources().getDrawable(value.resourceId,null));

                if (configBeingEdited < 0) {
                    // shouldn't happen
                    return;
                }

                EditText text = findViewById(R.id.otherText);
                if (view.getId() == R.id.otherButton) {
                    text.setEnabled(true);
                } else {
                    text.setEnabled(false);
                }

                // update the text in scrolling view representing the whole dice config
                int nbrSidesPressed = nbrSidesForDiceSidesButton(i);
                DiceGuiConfig cfg = diceConfigs.get(configBeingEdited);
                cfg.config.setNumberOfSides(nbrSidesPressed);

                // the dice button
                cfg.button.setText(String.format(Locale.getDefault(),
                        getString(R.string.diceConfigStringRepresentation),
                        cfg.config.getNumberOfDice(), nbrSidesPressed));

                // the dice text string for the numbers it can roll
                LinearLayout layout = (LinearLayout) cfg.button.getParent();
                TextView nbrsOnDice = layout.findViewById(R.id.numbers_on_die);
                nbrsOnDice.setText(String.format(Locale.getDefault(), getString(R.string.diceNumbersString),
                        cfg.config.getStartAt(),
                        cfg.config.getStartAt() + cfg.config.getIncrement(),
                        cfg.config.getStartAt() + cfg.config.getIncrement()*(nbrSidesPressed-1)));

            } else {
                TypedValue value = new TypedValue();
                getTheme().resolveAttribute(R.attr.round_button_unclicked, value, true);
                button.setBackground(getResources().getDrawable(value.resourceId,null));
            }
            i++;
        }
    }

    public void onDiceListItemClick(View view) {
        // first gather the data from the screen and update the config.  So that you get the config
        // value changes where the focus has not been lost yet.
        updateConfigFromScreen();

        // then update the dice list item with the updated config values
        repopulateCurrentListItemFromConfig();

        // now switch the active dice set.
        int i = 0;
        for (DiceGuiConfig cfg : diceConfigs) {
            if (cfg.button == view) {
                TypedValue value = new TypedValue();
                getTheme().resolveAttribute(R.attr.rounded_rectangle_clicked, value, true);
                ((LinearLayout)cfg.button.getParent()).setBackground(getResources().getDrawable(value.resourceId,null));
                configBeingEdited = i;
                editConfig(i);
            } else {
                TypedValue value = new TypedValue();
                getTheme().resolveAttribute(R.attr.rounded_rectangle_unclicked, value, true);
                ((LinearLayout)cfg.button.getParent()).setBackground(getResources().getDrawable(value.resourceId,null));
            }
            i++;
        }
    }

    public void onDelete(View view) {
        int nbrConfigs = diceConfigs.size();

        if (nbrConfigs == 1) {
            // refuse to delete the only config...
            return;
        }

        LinearLayout layoutDiceList = findViewById(R.id.dice_list);

        diceConfigs.remove(configBeingEdited);
        layoutDiceList.removeViewAt(configBeingEdited*2);

        // if we are at the beginning, delete the divider right after us, otherwise delete
        // the divider before us.  Also, always force the first item to be added (not negated).
        if (configBeingEdited == 0) {
            layoutDiceList.removeViewAt(0);
            diceConfigs.get(0).config.setIsAddOperation(true);
        } else {
            layoutDiceList.removeViewAt(configBeingEdited*2-1);
        }

        configBeingEdited = 0;
        editConfig(configBeingEdited);
        TypedValue value = new TypedValue();
        getTheme().resolveAttribute(R.attr.rounded_rectangle_clicked, value, true);
        ((LinearLayout)diceConfigs.get(configBeingEdited).button.getParent()).setBackground(getResources().getDrawable(value.resourceId,null));
    }

    public void onNew(View view) {
        // first gather the data from the screen and update the config.  So that you get the config
        // value changes where the focus has not been lost yet.
        updateConfigFromScreen();

        // then update the dice list item with the updated config values
        repopulateCurrentListItemFromConfig();

        LinearLayout layoutDiceList = findViewById(R.id.dice_list);
        LayoutInflater inflater = getLayoutInflater();

        // Unset the currently highlighted item so that it is not shown as being edited any longer
        for (int i=0; i < layoutDiceList.getChildCount(); i+=2) {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.rounded_rectangle_unclicked, value, true);
            layoutDiceList.getChildAt(i).setBackground(getResources().getDrawable(value.resourceId,null));
        }

        // first add the divider (which contains the operation...
        LinearLayout layoutNew = (LinearLayout)inflater.inflate(R.layout.dice_list_divider,
                layoutDiceList, false);

        Spinner operationSpinner = layoutNew.findViewById(R.id.operation);

        ArrayAdapter<CharSequence> spinAdapter = ArrayAdapter.createFromResource(this,
                R.array.operationArray, android.R.layout.simple_spinner_item);
        spinAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        operationSpinner.setAdapter(spinAdapter);
        operationSpinner.setOnItemSelectedListener(this);

        layoutDiceList.addView(layoutNew);

        // then add the new dice item and set up the fields
        layoutNew = (LinearLayout)inflater.inflate(R.layout.dice_list_item, layoutDiceList, false);
        TypedValue value = new TypedValue();
        getTheme().resolveAttribute(R.attr.rounded_rectangle_clicked, value, true);
        layoutNew.setBackground(getResources().getDrawable(value.resourceId,null));

        Button button = layoutNew.findViewById(R.id.die_config_button);
        DieConfiguration config = new DieConfiguration(DEFAULT_NBR_DICE, DEFAULT_NBR_SIDES,
                DEFAULT_START, DEFAULT_INCREMENT, DEFAULT_START - 1, true);
        diceConfigs.add(new DiceGuiConfig(config, button));
        button.setText(String.format(Locale.getDefault(), getString(R.string.diceConfigStringRepresentation),
                config.getNumberOfDice(), config.getNumberOfSides()));

        TextView diceNumbers = layoutNew.findViewById(R.id.numbers_on_die);

        diceNumbers.setText(String.format(Locale.getDefault(), getString(R.string.diceNumbersString),
                config.getStartAt(),
                config.getStartAt() + config.getIncrement(),
                config.getStartAt() + (config.getNumberOfSides()-1) * config.getIncrement()));
        layoutDiceList.addView(layoutNew);

        // populate the detailed dice config page
        editConfig(diceConfigs.size() - 1);

        // Scroll to the end
        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
            HorizontalScrollView scrollViewDice = findViewById(R.id.scrollViewDice);
            scrollViewDice.fullScroll(View.FOCUS_RIGHT);
        } else {
            ScrollView scrollViewDice = findViewById(R.id.scrollViewDiceLandscape);
            scrollViewDice.fullScroll(View.FOCUS_DOWN);
        }
    }

    public void onItemSelected(AdapterView<?> parent, View view, int pos, long id) {
        // the user selected an operation in the operation spinner, we need to update the
        // appropriate DieConfiguration.

        if (view == null) {
            // I'm not sure why we get null here when the screen rotates...
            return;
        }

        ViewParent item = parent.getParent();
        LinearLayout layoutDiceList = findViewById(R.id.dice_list);
        int nbrItems = layoutDiceList.getChildCount();
        int i;
        for (i=0; i < nbrItems; i++) {
            if (item == layoutDiceList.getChildAt(i)) {
                break;
            }
        }

        if (i==nbrItems) {
            // not found - should not happen
            return;
        }

        TextView text = (TextView) view;
        if (text.getText().toString().equals(getString(R.string.addition))) {
            diceConfigs.get(i / 2 + 1).config.setIsAddOperation(true);
        } else {
            diceConfigs.get(i / 2 + 1).config.setIsAddOperation(false);
        }
    }

    public void onNothingSelected(AdapterView<?> parent) {
    }

    public void onSaveDiceCustomization(MenuItem item) {
        LinearLayout layout = findViewById(R.id.dice_list);
        LayoutInflater inflater = getLayoutInflater();

        LinearLayout saveDialogView = (LinearLayout) inflater.inflate(R.layout.save_as_dialog, layout, false);

        if (saveFileName != null) {
            EditText text = saveDialogView.findViewById(R.id.filename);
            text.setText(saveFileName);
        }


        saveDialog = new AlertDialog.Builder(this).setTitle(R.string.save).setView(saveDialogView).show();
    }

    public void onDoneDiceCustomization(MenuItem item) {
        setResult(RESULT_OK, null);
        finish();
    }

    public void onCancel(View view) {
        saveDialog.dismiss();
        saveDialog = null;
    }

    public void onSaveFile(View view) {
        EditText text = saveDialog.findViewById(R.id.filename);

        String filename = text.getText().toString();
        if (filename.isEmpty()) {
            return;
        }

        updateConfigFromScreen();

        DieConfiguration[] configs = new DieConfiguration[diceConfigs.size()];
        for (int i = 0; i < configs.length; i++) {
            configs[i] = diceConfigs.get(i).config;
        }

        try {
            FileOutputStream outputStream = openFileOutput(filename, Context.MODE_PRIVATE);
            DieConfiguration.saveToFile(outputStream, configs);
            outputStream.close();
        } catch (IOException e) {
            System.out.println("Exception on writing to file: " + filename + " message: " + e.getMessage());
            return;
        }

        saveDialog.dismiss();
        saveDialog = null;
    }

    private void updateConfigFromScreen() {
        // Repopulate the dice config from the screen in case the user changed something
        // there and the field has not lost focus yet.
        if (configBeingEdited >= 0) {
            DieConfiguration config = diceConfigs.get(configBeingEdited).config;

            EditText diceField = findViewById(R.id.number_of_dice);
            String stringDiceField = diceField.getText().toString();
            int nbrDice = config.getNumberOfDice();
            if (!stringDiceField.isEmpty()) {
                nbrDice = Integer.valueOf(stringDiceField);
                config.setNumberOfDice(nbrDice);
            }

            diceField = findViewById(R.id.die_start);
            stringDiceField = diceField.getText().toString();
            int start = config.getStartAt();
            if (!stringDiceField.isEmpty()) {
                start = Integer.valueOf(stringDiceField);
                config.setStartAt(start);
            }

            diceField = findViewById(R.id.die_next);
            stringDiceField = diceField.getText().toString();
            int increment = config.getIncrement();
            if (!stringDiceField.isEmpty()) {
                increment = Integer.valueOf(stringDiceField) - start;
                config.setIncrement(increment);
            }

            diceField = findViewById(R.id.die_reroll);
            stringDiceField = diceField.getText().toString();
            int reroll = config.getReRollOn();
            if (!stringDiceField.isEmpty()) {
                reroll = Integer.valueOf(stringDiceField);
                config.setReRollOn(reroll);
            }

            diceField = findViewById(R.id.otherText);
            if (diceField.isEnabled()) {
                stringDiceField = diceField.getText().toString();
                int nbrSides = config.getNumberOfSides();
                if (!stringDiceField.isEmpty()) {
                    nbrSides = Integer.valueOf(stringDiceField);
                    config.setNumberOfSides(nbrSides);
                }
            }
        }
    }

    private void configsToView(DieConfiguration[] configs) {
        if (configs == null) {
            // shouldn't happen
            return;
        }

        diceConfigs = new ArrayList<>();

        LinearLayout layout = findViewById(R.id.dice_list);
        layout.removeAllViews();
        LayoutInflater inflater = getLayoutInflater();

        int i = 0;
        Spinner operation = null;
        // by default, the first config selected for editing when the new config is loaded
        for (DieConfiguration config : configs) {
            // Set the operation of the previously added operator to what it says in the current
            // die config.
            if (operation != null) {
                if (config.isAddOperation()) {
                    operation.setSelection(0);
                } else {
                    operation.setSelection(1);
                }
            }

            LinearLayout layoutDiceListItem = (LinearLayout) inflater.inflate(R.layout.dice_list_item, layout, false);
            TextView diceNumbers = layoutDiceListItem.findViewById(R.id.numbers_on_die);
            Button diceButton = layoutDiceListItem.findViewById(R.id.die_config_button);

            // save all the config buttons so that we can easily find the one being clicked
            // and set it to pressed and set all the other ones to un-pressed.
            diceConfigs.add(new DiceGuiConfig(config, diceButton));

            diceNumbers.setText(String.format(Locale.getDefault(), getString(R.string.diceNumbersString),
                    config.getStartAt(),
                    config.getStartAt() + config.getIncrement(),
                    config.getStartAt() + (config.getNumberOfSides()-1) * config.getIncrement()));

            if (config.getReRollOn() >= config.getStartAt()) {
                TextView reRoll = layoutDiceListItem.findViewById(R.id.die_reroll_text);
                reRoll.setText(String.format(Locale.getDefault(), getString(R.string.rerollText), config.getReRollOn()));
            }

            diceButton.setText(String.format(Locale.getDefault(), getString(R.string.diceConfigStringRepresentation),
                    config.getNumberOfDice(), config.getNumberOfSides()));
            layout.addView(layoutDiceListItem);

            if (i <= configs.length - 2) {
                // there is at least one more dice config to come, add a divider
                LinearLayout layoutDivider = (LinearLayout) inflater.inflate(R.layout.dice_list_divider, layout, false);
                operation = layoutDivider.findViewById(R.id.operation);
                ArrayAdapter<CharSequence> spinAdapter = ArrayAdapter.createFromResource(this,
                        R.array.operationArray, android.R.layout.simple_spinner_item);
                spinAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                operation.setAdapter(spinAdapter);
                operation.setOnItemSelectedListener(this);
                layout.addView(layoutDivider);
            }
            i++;
        }

        editConfig(0);
    }

    private void editConfig(int i) {
        configBeingEdited = i;
        DieConfiguration config = diceConfigs.get(i).config;
        for (Button button : diceSidesButtons) {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.round_button_unclicked, value, true);
            button.setBackground(getResources().getDrawable(value.resourceId,null));
        }

        Button button = getButtonForDieSides(config.getNumberOfSides());
        TextView other = findViewById(R.id.otherText);
        if (button == getButtonForDieSides(-1)) {
            // the "other" button is pressed.  Populate the associated edit text with
            // the number of sides and enable it.
            other.setEnabled(true);
            other.setText(String.format(Locale.getDefault(), "%d", config.getNumberOfSides()));
        } else {
            other.setEnabled(false);
        }

        TypedValue value = new TypedValue();
        getTheme().resolveAttribute(R.attr.round_button_clicked, value, true);
        button.setBackground(getResources().getDrawable(value.resourceId,null));

        EditText edit = findViewById(R.id.number_of_dice);
        edit.setText(String.format(Locale.getDefault(), "%d", config.getNumberOfDice()));
        edit = findViewById(R.id.die_start);
        edit.setText(String.format(Locale.getDefault(), "%d", config.getStartAt()));
        edit = findViewById(R.id.die_next);
        edit.setText(String.format(Locale.getDefault(), "%d", config.getIncrement() + config.getStartAt()));
        edit = findViewById(R.id.die_reroll);
        if (config.getReRollOn() >= config.getStartAt()) {
            edit.setText(String.format(Locale.getDefault(), "%d", config.getReRollOn()));
        } else {
            edit.setText("");
        }
        configBeingEdited = i;
    }

    private int nbrSidesForDiceSidesButton(int index) {
        switch (index) {
            case 0:
                return 2;
            case 1:
                return 3;
            case 2:
                return 4;
            case 3:
                return 6;
            case 4:
                return 8;
            case 5:
                return 10;
            case 6:
                return 12;
            case 7:
                return 20;
            case 8:
                return 30;
            case 9:
                return 100;
            default:
                TextView other = findViewById(R.id.otherText);
                String otherTextString = other.getText().toString();
                if (otherTextString.isEmpty()) {
                    other.setText(String.format(Locale.getDefault(), "%d", DEFAULT_NBR_SIDES));
                    return DEFAULT_NBR_SIDES;
                }
                return Integer.decode(otherTextString);
        }
    }
    private Button getButtonForDieSides(int nbrSides) {
        switch (nbrSides) {
            case 2:
                return diceSidesButtons[0];
            case 3:
                return diceSidesButtons[1];
            case 4:
                return diceSidesButtons[2];
            case 6:
                return diceSidesButtons[3];
            case 8:
                return diceSidesButtons[4];
            case 10:
                return diceSidesButtons[5];
            case 12:
                return diceSidesButtons[6];
            case 20:
                return diceSidesButtons[7];
            case 30:
                return diceSidesButtons[8];
            case 100:
                return diceSidesButtons[9];
            default:
                return diceSidesButtons[10];
        }
    }
}
