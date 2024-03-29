package com.quasar.cerulean.rainbowdice;

import android.content.Intent;
import android.content.res.Configuration;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

public class DiceLogActivity extends AppCompatActivity {
    private LogFile log;
    private View selectedView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        ConfigurationFile configurationFile = new ConfigurationFile(this);
        String themeName = configurationFile.getTheme();
        if (themeName == null || themeName.isEmpty()) {
            themeName = "Space";
        }
        int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());
        setTheme(currentThemeId);

        super.onCreate(savedInstanceState);

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        }

        setContentView(R.layout.activity_dice_log);

        log = new LogFile(this);

        LinearLayout layout = findViewById(R.id.dice_log_list);
        LayoutInflater inflater = getLayoutInflater();
        for (int i = log.size()-1; i >= 0 ; i--) {
            LogFile.LogItem item = log.get(i);
            LinearLayout layoutItem = (LinearLayout)inflater.inflate(R.layout.dice_log_item, layout, false);

            TextView text = layoutItem.findViewById(R.id.dice_log_time);
            text.setText(item.getLogTime());

            text = layoutItem.findViewById(R.id.dice_log_name);
            text.setText(item.getDiceName());

            text = layoutItem.findViewById(R.id.dice_log_representation);
            text.setText(item.getDiceRepresentation().replace('\n', ' '));

            text = layoutItem.findViewById(R.id.dice_log_result);
            text.setText(item.getRollResultsString());

            layout.addView(layoutItem);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_log_menu, menu);
        return true;
    }

    public void onLogItemClicked(View view) {
        if (selectedView == view) {
            view.setBackgroundColor(0x00000000);
            selectedView = null;
            return;
        }

        if (selectedView != null) {
            selectedView.setBackgroundColor(0x00000000);
        }
        view.setBackgroundColor(0x500f0fff);
        selectedView = view;
    }

    public void onLogLoad(MenuItem item) {
        if (selectedView == null) {
            setResult(RESULT_CANCELED, null);
            return;
        }

        LinearLayout diceListLayout = findViewById(R.id.dice_log_list);
        int nbrItems = diceListLayout.getChildCount();
        int i;
        for (i = 0; i < nbrItems; i++) {
            if (selectedView == diceListLayout.getChildAt(i)) {
                break;
            }
        }

        if (i >= nbrItems) {
            setResult(RESULT_CANCELED);
            return;
        }

        Intent data = new Intent();
        data.putExtra(Constants.LOG_ITEM_TO_LOAD, nbrItems - i - 1);
        setResult(RESULT_OK, data);
        finish();
    }
    public void onLogExit(MenuItem item) {
        setResult(RESULT_CANCELED, null);
        finish();
    }
}
