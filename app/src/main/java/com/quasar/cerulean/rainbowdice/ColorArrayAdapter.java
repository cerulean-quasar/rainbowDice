package com.quasar.cerulean.rainbowdice;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class ColorArrayAdapter extends BaseAdapter {
    private static final float[] RED = new float[]{1.0f, 0.0f, 0.0f, 1.0f};
    private static final float[] ORANGE = new float[]{1.0f, 0.5f, 0.0f, 1.0f};
    private static final float[] YELLOW = new float[]{1.0f, 1.0f, 0.0f, 1.0f};
    private static final float[] GREEN = new float[]{0.0f, 1.0f, 0.0f, 1.0f};
    private static final float[] BLUE = new float[]{0.0f, 0.0f, 1.0f, 1.0f};
    private static final float[] PURPLE = new float[]{1.0f, 0.0f, 1.0f, 1.0f};
    private static final float[] WHITE = new float[]{1.0f, 1.0f, 1.0f, 1.0f};
    private static final float[] BLACK = new float[]{0.2f, 0.2f, 0.2f, 1.0f};

    private String[] strings;
    private Context ctx;
    private int layout;
    private int textItem;

    public ColorArrayAdapter(Context inctx, int arrayName, int inLayout, int inTextItem) {
        ctx = inctx;
        strings = ctx.getResources().getStringArray(arrayName);
        layout = inLayout;
        textItem = inTextItem;
    }

    public int getPosition(String selected) {
        int i = 0;
        for (String string : strings) {
            if (selected.equals(string)) {
                return i;
            }
            i++;
        }
        return -1;
    }

    private int[] getFgBgColors(String selected) {
        Resources resources = ctx.getResources();

        int [] colors = new int[2];
        if (selected.equals(resources.getString(R.string.red))) {
            colors[1] = Color.RED;
        } else if (selected.equals(resources.getString(R.string.orange))) {
            colors[1] = 0xFFFFCC00;
        } else if (selected.equals(resources.getString(R.string.yellow))) {
            colors[1] = Color.YELLOW;
        } else if (selected.equals(resources.getString(R.string.green))) {
            colors[1] = Color.GREEN;
        } else if (selected.equals(resources.getString(R.string.blue))) {
            colors[1] = Color.BLUE;
        } else if (selected.equals(resources.getString(R.string.purple))) {
            colors[1] = 0xFFFF00FF;
        } else if (selected.equals(resources.getString(R.string.black))) {
            colors[1] = Color.BLACK;
        } else if (selected.equals(resources.getString(R.string.white))) {
            colors[1] = Color.WHITE;
        } else {
            // rainbow
            return null;
        }

        colors[0] = ~colors[1] | 0xFF000000;

        return colors;
    }

    public float[] getColor(String selected) {
        Resources resources = ctx.getResources();

        if (selected.equals(resources.getString(R.string.red))) {
            return RED;
        } else if (selected.equals(resources.getString(R.string.orange))) {
            return ORANGE;
        } else if (selected.equals(resources.getString(R.string.yellow))) {
            return YELLOW;
        } else if (selected.equals(resources.getString(R.string.green))) {
            return GREEN;
        } else if (selected.equals(resources.getString(R.string.blue))) {
            return BLUE;
        } else if (selected.equals(resources.getString(R.string.purple))) {
            return PURPLE;
        } else if (selected.equals(resources.getString(R.string.black))) {
            return BLACK;
        } else if (selected.equals(resources.getString(R.string.white))) {
            return WHITE;
        } else {
            // rainbow
            return  null;
        }
    }

    public int getPositionForColor(float[] color) {
        Resources resources = ctx.getResources();

        if (color == null) {
            return getPosition(resources.getString(R.string.rainbow));
        } else if (compareColor(color, RED)) {
            return getPosition(resources.getString(R.string.red));
        } else if (compareColor(color, ORANGE)) {
            return getPosition(resources.getString(R.string.orange));
        } else if (compareColor(color, YELLOW)) {
            return getPosition(resources.getString(R.string.yellow));
        } else if (compareColor(color, GREEN)) {
            return getPosition(resources.getString(R.string.green));
        } else if (compareColor(color, BLUE)) {
            return getPosition(resources.getString(R.string.blue));
        } else if (compareColor(color, PURPLE)) {
            return getPosition(resources.getString(R.string.purple));
        } else if (compareColor(color, WHITE)) {
            return getPosition(resources.getString(R.string.white));
        } else if (compareColor(color, BLACK)) {
            return getPosition(resources.getString(R.string.black));
        } else {
            // should not happen
            return getPosition(resources.getString(R.string.rainbow));
        }
    }

    private boolean compareColor(float[] color1, float[] color2) {
        if (color1 == null) {
            return color2 == null;
        }

        int length = 4;
        if (color1.length != length || color2.length != length) {
            return false;
        }

        for (int i = 0; i < length; i++) {
            if (color1[i] != color2[i]) {
                return false;
            }
        }

        return true;
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
            LayoutInflater inflater = (LayoutInflater) ctx.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(layout, container, false);
        }

        TextView text = convertView.findViewById(textItem);
        String item = getItem(position);
        int[] colors = getFgBgColors(item);
        if (colors != null) {
            text.setBackgroundColor(colors[1]);
            text.setTextColor(colors[0]);
        }
        text.setText(item);

        return text;
    }

}
