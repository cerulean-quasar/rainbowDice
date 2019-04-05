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
    private String[] strings;
    private Context ctx;
    private int layout;
    private int textItem;
    private int[] fgBgColors;

    public ColorArrayAdapter(Context inctx, int arrayName, int inLayout, int inTextItem) {
        ctx = inctx;
        strings = ctx.getResources().getStringArray(arrayName);
        layout = inLayout;
        textItem = inTextItem;
        fgBgColors = null;
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
            return Constants.RED;
        } else if (selected.equals(resources.getString(R.string.orange))) {
            return Constants.ORANGE;
        } else if (selected.equals(resources.getString(R.string.yellow))) {
            return Constants.YELLOW;
        } else if (selected.equals(resources.getString(R.string.green))) {
            return Constants.GREEN;
        } else if (selected.equals(resources.getString(R.string.blue))) {
            return Constants.BLUE;
        } else if (selected.equals(resources.getString(R.string.purple))) {
            return Constants.PURPLE;
        } else if (selected.equals(resources.getString(R.string.black))) {
            return Constants.BLACK;
        } else if (selected.equals(resources.getString(R.string.white))) {
            return Constants.WHITE;
        } else {
            // rainbow
            return  null;
        }
    }

    public int getPositionForColor(float[] color) {
        Resources resources = ctx.getResources();

        if (color == null) {
            return getPosition(resources.getString(R.string.rainbow));
        } else if (compareColor(color, Constants.RED)) {
            return getPosition(resources.getString(R.string.red));
        } else if (compareColor(color, Constants.ORANGE)) {
            return getPosition(resources.getString(R.string.orange));
        } else if (compareColor(color, Constants.YELLOW)) {
            return getPosition(resources.getString(R.string.yellow));
        } else if (compareColor(color, Constants.GREEN)) {
            return getPosition(resources.getString(R.string.green));
        } else if (compareColor(color, Constants.BLUE)) {
            return getPosition(resources.getString(R.string.blue));
        } else if (compareColor(color, Constants.PURPLE)) {
            return getPosition(resources.getString(R.string.purple));
        } else if (compareColor(color, Constants.WHITE)) {
            return getPosition(resources.getString(R.string.white));
        } else if (compareColor(color, Constants.BLACK)) {
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
        // Set the background color to the color being selected except if it is rainbow being selected.
        // For rainbow, do not set the color if the TextView is being created.  If it is being reused,
        // set it to the default text color and for the background, use the color opposite of the
        // text color.
        String item = getItem(position);
        int[] colors = getFgBgColors(item);

        TextView text;
        if (convertView == null) {
            LayoutInflater inflater = (LayoutInflater) ctx.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            convertView = inflater.inflate(layout, container, false);
            text = convertView.findViewById(textItem);
            if (fgBgColors == null) {
                fgBgColors = new int[2];
                fgBgColors[0] = text.getTextColors().getDefaultColor();
                fgBgColors[1] = ~fgBgColors[0] | 0xFF000000;
            }
        } else {
            text = convertView.findViewById(textItem);
            if (colors == null) {
                colors = fgBgColors;
            }
        }

        if (colors != null) {
            text.setBackgroundColor(colors[1]);
            text.setTextColor(colors[0]);
        }
        text.setText(item);

        return text;
    }

}
