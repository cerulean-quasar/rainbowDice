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
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
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

    private static class MyDragShadowBuilder extends View.DragShadowBuilder {

        // The drag shadow image, defined as a drawable thing
        private static Drawable shadow;

        // Defines the constructor for myDragShadowBuilder
        public MyDragShadowBuilder(View v) {

            // Stores the View parameter passed to myDragShadowBuilder.
            super(v);

            // Creates a draggable image that will fill the Canvas provided by the system.
            shadow = new ColorDrawable(Color.LTGRAY);
        }

        // Defines a callback that sends the drag shadow dimensions and touch point back to the
        // system.
        @Override
        public void onProvideShadowMetrics (Point size, Point touch) {
            // Defines local variables
            int width, height;

            width = getView().getWidth();
            height = getView().getHeight();

            // The drag shadow is a ColorDrawable. This sets its dimensions to be the same as the
            // Canvas that the system will provide. As a result, the drag shadow will fill the
            // Canvas.
            shadow.setBounds(0, 0, width, height);

            // Sets the size parameter's width and height values. These get back to the system
            // through the size parameter.
            size.set(width, height);

            // Sets the touch point's position to be in the middle of the drag shadow in y and at
            // the beginning in x
            touch.set(0, height/2);
        }

        // Defines a callback that draws the drag shadow in a Canvas that the system constructs
        // from the dimensions passed in onProvideShadowMetrics().
        @Override
        public void onDrawShadow(Canvas canvas) {

            // Draws the ColorDrawable in the Canvas passed in from the system.
            shadow.draw(canvas);
        }
    }

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
        LinearLayout layout = findViewById(R.id.dice_layout);
        InputMethodManager imm = (InputMethodManager)getSystemService(Activity.INPUT_METHOD_SERVICE);
        IBinder tkn = layout.getWindowToken();
        if (tkn != null && imm != null) {
            imm.hideSoftInputFromWindow(tkn, 0);
        }
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
        boolean focusRequested = false;
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
            if (!focusRequested) {
                button.requestFocus();
                focusRequested = true;
            }
            button.setOnLongClickListener(new View.OnLongClickListener() {
                public boolean onLongClick(View view) {
                    String rowText = null;
                    for (int i = 0; i < diceLayout.getChildCount(); i++) {
                        LinearLayout diceItem = (LinearLayout)diceLayout.getChildAt(i);
                        ImageButton upDown = diceItem.findViewById(R.id.moveDice);
                        if (upDown == view) {
                            EditText dice = diceItem.findViewById(R.id.filename);
                            if (dice == null) {
                                return false;
                            }
                            rowText = dice.getText().toString();
                            if (rowText.length() == 0) {
                                return false;
                            }
                        }
                    }
                    if (rowText == null) {
                        return false;
                    }
                    ClipData.Item item = new ClipData.Item(rowText);
                    String[] mimeTypes = new String[1];
                    mimeTypes[0] = ClipDescription.MIMETYPE_TEXT_PLAIN;
                    ClipData data = new ClipData(diceName, mimeTypes, item);

                    view.startDrag(data, new MyDragShadowBuilder(editDeleteLayout), null, 0);
                    return true;
                }
            });

            text.setOnDragListener(null);
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
                                if (tkn != null && imm != null) {
                                    imm.hideSoftInputFromWindow(tkn, 0);
                                }
                            }
                        });
                        editDeleteLayout.addView(dropChanges);
                    }
                }
            });
        }

        final Context ctx = this;
        diceLayout.setOnDragListener(new View.OnDragListener() {
            ImageView dropDisplay = null;
            int dropDisplayPosition = -1;
            View draggedItem = null;
            int draggedItemPosition = -1;
            boolean dropped = false;

            public boolean onDrag(View view, DragEvent event) {
                int eventType = event.getAction();
                switch (eventType) {
                    case DragEvent.ACTION_DRAG_ENTERED:
                    case DragEvent.ACTION_DRAG_STARTED:
                        dropDisplay = new ImageView(ctx);
                        TypedValue value = new TypedValue();
                        getTheme().resolveAttribute(R.attr.divider, value, true);
                        dropDisplay.setImageDrawable(getResources().getDrawable(value.resourceId,null));

                        if (event.getClipDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)) {
                            // remove the item from the layout so it looks like it is being dragged around
                            return true;
                        }
                        break;
                    case DragEvent.ACTION_DRAG_LOCATION:
                        // move the dropDisplay to where the user has their finger so they know
                        // that if they release here, that is where the view will be dropped into.
                        int childCount = diceLayout.getChildCount();
                        for (int i = 0; i < childCount; i++) {
                            View diceItem = diceLayout.getChildAt(i);
                            float y = event.getY();
                            float diceItemY = diceItem.getY();
                            float diceItemH = diceItem.getHeight();
                            if (y >= diceItemY && y <= diceItemY + diceItemH) {
                                if (draggedItem == null) {
                                    draggedItem = diceLayout.getChildAt(i);
                                    draggedItemPosition = i;
                                    diceLayout.removeViewAt(i);
                                    diceLayout.addView(dropDisplay, i);
                                    dropDisplayPosition = i;
                                    return true;
                                }

                                if (y > diceItemY + diceItemH/2) {
                                    i++;
                                }
                                if (dropDisplayPosition >= 0 &&
                                        (view == dropDisplay || i == dropDisplayPosition + 1)) {
                                    return true;
                                } else if (i == dropDisplayPosition) {
                                    /* do nothing - the dropDisplayPosition is already at this position */
                                    return true;
                                } else {
                                    if (dropDisplayPosition >= 0) {
                                        diceLayout.removeViewAt(dropDisplayPosition);
                                        if (dropDisplayPosition < i) {
                                            i--;
                                        }
                                    }
                                    diceLayout.addView(dropDisplay, i);
                                    dropDisplayPosition = i;
                                }
                                return true;
                            }
                        }
                        // trying to move to the last position...
                        if (dropDisplayPosition >= 0) {
                            diceLayout.removeViewAt(dropDisplayPosition);
                        }
                        dropDisplayPosition = diceLayout.getChildCount();
                        diceLayout.addView(dropDisplay, dropDisplayPosition);
                        return true;
                    case DragEvent.ACTION_DRAG_EXITED:
                        diceLayout.removeViewAt(dropDisplayPosition);
                        dropDisplayPosition = -1;
                        break;
                    case DragEvent.ACTION_DRAG_ENDED:
                        if (!dropped) {
                            if (dropDisplayPosition >= 0) {
                                diceLayout.removeViewAt(dropDisplayPosition);
                            }
                            if (draggedItem != null) {
                                diceLayout.addView(draggedItem, draggedItemPosition);
                            }
                            dropDisplay = null;
                            dropDisplayPosition = -1;
                            draggedItem = null;
                            draggedItemPosition = -1;
                        }
                        dropped = false;
                        return true;
                    case DragEvent.ACTION_DROP:
                        if (draggedItem == null) {
                            return false;
                        }

                        ClipData.Item item = event.getClipData().getItemAt(0);
                        String text = item.getText().toString();

                        // the view was removed from the layout but is still in the config list
                        // therefore, if you want to move the dice in the config list and you are
                        // moving the dice down, then you need to add 1 back into the position that
                        // you pass to the moveDice list operation.
                        if (draggedItemPosition < dropDisplayPosition) {
                            diceConfigManager.moveDice(text, dropDisplayPosition+1);
                        } else {
                            diceConfigManager.moveDice(text, dropDisplayPosition);
                        }

                        diceLayout.addView(draggedItem, dropDisplayPosition);

                        diceLayout.removeView(dropDisplay);
                        dropDisplay = null;
                        dropDisplayPosition = -1;
                        draggedItem = null;
                        draggedItemPosition = -1;
                        dropped = true;
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
