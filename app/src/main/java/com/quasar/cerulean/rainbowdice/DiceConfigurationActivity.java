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

import android.app.Activity;
import android.app.Dialog;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.IBinder;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.TypedValue;
import android.view.DragEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.inputmethod.InputMethodManager;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import java.io.File;
import java.util.LinkedList;
import java.util.Locale;

import static com.quasar.cerulean.rainbowdice.Constants.DICE_CUSTOMIZATION_ACTIVITY;
import static com.quasar.cerulean.rainbowdice.Constants.DICE_FILENAME;

public class DiceConfigurationActivity extends AppCompatActivity {
    DiceConfigurationManager diceConfigManager;
    private class DeleteRequestedInfo {
        public String name;
        public Dialog confirmDialog;
        public int layoutFileLocation;
    }
    DeleteRequestedInfo deleteRequestedInfo = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        diceConfigManager = new DiceConfigurationManager(this);
        String theme = diceConfigManager.getTheme();
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
    }

    @Override
    protected void onResume() {
        super.onResume();

        resetFileList();
    }

    @Override
    protected void onPause() {
        super.onPause();

        diceConfigManager.save();
    }

    private void resetFileList() {
        diceConfigManager = new DiceConfigurationManager(this);
        final LinearLayout diceLayout = findViewById(R.id.dice_layout);
        diceLayout.removeAllViews();
        LayoutInflater inflater = getLayoutInflater();
        LinkedList<String> diceList = diceConfigManager.getDiceList();
        for (String dice : diceList) {
            LinearLayout layout = (LinearLayout) inflater.inflate(R.layout.edit_delete_row, diceLayout, false);
            EditText text = layout.findViewById(R.id.filename);
            text.setText(dice);
            diceLayout.addView(layout);

            final String diceName = dice;
            final Context ctx = this;
            final LinearLayout editDeleteLayout = layout;
            final EditText edit = text;
            ImageButton button = layout.findViewById(R.id.moveDice);
            button.setOnLongClickListener(new View.OnLongClickListener() {
                public boolean onLongClick(View view) {
                    ClipData.Item item = new ClipData.Item(diceName);
                    String[] mimeTypes = new String[1];
                    mimeTypes[0] = ClipDescription.MIMETYPE_TEXT_PLAIN;
                    ClipData data = new ClipData(diceName, mimeTypes, item);

                    view.startDrag(data, new View.DragShadowBuilder(editDeleteLayout), null, 0);
                    return true;
                }
            });
            text.addTextChangedListener(new TextWatcher() {
                public void afterTextChanged(Editable s) {
                }

                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    if (editDeleteLayout.getChildCount() == 4) {
                        ImageButton save = new ImageButton(ctx);
                        save.setBackgroundColor(0);
                        save.setImageDrawable(ctx.getDrawable(R.drawable.save));
                        save.setOnClickListener(new View.OnClickListener() {
                            public void onClick(View view) {
                                String newName = edit.getText().toString();
                                if (newName.length() > 0) {
                                    diceConfigManager.renameDice(diceName, newName);

                                    editDeleteLayout.removeViewAt(5);
                                    editDeleteLayout.removeViewAt(4);

                                    InputMethodManager imm =
                                            (InputMethodManager)ctx.getSystemService(
                                                    Activity.INPUT_METHOD_SERVICE);
                                    IBinder tkn = edit.getWindowToken();
                                    if (tkn != null) {
                                        imm.hideSoftInputFromWindow(tkn, 0);
                                    }
                                }
                            }
                        });
                        editDeleteLayout.addView(save);

                        ImageButton dropChanges = new ImageButton(ctx);
                        dropChanges.setBackgroundColor(0);
                        dropChanges.setImageDrawable(ctx.getDrawable(R.drawable.drop_changes));
                        dropChanges.setOnClickListener(new View.OnClickListener() {
                            public void onClick(View view) {
                                edit.setText(diceName);
                                editDeleteLayout.removeViewAt(5);
                                editDeleteLayout.removeViewAt(4);
                                InputMethodManager imm =
                                        (InputMethodManager)ctx.getSystemService(
                                                Activity.INPUT_METHOD_SERVICE);
                                IBinder tkn = edit.getWindowToken();
                                if (tkn != null) {
                                    imm.hideSoftInputFromWindow(tkn, 0);
                                }
                            }
                        });
                        editDeleteLayout.addView(dropChanges);
                    }
                }
            });
        }

        diceLayout.setOnDragListener(new View.OnDragListener() {
            public boolean onDrag(View view, DragEvent event) {
                int eventType = event.getAction();
                switch (eventType) {
                    case DragEvent.ACTION_DRAG_STARTED:
                        if (event.getClipDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)) {
                            return true;
                        }
                        break;
                    case DragEvent.ACTION_DRAG_LOCATION:
                    case DragEvent.ACTION_DRAG_ENTERED:
                    case DragEvent.ACTION_DRAG_EXITED:
                    case DragEvent.ACTION_DRAG_ENDED:
                        return true;
                    case DragEvent.ACTION_DROP:
                        ClipData.Item item = event.getClipData().getItemAt(0);
                        String text = item.getText().toString();
                        LinearLayout diceItem;
                        int dragAt = -1;
                        LinearLayout draggedItem = null;
                        int draggedItemAt = -1;

                        float y = event.getY();

                        for (int i = 0; i < diceLayout.getChildCount(); i++) {
                            diceItem = (LinearLayout) diceLayout.getChildAt(i);
                            EditText editText = diceItem.findViewById(R.id.filename);
                            String text2 = editText.getText().toString();
                            if (text.equals(text2)) {
                                draggedItem = diceItem;
                                draggedItemAt = i;
                            }
                            if (dragAt == -1 && y < diceItem.getY()) {
                                dragAt = i - 1;
                            }

                            if (draggedItem != null && dragAt != -1) {
                                break;
                            }
                        }

                        if (draggedItem == null) {
                            return false;
                        }

                        if (draggedItemAt == dragAt) {
                            // the item didn't move.  just return false
                            return false;
                        }
                        diceConfigManager.moveDice(text, dragAt);
                        if (draggedItemAt < dragAt) {
                            dragAt -= 1;
                        }

                        if (dragAt == -1) {
                            dragAt = diceLayout.getChildCount();
                        }

                        diceLayout.removeView(draggedItem);
                        diceLayout.addView(draggedItem, dragAt);
                        return true;
                }
                return false;
            }
        });
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

            resetFileList();
        }
    }

    private int getDicePositionForRowContainingView(View view, int rid) {
        // find the list item that created the event
        LinearLayout layout = findViewById(R.id.dice_layout);
        for (int i = 0; i < layout.getChildCount(); i++) {
            LinearLayout item = (LinearLayout) layout.getChildAt(i);
            View edit = item.findViewById(rid);
            if (edit == view) {
                // this is the row that caused the event
                return i;
            }
        }

        return -1;
    }

    private EditText getDiceViewForRowContainingView(View view, int rid) {
        // find the list item that created the event
        LinearLayout layout = findViewById(R.id.dice_layout);
        for (int i = 0; i < layout.getChildCount(); i++) {
            LinearLayout item = (LinearLayout) layout.getChildAt(i);
            ImageButton edit = item.findViewById(rid);
            if (edit == view) {
                // this is the row that caused the event
                EditText text = item.findViewById(R.id.filename);
                return text;
            }
        }

        return null;
    }

    public void onDeleteFile(View view) {
        // find the list item that created the delete event
        int i = getDicePositionForRowContainingView(view, R.id.trash);
        EditText text = getDiceViewForRowContainingView(view, R.id.trash);
        if (text == null) {
            return;
        }

        String name = text.getText().toString();
        if (name.isEmpty()) {
            return;
        }

        LinearLayout layout = findViewById(R.id.dice_layout);
        LinearLayout confirmLayout = (LinearLayout) getLayoutInflater().inflate(R.layout.configuration_confirm_dialog, layout, false);
        deleteRequestedInfo = new DeleteRequestedInfo();
        deleteRequestedInfo.name = name;
        deleteRequestedInfo.layoutFileLocation = i;
        String confirmMessage = String.format(Locale.getDefault(), getString(R.string.confirm), name);
        deleteRequestedInfo.confirmDialog = new AlertDialog.Builder(this).setTitle(confirmMessage).setView(confirmLayout).show();
    }

    public void onDeleteDiceConfig(View view) {
        // delete the file
        LinearLayout layout = findViewById(R.id.dice_layout);
        layout.removeViewAt(deleteRequestedInfo.layoutFileLocation);
        diceConfigManager.deleteDice(deleteRequestedInfo.name);
        deleteRequestedInfo.confirmDialog.dismiss();
        deleteRequestedInfo = null;
    }

    public void onCancelDeleteDiceConfig(View view) {
        deleteRequestedInfo.confirmDialog.dismiss();
        deleteRequestedInfo = null;
    }

    public void onEdit(View view) {
        // find the list item that created the edit event
        EditText text = getDiceViewForRowContainingView(view, R.id.editDice);
        if (text == null) {
            return;
        }

        String filename = text.getText().toString();

        // start the customization app with the file
        if (!filename.isEmpty()) {
            Intent intent = new Intent(this, DiceCustomizationActivity.class);
            intent.putExtra(DICE_FILENAME, filename);
            startActivityForResult(intent, DICE_CUSTOMIZATION_ACTIVITY);
        }
    }

    public void onDone(MenuItem item) {
        diceConfigManager.save();

        setResult(RESULT_OK, null);
        finish();
    }

    public void onNewDiceConfiguration(MenuItem item) {
        diceConfigManager.save();
        Intent intent = new Intent(this, DiceCustomizationActivity.class);
        startActivityForResult(intent, DICE_CUSTOMIZATION_ACTIVITY);
    }
}
