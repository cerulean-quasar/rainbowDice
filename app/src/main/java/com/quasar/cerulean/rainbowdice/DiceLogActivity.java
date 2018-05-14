package com.quasar.cerulean.rainbowdice;

import android.content.res.Configuration;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.TextView;

public class DiceLogActivity extends AppCompatActivity {
    private LogFile log;

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
            TypedValue value = new TypedValue();
            getTheme().resolveAttribute(R.attr.background_landscape, value, true);
            getWindow().setBackgroundDrawableResource(value.resourceId);
        }

        setContentView(R.layout.activity_dice_log);

        log = new LogFile(this);

        LinearLayout layout = findViewById(R.id.dice_log_list);
        LayoutInflater inflater = getLayoutInflater();
        for (int i = 0; i < log.size(); i++) {
            LogFile.LogItem item = log.get(i);
            LinearLayout layoutItem = (LinearLayout)inflater.inflate(R.layout.dice_log_item, layout, false);

            TextView text = layoutItem.findViewById(R.id.dice_log_time);
            text.setText(item.logTime);

            text = layoutItem.findViewById(R.id.dice_log_name);
            text.setText(item.diceName);

            text = layoutItem.findViewById(R.id.dice_log_representation);
            text.setText(item.diceRepresentation);

            text = layoutItem.findViewById(R.id.dice_log_result);
            text.setText(item.rollResults);

            layout.addView(layoutItem);
        }
    }
}
