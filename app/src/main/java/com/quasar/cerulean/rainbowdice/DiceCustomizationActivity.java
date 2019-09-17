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
import android.content.res.Resources;
import android.os.Parcelable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewParent;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;

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
    private static final int OTHER_BUTTON_INDEX = 10;
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

    private class DiceSidesInfo {
        DiceSidesInfo(Button inButton, int inDrawableClicked, int inDrawableUnclicked) {
            button = inButton;

            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(inDrawableClicked, value, true);
            drawableClicked = value.resourceId;

            value = new TypedValue();
            getTheme().resolveAttribute(inDrawableUnclicked, value, true);
            drawableUnclicked = value.resourceId;
        }
        public Button button;
        public int drawableClicked;
        public int drawableUnclicked;
    }
    DiceSidesInfo[] diceSidesInfos;

    int configBeingEdited = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        ConfigurationFile configurationFile = new ConfigurationFile(this);
        String themeName = configurationFile.getTheme();
        int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
        setTheme(currentThemeId);

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_dice_customization);
        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            LinearLayout layout = findViewById(R.id.dice_list);
            layout.setOrientation(LinearLayout.VERTICAL);
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        }

        diceSidesInfos = new DiceSidesInfo[11];
        int i = 0;
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d1Button),
                R.attr.constant_clicked, R.attr.constant_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d2Button),
                R.attr.d2_clicked, R.attr.d2_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d3Button),
                R.attr.d6_clicked, R.attr.d6_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d4Button),
                R.attr.d4_clicked, R.attr.d4_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d6Button),
                R.attr.d6_clicked, R.attr.d6_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d8Button),
                R.attr.d8_clicked, R.attr.d8_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d10Button),
                R.attr.d10_clicked, R.attr.d10_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d12Button),
                R.attr.d12_clicked, R.attr.d12_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d20Button),
                R.attr.d20_clicked, R.attr.d20_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.d30Button),
                R.attr.d30_clicked, R.attr.d30_unclicked);
        diceSidesInfos[i++] = new DiceSidesInfo((Button)findViewById(R.id.otherButton),
                R.attr.d10_clicked, R.attr.d10_unclicked);

        configBeingEdited = 0;
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
            configBeingEdited = savedInstanceState.getInt(Constants.DICE_BEING_EDITED);
        } else {
            Intent intent = getIntent();
            String filename = intent.getStringExtra(Constants.DICE_FILENAME);
            if (filename != null && !filename.isEmpty()) {
                try {
                    FileInputStream inputStream = openFileInput(filename);
                    DiceGroup dice = DiceGroup.loadFromFile(inputStream);
                    if (dice != null) {
                        configs = dice.dieConfigurations();
                    }
                    inputStream.close();
                    saveFileName = filename;
                } catch (FileNotFoundException e) {
                    System.out.println("Could not find file on opening: " + filename + " message: " + e.getMessage());
                    configs = null;
                    saveFileName = null;
                } catch (IOException e) {
                    System.out.println("Exception on reading from file: " + filename + " message: " + e.getMessage());
                    configs = null;
                    saveFileName = null;
                }
            }
        }

        if (configs == null) {
            // load the default configuration
            configs = new DieConfiguration[1];
            configs[0] = new DieConfiguration(1, 6, 1, 1,
                    null, true, null);
        }

        configsToView(configs, configBeingEdited);
        //loadEditPanel(diceConfigs.get(configBeingEdited).config.getNumberOfSides() == 1);

        final TextView text = findViewById(R.id.otherText);
        text.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // only change the config if the user is making an edit.  If it is being set to a
                // default value, then ignore the change.
                if (!text.isEnabled()) {
                    return;
                }

                String string = s.toString();
                if (string.isEmpty()) {
                    return;
                }

                DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                int nbrSides;
                if (DieSideConfiguration.isNumeric(string)) {
                    nbrSides = Integer.decode(string);
                } else {
                    nbrSides = DEFAULT_NBR_SIDES;
                }

                if (nbrSides == 0) {
                    nbrSides = DEFAULT_NBR_SIDES;
                }

                cfg.setNumberOfSides(nbrSides);
                editConfig(configBeingEdited, true, false);
                repopulateCurrentListItemFromConfig();
            }

            @Override
            public void afterTextChanged(Editable s) {
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

        instanceState.putInt(Constants.DICE_BEING_EDITED, configBeingEdited);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_customization_menu, menu);
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == Constants.DICE_SIDES_ACTIVITY && resultCode == RESULT_OK) {
            String json = data.getStringExtra(Constants.DICE_SIDES_JSON);
            try {
                JSONArray arr = new JSONArray(json);
                DieSideConfiguration[] sides = DieSideConfiguration.fromJsonArray(arr);
                diceConfigs.get(configBeingEdited).config.setSides(sides);
            } catch (JSONException e) {
                // should not happen
                return;
            }
            repopulateCurrentListItemFromConfig();
        }
    }

    private void repopulateCurrentListItemFromConfig() {
        DiceGuiConfig guiCfg = diceConfigs.get(configBeingEdited);
        DieConfiguration cfg = guiCfg.config;

        // Set the text that specifies the numbers on the die in the format: 1,2,...,6
        LinearLayout layout = findViewById(R.id.dice_list);
        View item = layout.getChildAt(configBeingEdited * 2);
        Button dieConfigButton = item.findViewById(R.id.die_config_button);

        DiceSidesInfo info = getInfoForDieSides(cfg.getNumberOfSides());
        item.setBackground(getResources().getDrawable(info.drawableClicked, null));

        if (cfg.getNumberOfSides() == 1) {
            dieConfigButton.setText(cfg.getSide(0).toString());
        } else {
            String nbrsOnDiceString = String.format(Locale.getDefault(),
                    getString(R.string.diceNumbersString),
                    cfg.getSide(0).toShortString(), cfg.getSide(1).toShortString(),
                    cfg.getSide(cfg.getNumberOfSides() - 1).toShortString());

            // Set the text that specifies the dice being rolled in the format: 1D6
            String diceRepresentation = String.format(Locale.getDefault(),
                    getString(R.string.diceConfigStringRepresentation),
                    cfg.getNumberOfDice(), cfg.getNumberOfSides());

            ArrayList<String> rerollOn = new ArrayList<>();
            for (int i = 0; i < cfg.getNumberOfSides(); i++) {
                DieSideConfiguration side = cfg.getSide(i);
                if (side.reRollOn()) {
                    rerollOn.add(side.toShortString());
                }
            }
            String rerollString = "";
            if (rerollOn.size() > 0) {
                // update the re-roll text
                StringBuilder builder = new StringBuilder();
                int length = rerollOn.size();
                for (String symbol : rerollOn) {
                    builder.append(symbol);
                    if (length > 1) {
                        builder.append(',');
                    }
                    length--;
                }
                rerollString = String.format(Locale.getDefault(), getString(R.string.rerollText),
                        builder.toString());
            }

            dieConfigButton.setText(rerollString + "\n" + nbrsOnDiceString + "\n" + diceRepresentation);
        }
    }

    public void dxClicked(View view) {
        boolean isOtherButton = false;
        if (view.getId() == R.id.otherButton) {
            isOtherButton = true;
        }

        // save the config values from the screen in case we need to load a new edit panel.
        updateConfigFromScreen();

        // next set the number of sides in the config to the new value
        for (int i = 0; i < diceSidesInfos.length; i++) {
            if (view == diceSidesInfos[i].button) {
                diceConfigs.get(configBeingEdited).config.setNumberOfSides(
                        nbrSidesForDiceSidesButton(i));
                break;
            }
        }

        // next load the panel and make sure the panel is updated with the config values.
        editConfig(configBeingEdited, isOtherButton);

        // next reset the dice list item to display the new number of sides.
        repopulateCurrentListItemFromConfig();
    }

    private void loadEditPanel(boolean newPanelIsConstant) {
        LinearLayout layoutEditPanel = findViewById(R.id.edit_panel);
        LinearLayout layoutPanelDie = findViewById(R.id.edit_panel_die);
        LinearLayout layoutPanelConstant = findViewById(R.id.edit_panel_constant);
        if ((layoutPanelConstant != null && !newPanelIsConstant) ||
                (layoutPanelConstant == null && layoutPanelDie == null && !newPanelIsConstant)) {

            // we were editing a constant and now we are editing a die.  Set the panel to the die panel.
            layoutEditPanel.removeAllViews();
            LinearLayout layoutEditPanelDie = (LinearLayout) getLayoutInflater().inflate(
                    R.layout.customization_die, layoutEditPanel, false);
            layoutEditPanel.addView(layoutEditPanelDie);

            EditText text = findViewById(R.id.number_of_dice);
            text.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    String string = s.toString();
                    if (string.isEmpty() || Integer.decode(string) == 0) {
                        // set to the default!
                        cfg.setNumberOfDice(DEFAULT_NBR_DICE);
                    } else {
                        int nbrDice = Integer.decode(string);
                        cfg.setNumberOfDice(nbrDice);
                    }

                    repopulateCurrentListItemFromConfig();
                }

                @Override
                public void afterTextChanged(Editable s) {
                }
            });

            Spinner colorSpinner = findViewById(R.id.die_color);
            ColorArrayAdapter adapter = new ColorArrayAdapter(this,
                    R.array.diceColor, R.layout.list_item_dice_configuration, R.id.list_item);
            colorSpinner.setAdapter(adapter);
        } else if ((layoutPanelDie != null && newPanelIsConstant) ||
                (layoutPanelConstant == null && layoutPanelDie == null && newPanelIsConstant)) {

            // we were editing a die, now we are editing a constant.  Set the panel to the constant
            // edit panel.
            layoutEditPanel.removeAllViews();
            LinearLayout layoutEditPanelDie = (LinearLayout) getLayoutInflater().inflate(
                    R.layout.customization_constant, layoutEditPanel, false);
            layoutEditPanel.addView(layoutEditPanelDie);

            EditText text = findViewById(R.id.constant_symbol);
            text.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;

                    String string = s.toString();
                    DieSideConfiguration side = cfg.getSide(0);
                    if (string.equals(side.symbol())) {
                        // this is called by the value's text getting changed because we changed
                        // it with the code below.  i.e. this is a programatic code change, not
                        // one initiated by the user.  We need this return here to avoid an
                        // infinite loop.
                        return;
                    }
                    side.setSymbol(string);
                    if (DieSideConfiguration.isNumeric(string)) {
                        // is a number - adjust the value as well
                        side.setValue(Integer.decode(string));
                        EditText text1 = findViewById(R.id.constant_value);
                        text1.setText(string);
                    }

                    repopulateCurrentListItemFromConfig();
                }

                @Override
                public void afterTextChanged(Editable s) {
                }
            });

            text = findViewById(R.id.constant_value);
            text.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    DieConfiguration cfg = diceConfigs.get(configBeingEdited).config;
                    DieSideConfiguration side0 = cfg.getSide(0);

                    String string = s.toString();
                    if (string.isEmpty()) {
                        side0.setValue(null);
                    } else if (DieSideConfiguration.isNumeric(string)) {
                        Integer value = Integer.decode(string);
                        if (value.equals(side0.value())) {
                            // this is called by the symbol's text getting changed because we changed
                            // it with the code below.  i.e. this is a programatic code change, not
                            // one initiated by the user.  We need this return here to avoid an
                            // infinite loop.
                            return;
                        }
                        side0.setValue(value);
                        EditText text1 = findViewById(R.id.constant_symbol);
                        if (DieSideConfiguration.isNumeric(text1.getText().toString())) {
                            // the symbol is a number - adjust the symbol to match the value
                            side0.setSymbol(string);
                            text1.setText(string);
                        }
                    }

                    repopulateCurrentListItemFromConfig();
                }

                @Override
                public void afterTextChanged(Editable s) {
                }
            });

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
            DiceSidesInfo info = getInfoForDieSides(cfg.config.getNumberOfSides());
            if (cfg.button == view) {
                ((View)cfg.button.getParent()).setBackground(getResources().getDrawable(info.drawableClicked,null));
                configBeingEdited = i;
                editConfig(i, info == diceSidesInfos[OTHER_BUTTON_INDEX]);
            } else {
                ((View)cfg.button.getParent()).setBackground(getResources().getDrawable(info.drawableUnclicked,null));
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
        DiceGuiConfig cfg = diceConfigs.get(configBeingEdited);
        DiceSidesInfo info = getInfoForDieSides(cfg.config.getNumberOfSides());
        editConfig(configBeingEdited, info == diceSidesInfos[OTHER_BUTTON_INDEX]);
        ((View)cfg.button.getParent()).setBackground(getResources().getDrawable(info.drawableClicked,null));
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
            View diceItem = layoutDiceList.getChildAt(i);
            Button button = diceItem.findViewById(R.id.die_config_button);
            DiceSidesInfo info = getInfoForButton(button);
            diceItem.setBackground(getResources().getDrawable(info.drawableUnclicked,null));
        }

        // first add the divider (which contains the operation...
        LinearLayout layoutDivider = (LinearLayout)inflater.inflate(R.layout.dice_list_divider,
                layoutDiceList, false);

        Spinner operationSpinner = layoutDivider.findViewById(R.id.operation);

        OperationAdapter spinAdapter = new OperationAdapter(this);
        operationSpinner.setAdapter(spinAdapter);
        operationSpinner.setOnItemSelectedListener(this);

        layoutDiceList.addView(layoutDivider);

        // then add the new dice item and set up the fields
        ConstraintLayout layoutNew = (ConstraintLayout) inflater.inflate(R.layout.dice_list_item, layoutDiceList, false);

        Button button = layoutNew.findViewById(R.id.die_config_button);
        DieConfiguration config = new DieConfiguration(diceConfigs.get(diceConfigs.size() -1).config);
        diceConfigs.add(new DiceGuiConfig(config, button));

        // populate the detailed dice config page
        int i = diceConfigs.size() - 1;
        DiceSidesInfo sidesInfo = getInfoForDieSides(diceConfigs.get(i).config.getNumberOfSides());
        layoutNew.setBackground(getResources().getDrawable(sidesInfo.drawableClicked,null));
        layoutDiceList.addView(layoutNew);
        editConfig(diceConfigs.size() - 1, sidesInfo == diceSidesInfos[OTHER_BUTTON_INDEX]);

        // update the dice list item
        repopulateCurrentListItemFromConfig();

        // Scroll to the end
        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
            HorizontalScrollView scrollViewDice = findViewById(R.id.scrollViewDice);
            scrollViewDice.fullScroll(View.FOCUS_RIGHT);
        } else {
            ScrollView scrollViewDice = findViewById(R.id.scrollViewDiceLandscape);
            scrollViewDice.fullScroll(View.FOCUS_DOWN);
        }
    }

    public void onDetailsClick(View view) {
        Intent intent = new Intent(this, DiceSidesActivity.class);
        try {
            DieSideConfiguration[] sides = diceConfigs.get(configBeingEdited).config.getSides();
            JSONArray arr = DieSideConfiguration.toJsonArray(sides);
            intent.putExtra(Constants.DICE_SIDES_JSON, arr.toString());
        } catch (JSONException e) {
            // should not happen!
            return;
        }

        startActivityForResult(intent, Constants.DICE_SIDES_ACTIVITY);
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

        if (pos == OperationAdapter.plusPosition) {
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

        LinearLayout saveDialogView = (LinearLayout) inflater.inflate(R.layout.customization_save_as_dialog, layout, false);

        if (saveFileName != null) {
            EditText text = saveDialogView.findViewById(R.id.filename);
            text.setText(saveFileName);
        }

        saveDialog = new AlertDialog.Builder(this).setTitle(R.string.save).setView(saveDialogView).show();
    }

    public void onCancelDiceCustomization(MenuItem item) {
        setResult(RESULT_CANCELED, null);
        finish();
    }

    public void onRecipesDiceCustomization(MenuItem item) {
        LinearLayout layout = findViewById(R.id.dice_list);
        LayoutInflater inflater = getLayoutInflater();

        LinearLayout recipesView = (LinearLayout) inflater.inflate(
                R.layout.customization_recipes_dialog, layout, false);

        final Spinner spinner = recipesView.findViewById(R.id.spinnerRecipes);
        final StringArrayAdapter adapter = new StringArrayAdapter(this, R.array.recipes,
                R.layout.list_item_dice_configuration, R.id.list_item);
        spinner.setAdapter(adapter);
        final Dialog recipesDialog = new AlertDialog.Builder(this).setTitle(R.string.recipes).setView(recipesView).show();
        Button cancel = recipesView.findViewById(R.id.cancelRecipes);
        cancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                recipesDialog.dismiss();
            }
        });
        Button add = recipesView.findViewById(R.id.addRecipes);
        add.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String selected = spinner.getSelectedItem().toString();
                DieConfiguration[] newDice;
                Resources resources = getResources();
                if (selected.equals(resources.getString(R.string.fudge))) {
                    newDice = DieConfiguration.createFudge();
                } else if (selected.equals(resources.getString(R.string.percentile))) {
                    newDice = DieConfiguration.createPercentile();
                } else {
                    // should not get here... forgot to add new item in config?
                    recipesDialog.dismiss();
                    return;
                }

                DieConfiguration[] allDice = new DieConfiguration[diceConfigs.size() + newDice.length];
                int i = 0;
                for (DiceGuiConfig config : diceConfigs) {
                    allDice[i] = config.config;
                    i++;
                }
                for (DieConfiguration config : newDice) {
                    allDice[i] = config;
                    i++;
                }

                configsToView(allDice, 0);
                recipesDialog.dismiss();
            }
        });

        Button replace = recipesView.findViewById(R.id.replaceRecipes);
        replace.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String selected = spinner.getSelectedItem().toString();
                DieConfiguration[] newDice;
                Resources resources = getResources();
                if (selected.equals(resources.getString(R.string.fudge))) {
                    newDice = DieConfiguration.createFudge();
                } else if (selected.equals(resources.getString(R.string.percentile))) {
                    newDice = DieConfiguration.createPercentile();
                } else {
                    // should not get here... forgot to add new item in config?
                    recipesDialog.dismiss();
                    return;
                }
                configsToView(newDice, 0);
                recipesDialog.dismiss();
            }
        });
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

        String errorMessage = null;
        DiceGroup dice = new DiceGroup(configs);
        try {
            FileOutputStream outputStream = openFileOutput(filename, Context.MODE_PRIVATE);
            dice.saveToFile(outputStream);
            outputStream.close();
        } catch (Exception e) {
            errorMessage = e.getMessage();
            System.out.println("Exception on writing to file: " + filename + " message: " + errorMessage);
        }

        if (errorMessage != null) {
            // failed to save the file.  Pop up a dialog box with error message and do not dismiss
            // the save dialog.
            LinearLayout layout = findViewById(R.id.dice_list);
            LinearLayout errorLayout = (LinearLayout) getLayoutInflater().inflate(
                    R.layout.message_dialog, layout, false);

            TextView error = errorLayout.findViewById(R.id.message);
            error.setText(errorMessage);

            final Dialog errorDialog = new AlertDialog.Builder(this)
                    .setTitle(R.string.error).setView(errorLayout).show();

            Button ok = errorLayout.findViewById(R.id.okButton);
            ok.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    errorDialog.dismiss();
                }
            });
        } else {
            saveDialog.dismiss();
            saveDialog = null;

            setResult(RESULT_OK, null);
            finish();
        }
    }

    private void updateConfigFromScreen() {
        // Repopulate the dice config from the screen in case the user changed the die color.
        if (configBeingEdited >= 0) {
            DieConfiguration config = diceConfigs.get(configBeingEdited).config;

            if (config.getNumberOfSides() != 1) {
                Spinner colorSpinner = findViewById(R.id.die_color);
                ColorArrayAdapter colorAdapter = (ColorArrayAdapter)colorSpinner.getAdapter();
                String selected = colorSpinner.getSelectedItem().toString();
                config.setColor(colorAdapter.getColor(selected));
            }
        }
    }

    private void configsToView(DieConfiguration[] configs, int configToEdit) {
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
                    operation.setSelection(OperationAdapter.plusPosition);
                } else {
                    operation.setSelection(OperationAdapter.minusPosition);
                }
            }

            ConstraintLayout layoutDiceListItem = (ConstraintLayout) inflater.inflate(R.layout.dice_list_item, layout, false);
            Button diceButton = layoutDiceListItem.findViewById(R.id.die_config_button);

            // save all the config buttons so that we can easily find the one being clicked
            // and set it to pressed and set all the other ones to un-pressed.
            diceConfigs.add(new DiceGuiConfig(config, diceButton));
            configBeingEdited = diceConfigs.size() - 1;
            layout.addView(layoutDiceListItem);
            repopulateCurrentListItemFromConfig();

            if (i <= configs.length - 2) {
                // there is at least one more dice config to come, add a divider
                LinearLayout layoutDivider = (LinearLayout) inflater.inflate(R.layout.dice_list_divider, layout, false);
                operation = layoutDivider.findViewById(R.id.operation);
                OperationAdapter spinAdapter = new OperationAdapter(this);
                operation.setAdapter(spinAdapter);
                operation.setOnItemSelectedListener(this);
                layout.addView(layoutDivider);
            }
            i++;
        }

        editConfig(configToEdit, getInfoForDieSides(diceConfigs.get(0).config.getNumberOfSides()) ==
                diceSidesInfos[OTHER_BUTTON_INDEX]);

        // now switch the active dice set.
        i = 0;
        for (DiceGuiConfig cfg : diceConfigs) {
            DiceSidesInfo info = getInfoForDieSides(cfg.config.getNumberOfSides());
            if (i == configToEdit) {
                ((View)cfg.button.getParent()).setBackground(getResources().getDrawable(info.drawableClicked,null));
            } else {
                ((View)cfg.button.getParent()).setBackground(getResources().getDrawable(info.drawableUnclicked,null));
            }
            i++;
        }

    }

    private void editConfig(int i, boolean isOtherButton) {
        editConfig(i, isOtherButton, isOtherButton);
    }

    private void editConfig(int i, boolean isOtherButton, boolean setupOtherButton) {
        configBeingEdited = i;
        DieConfiguration config = diceConfigs.get(i).config;
        boolean newPanelIsConstant = config.getNumberOfSides() == 1;
        loadEditPanel(newPanelIsConstant);

        for (DiceSidesInfo info : diceSidesInfos) {
            info.button.setBackground(getResources().getDrawable(info.drawableUnclicked,null));
        }

        DiceSidesInfo info = getInfoForDieSides(config.getNumberOfSides());
        TextView other = findViewById(R.id.otherText);
        if (isOtherButton) {
            // the "other" button is pressed.  Populate the associated edit text with
            // the number of sides and enable it.
            if (setupOtherButton) {
                other.setText(String.format(Locale.getDefault(), "%d", config.getNumberOfSides()));
                other.setEnabled(true);
                info = diceSidesInfos[OTHER_BUTTON_INDEX];
            }
        } else {
            other.setEnabled(false);
        }

        info.button.setBackground(getResources().getDrawable(info.drawableClicked,null));

        if (newPanelIsConstant) {
            DieSideConfiguration side = config.getSide(0);
            EditText edit = findViewById(R.id.constant_symbol);
            edit.setText(side.symbol());

            edit = findViewById(R.id.constant_value);
            Integer value = side.value();
            if (value != null) {
                edit.setText(String.format(Locale.getDefault(), "%d", value));
            } else {
                edit.setText("");
            }
        } else {
            EditText edit = findViewById(R.id.number_of_dice);
            edit.setText(String.format(Locale.getDefault(), "%d", config.getNumberOfDice()));

            Spinner colorSpinner = findViewById(R.id.die_color);
            ColorArrayAdapter adapter = (ColorArrayAdapter) colorSpinner.getAdapter();
            float[] color = config.getColor();
            int pos = adapter.getPositionForColor(color);
            colorSpinner.setSelection(pos);
        }
    }

    private DiceSidesInfo getInfoForButton(Button button) {
        int length = diceConfigs.size();
        for (int i = 0; i < length; i++) {
            DiceGuiConfig cfg = diceConfigs.get(i);
            if (cfg.button == button) {
                return getInfoForDieSides(cfg.config.getNumberOfSides());
            }
        }
        return null;  // should not happen
    }

    private int nbrSidesForDiceSidesButton(int index) {
        switch (index) {
            case 0:
                return 1;
            case 1:
                return 2;
            case 2:
                return 3;
            case 3:
                return 4;
            case 4:
                return 6;
            case 5:
                return 8;
            case 6:
                return 10;
            case 7:
                return 12;
            case 8:
                return 20;
            case 9:
                return 30;
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

    private DiceSidesInfo getInfoForDieSides(int nbrSides) {
        switch (nbrSides) {
            case 1:
                return diceSidesInfos[0];
            case 2:
                return diceSidesInfos[1];
            case 3:
                return diceSidesInfos[2];
            case 4:
                return diceSidesInfos[3];
            case 6:
                return diceSidesInfos[4];
            case 8:
                return diceSidesInfos[5];
            case 10:
                return diceSidesInfos[6];
            case 12:
                return diceSidesInfos[7];
            case 20:
                return diceSidesInfos[8];
            case 30:
                return diceSidesInfos[9];
            default:
                return diceSidesInfos[10];
        }
    }
}
