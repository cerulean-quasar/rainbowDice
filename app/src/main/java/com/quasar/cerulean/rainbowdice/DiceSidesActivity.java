package com.quasar.cerulean.rainbowdice;

import android.content.Intent;
import android.os.Parcelable;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import org.json.JSONArray;
import org.json.JSONException;

public class DiceSidesActivity extends AppCompatActivity {
    private static final int NBR_COLUMNS = 5;
    RecyclerView m_recyclerDiceSides;
    RecyclerView.LayoutManager m_layoutManager;
    DiceSidesAdapter m_adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        DiceConfigurationManager configurationFile = new DiceConfigurationManager(this);
        String themeName = configurationFile.getTheme();
        if (themeName == null || themeName.isEmpty()) {
            themeName = "Space";
        }
        int currentThemeId = getResources().getIdentifier(themeName, "style", getPackageName());

        setTheme(currentThemeId);
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_dice_sides);

        DieSideConfiguration[] sides = null;
        if (savedInstanceState != null) {
            Parcelable[] parcelableConfigs = savedInstanceState.getParcelableArray(Constants.DICE_SIDE_CONFIGURATION);
            if (parcelableConfigs != null) {
                sides = new DieSideConfiguration[parcelableConfigs.length];
                int i=0;
                for (Parcelable parcelableConfig : parcelableConfigs) {
                    sides[i++] = (DieSideConfiguration) parcelableConfig;
                }
            }
        }

        if (sides == null) {
            Intent intent = getIntent();
            String jsonDiceSides = intent.getStringExtra(Constants.DICE_SIDES_JSON);
            try {
                JSONArray arr = new JSONArray(jsonDiceSides);
                sides = DieSideConfiguration.fromJsonArray(arr);
            } catch (JSONException e) {
                // should not hit here... fail out.
                setResult(RESULT_CANCELED);
                finish();
                return;
            }
        }

        m_recyclerDiceSides = findViewById(R.id.diceSidesContainer);

        // use this setting to improve performance if you know that changes
        // in content do not change the layout size of the RecyclerView
        m_recyclerDiceSides.setHasFixedSize(true);

        m_layoutManager = new GridLayoutManager(this, NBR_COLUMNS);

        m_adapter = new DiceSidesAdapter(m_layoutManager, sides, NBR_COLUMNS);

        m_recyclerDiceSides.setLayoutManager(m_layoutManager);
        m_recyclerDiceSides.setAdapter(m_adapter);
    }

    @Override
    public void onSaveInstanceState(Bundle instanceState) {
        super.onSaveInstanceState(instanceState);

        DieSideConfiguration[] sides = m_adapter.getDieSideConfigurations();
        instanceState.putParcelableArray(Constants.DICE_SIDE_CONFIGURATION, sides);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.dice_sides_menu, menu);
        return true;
    }

    public void onCancelDiceSides(MenuItem menuItem) {
        setResult(RESULT_CANCELED);
        finish();
    }

    public void onSaveDiceSides(MenuItem menuItem) {
        Intent intent = new Intent();
        DieSideConfiguration[] sides = m_adapter.getDieSideConfigurations();

        try {
            JSONArray arr = DieSideConfiguration.toJsonArray(sides);
            String json = arr.toString();
            intent.putExtra(Constants.DICE_SIDES_JSON, json);
            setResult(RESULT_OK, intent);
        } catch (JSONException e) {
            // should not get here
            setResult(RESULT_CANCELED);
        }

        finish();
    }
}
