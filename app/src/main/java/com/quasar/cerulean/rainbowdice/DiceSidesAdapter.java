package com.quasar.cerulean.rainbowdice;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spanned;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;

public class DiceSidesAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    // the item view type is the same as the column number except for the header.
    private static final int REPEATS_VIEW_TYPE = 0;
    private static final int SYMBOL_VIEW_TYPE = 1;
    private static final int NOVALUE_VIEW_TYPE = 2;
    private static final int VALUE_VIEW_TYPE = 3;
    private static final int REROLL_VIEW_TYPE = 4;
    private static final int HEADER_VIEW_TYPE = 5;
    private static final int HEADER_NOVALUE_VIEW_TYPE = 6;

    private static final int MAX_TEXT_LENGTH = 20;
    private static final int MAX_TEXT_LENGTH_NUMBER = 9;

    private RecyclerView.LayoutManager m_manager;
    private int m_nbrColumns;

    private class DieSides {
        public DieSideConfiguration side;
        public int repeatNumber;
        public DieSides(DieSideConfiguration inSide, int inRepeatNumber) {
            side = inSide;
            repeatNumber = inRepeatNumber;
        }
    }
    private ArrayList<DieSides> m_sides;
    private final int m_totalDiceSides;

    private class ViewHolderHeader extends RecyclerView.ViewHolder {
        public TextView m_textHeader;
        public ViewHolderHeader(TextView view) {
            super(view);
            m_textHeader = view;
        }
    }

    private class ViewHolderHeaderNoValue extends RecyclerView.ViewHolder {
        public Button m_noValue;
        public ViewHolderHeaderNoValue(Button view) {
            super(view);
            m_noValue = view;
        }
    }

    private class ViewHolderRepeats extends RecyclerView.ViewHolder {
        public Spinner m_repeats;
        public ViewHolderRepeats(Spinner view) {
            super(view);
            m_repeats = view;
        }
    }

    private class ViewHolderSymbol extends RecyclerView.ViewHolder {
        public EditText m_symbol;
        public ViewHolderSymbol(EditText view) {
            super(view);
            m_symbol = view;
        }
    }

    private class ViewHolderNoValue extends RecyclerView.ViewHolder {
        public Button m_noValue;
        public ViewHolderNoValue(Button view) {
            super(view);
            m_noValue = view;
        }
    }

    private class ViewHolderValue extends RecyclerView.ViewHolder {
        public EditText m_value;
        public ViewHolderValue(EditText view) {
            super(view);
            m_value = view;
        }
    }

    private class ViewHolderReRoll extends RecyclerView.ViewHolder {
        public CheckBox m_reRoll;
        public ViewHolderReRoll(CheckBox view) {
            super(view);
            m_reRoll = view;
        }
    }

    private class RepeatAdapter extends BaseAdapter {
        int m_max;
        int m_layout;
        int m_textItem;

        RepeatAdapter(int max, int inLayout, int inTextItem) {
            m_max = max;
            m_layout = inLayout;
            m_textItem = inTextItem;
        }

        public void setMax(int max) {
            m_max = max;
        }

        @Override
        public int getCount() {
            return m_max;
        }

        @Override
        public String getItem(int position) {
            return String.format(Locale.getDefault(), "%d", position+1);
        }

        @Override
        public long getItemId(int position) {
            return Integer.valueOf(position).hashCode();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup container) {
            if (convertView == null) {
                LayoutInflater inflater = (LayoutInflater) container.getContext().getSystemService(
                        Context.LAYOUT_INFLATER_SERVICE);
                convertView = inflater.inflate(m_layout, container, false);
            }

            TextView text = convertView.findViewById(m_textItem);
            text.setText(getItem(position));

            return text;
        }

    }

    private class RepeatsSelectedListener implements AdapterView.OnItemSelectedListener {
        private RecyclerView.ViewHolder m_vhRepeats;
        public RepeatsSelectedListener(RecyclerView.ViewHolder vhRepeats) {
            m_vhRepeats = vhRepeats;
        }
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            int repeat = position + 1;
            int i = getDiceSidesIndex(m_vhRepeats);
            DieSides side = m_sides.get(i);

            if (repeat > side.repeatNumber) {
                int k = m_sides.size() - 1;
                for (int j = side.repeatNumber; j < repeat; j++) {
                    DieSides sideToChange = m_sides.get(k);
                    int repeatNumber = sideToChange.repeatNumber - 1;

                    if (repeatNumber == 0) {
                        m_sides.remove(k);
                    } else {
                        sideToChange.repeatNumber = repeatNumber;
                    }

                    for (int l = 0; l < m_nbrColumns; l++) {
                        notifyItemRemoved(getPositionForRC(k, l));
                    }

                    if (repeatNumber == 0) {
                        k--;
                    }
                }
                side.repeatNumber = repeat;
                notifyDataSetChanged();
            } else if (repeat < side.repeatNumber) {
                int k = m_sides.size();
                DieSides lastSide = m_sides.get(k-1);
                Integer value = lastSide.side.value();
                String symbol = lastSide.side.symbol();
                boolean valueEqualsSymbol = lastSide.side.valueEqualsSymbol();
                for (int j = repeat; j < side.repeatNumber; j++) {
                    if (valueEqualsSymbol) {
                        value++;
                        symbol = String.format(Locale.getDefault(), "%d", value);
                    }
                    m_sides.add(k++, new DieSides(new DieSideConfiguration(symbol, value, false), 1));
                    for (int l = 0; l < m_nbrColumns; l++) {
                        notifyItemInserted(getPositionForRC(k, l));
                    }
                }
                side.repeatNumber = repeat;
                notifyDataSetChanged();
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
            // This function should never get called.
        }
    }

    public DiceSidesAdapter(RecyclerView.LayoutManager manager, DieSideConfiguration[] sides, int nbrColumns) {
        m_totalDiceSides = sides.length;
        m_manager = manager;
        m_nbrColumns = nbrColumns;
        m_sides = new ArrayList<>();
        for (int i=0; i < sides.length; ) {
            int j = i + 1;
            for (;j < sides.length; j++) {
                if (!sides[i].equals(sides[j])) {
                    break;
                }
            }
            DieSides sideWithRepeats = new DieSides(new DieSideConfiguration(sides[i]), j - i);
            m_sides.add(sideWithRepeats);
            i = j;
        }
    }

    // get the index into m_sides given a ViewHolder.
    private int getDiceSidesIndex(RecyclerView.ViewHolder vh) {
        return vh.getAdapterPosition() / m_nbrColumns - 1;
    }

    // get the position in the layout given a view holder for the row and a column number (starts at 0)
    private int getPositionForRC(RecyclerView.ViewHolder vh, int col) {
        return (getDiceSidesIndex(vh)+1)*m_nbrColumns + col;
    }
    private int getPositionForRC(int row, int col) {
        return (row+1)*m_nbrColumns + col;
    }

    public DieSideConfiguration[] getDieSideConfigurations() {
        DieSideConfiguration[] sides = new DieSideConfiguration[m_totalDiceSides];

        int i = 0;
        for (DieSides sidesWithRepeat : m_sides) {
            for (int j = 0; j < sidesWithRepeat.repeatNumber; j++) {
                sides[i++] = new DieSideConfiguration(sidesWithRepeat.side);
            }
        }

        return sides;
    }

    private Integer getIncrementingValue() {
        Integer value;
        Integer prevValue = null;
        Integer increment = null;
        for (DieSides side : m_sides) {
            value = side.side.value();
            if (value == null) {
                return null;
            }

            if (prevValue != null) {
                if (increment == null) {
                    increment = value - prevValue;
                } else if (increment != value - prevValue) {
                    return null;
                }
            }
            prevValue = value;
        }

        return increment;
    }

    private boolean maxHaveRerollChecked() {
        int count = 0;
        for (DieSides side : m_sides) {
            if (side.side.reRollOn()) {
                count++;
            }
        }

        return count >= m_sides.size()/2;
    }

    private boolean symbolsMatchesValues(boolean ignoreEmptySymbols) {
        for (DieSides side : m_sides) {
            if (ignoreEmptySymbols && side.side.symbol().isEmpty() && side.side.value() != null) {
                continue;
            }

            if (!side.side.valueEqualsSymbol()) {
                return false;
            }
        }

        return true;
    }

    @Override
    public int getItemViewType(int position) {
        // the first row is the header row and use the HEADER_VIEW_TYPE item view type, for all
        // others, the item view type is the column number.  The no-value column header uses
        // ViewHolderNoValue holder as well as the rest of the column, because we want to be able to
        // click on it and have all of the values clear.
        int col = position % m_nbrColumns;
        if (position < m_nbrColumns) {
            if (col == NOVALUE_VIEW_TYPE) {
                return HEADER_NOVALUE_VIEW_TYPE;
            } else {
                return HEADER_VIEW_TYPE;
            }
        } else {
            return col;
        }
    }

    @Override @NonNull
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        switch (viewType) {
            case REPEATS_VIEW_TYPE:
                Spinner repeatsView = new Spinner(parent.getContext());
                RepeatAdapter adapter = new RepeatAdapter(m_sides.size(),
                        R.layout.list_item_dice_configuration, R.id.list_item);
                repeatsView.setAdapter(adapter);
                final ViewHolderRepeats vhRepeats = new ViewHolderRepeats(repeatsView);
                repeatsView.setOnItemSelectedListener(new RepeatsSelectedListener(vhRepeats));
                return vhRepeats;
            case SYMBOL_VIEW_TYPE:
                final EditText symbolView = new EditText(parent.getContext());
                final ViewHolderSymbol vhSymbol =  new ViewHolderSymbol(symbolView);
                InputFilter[] filters = new InputFilter[1];
                filters[0] = new InputFilter.LengthFilter(MAX_TEXT_LENGTH);
                symbolView.setFilters(filters);
                symbolView.addTextChangedListener(new TextWatcher() {
                    boolean numberMatches = false;
                    Integer previouslySeenIncrement = null;

                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                        int i = getDiceSidesIndex(vhSymbol);
                        numberMatches = m_sides.get(i).side.valueEqualsSymbol();

                        Integer increment = getIncrementingValue();
                        if (previouslySeenIncrement == null) {
                            previouslySeenIncrement = increment;
                        } else if (increment != null) {
                            previouslySeenIncrement = increment;
                        }
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        int i = getDiceSidesIndex(vhSymbol);
                        DieSideConfiguration side = m_sides.get(i).side;
                        String symbol = s.toString();
                        if (symbol.equals(side.symbol())) {
                            // no change made, simply return.
                            return;
                        }

                        // the only reason we would change a value when the symbol changes is if
                        // each side value matches its corresponding symbol
                        boolean isNewSymbolNumeric = DieSideConfiguration.isNumeric(symbol);
                        if ((side.symbol().length() == 0 && side.value() != null && isNewSymbolNumeric) ||
                                (numberMatches && isNewSymbolNumeric)) {
                            side.setValue(Integer.valueOf(symbol));
                            int pos = getPositionForRC(vhSymbol, VALUE_VIEW_TYPE);
                            notifyItemChanged(pos);
                            side.setSymbol(symbol);

                            boolean symbolsMatchValues = symbolsMatchesValues(false);
                            int nbrSides = m_sides.size();
                            if (i == 0 && nbrSides > 1 && previouslySeenIncrement != null && symbolsMatchValues) {
                                resetValuesToWithIncrement(side, true);
                            } else if (i == 1 && nbrSides > 2 && previouslySeenIncrement != null && symbolsMatchValues) {
                                DieSideConfiguration sideFirst = m_sides.get(0).side;
                                previouslySeenIncrement = side.value() - sideFirst.value();
                                resetValuesToWithIncrement(sideFirst, false);
                            }
                        } else {
                            side.setSymbol(symbol);
                        }
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                    }

                    private void resetValuesToWithIncrement(DieSideConfiguration sideFirst, boolean changeSecond) {
                        Integer newValue = sideFirst.value();
                        int i = 0;
                        for (DieSides sideToChange : m_sides) {
                            String newValueString = String.format(Locale.getDefault(), "%d", newValue);
                            if (newValueString.length() > MAX_TEXT_LENGTH_NUMBER) {
                                // stop replacing because the value is too big.
                                break;
                            }
                            if (i > 0) {
                                // only change the second side's symbol if we are editing the first side's symbol.
                                if (i != 1 || changeSecond) {
                                    sideToChange.side.setSymbol(newValueString);
                                    notifyItemChanged(getPositionForRC(i, SYMBOL_VIEW_TYPE));
                                }
                                sideToChange.side.setValue(newValue);
                                notifyItemChanged(getPositionForRC(i, VALUE_VIEW_TYPE));
                            }
                            newValue += previouslySeenIncrement;
                            i++;
                        }
                    }
                });

                return vhSymbol;
            case NOVALUE_VIEW_TYPE:
                Button noValueView = new Button(parent.getContext());
                final ViewHolderNoValue vhNoValue =  new ViewHolderNoValue(noValueView);
                noValueView.setText(R.string.sideColumnClearValue);
                noValueView.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        int i = getDiceSidesIndex(vhNoValue);
                        m_sides.get(i).side.setValue(null);
                        int pos = getPositionForRC(vhNoValue, VALUE_VIEW_TYPE);
                        notifyItemChanged(pos);
                    }
                });
                return vhNoValue;
            case VALUE_VIEW_TYPE:
                EditText valueView = new EditText(parent.getContext());
                final ViewHolderValue vhValue =  new ViewHolderValue(valueView);
                valueView.setInputType(InputType.TYPE_CLASS_NUMBER|InputType.TYPE_NUMBER_FLAG_SIGNED);
                InputFilter[] filtersValue = new InputFilter[1];
                filtersValue[0] = new InputFilter.LengthFilter(MAX_TEXT_LENGTH_NUMBER);
                valueView.setFilters(filtersValue);
                valueView.addTextChangedListener(new TextWatcher() {
                    boolean numberMatches = false;
                    Integer previouslySeenIncrement = null;

                    @Override
                    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                        int i = getDiceSidesIndex(vhValue);
                        if (m_sides.get(i).side.valueEqualsSymbol()) {
                            numberMatches = true;
                        } else {
                            numberMatches = false;
                        }

                        Integer increment = getIncrementingValue();
                        if (previouslySeenIncrement == null) {
                            previouslySeenIncrement = increment;
                        } else if (increment != null) {
                            previouslySeenIncrement = increment;
                        }
                    }

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {
                        int i = getDiceSidesIndex(vhValue);
                        DieSideConfiguration side = m_sides.get(i).side;
                        String valueString = s.toString();
                        if (valueString.length() == 0) {
                            side.setValue(null);
                            return;
                        }

                        if (!DieSideConfiguration.isNumeric(valueString)) {
                            // the user might want to enter -1, but so far they only entered "-".
                            // just return for now and wait till they get the whole thing entered
                            // before updating the internal data.
                            return;
                        }

                        Integer value = Integer.valueOf(valueString);
                        if (side.value() != null && value.equals(side.value())) {
                            return;
                        }

                        if (numberMatches || (side.value() == null && DieSideConfiguration.isNumeric(side.symbol()))) {
                            side.setSymbol(valueString);
                            int pos = getPositionForRC(vhValue, SYMBOL_VIEW_TYPE);
                            notifyItemChanged(pos);
                        }
                        side.setValue(Integer.valueOf(s.toString()));


                        boolean noNullValues = true;
                        for (DieSides sideOther : m_sides) {
                            if (sideOther.side.value() == null) {
                                noNullValues = false;
                                break;
                            }
                        }

                        if (!noNullValues || previouslySeenIncrement == null) {
                            return;
                        }

                        boolean symbolsMatch = symbolsMatchesValues(false);
                        if (i == 0) {
                            // assume that all the sides increment by previouslySeenIncrement
                            // off of the first side.
                            resetValuesToWithIncrement(side, symbolsMatch, true);
                        } else if (i == 1) {
                            // assume that the user is changing the increment.  Change all sides
                            // to increment off the first side, but using the difference between the
                            // first side value and the second side.
                            DieSideConfiguration sideFirst = m_sides.get(0).side;
                            previouslySeenIncrement = side.value() - sideFirst.value();
                            resetValuesToWithIncrement(sideFirst, symbolsMatch, false);
                        }
                    }

                    @Override
                    public void afterTextChanged(Editable s) {
                    }

                    private void resetValuesToWithIncrement(DieSideConfiguration sideFirst,
                                                            boolean setSymbols, boolean changeSecond) {
                        int i = 0;
                        Integer newValue = sideFirst.value();
                        for (DieSides sideToChange : m_sides) {
                            String newValueString = String.format(Locale.getDefault(), "%d", newValue);
                            if (newValueString.length() > MAX_TEXT_LENGTH_NUMBER) {
                                // stop replacing because the value is too big.
                                break;
                            }
                            if (i > 0) {
                                if (setSymbols) {
                                    sideToChange.side.setSymbol(newValueString);
                                    notifyItemChanged(getPositionForRC(i, SYMBOL_VIEW_TYPE));
                                }

                                if (i != 1 || changeSecond) {
                                    sideToChange.side.setValue(newValue);
                                    notifyItemChanged(getPositionForRC(i, VALUE_VIEW_TYPE));
                                }
                            }
                            newValue += previouslySeenIncrement;
                            i++;
                        }
                    }
                });
                return vhValue;
            case REROLL_VIEW_TYPE:
                CheckBox rerollView = new CheckBox(parent.getContext());
                final ViewHolderReRoll vhReroll = new ViewHolderReRoll(rerollView);
                rerollView.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        boolean moreThanHalfChecked = maxHaveRerollChecked();
                        boolean isChecked = ((CheckBox)v).isChecked();
                        int i = getDiceSidesIndex(vhReroll);
                        m_sides.get(i).side.setRerollOn(isChecked);
                        if (moreThanHalfChecked != maxHaveRerollChecked()) {
                            notifyDataSetChanged();
                        }
                    }
                });
                return vhReroll;
            case HEADER_VIEW_TYPE:
                TextView headerView = new TextView(parent.getContext());
                return new ViewHolderHeader(headerView);
            case HEADER_NOVALUE_VIEW_TYPE:
            default:  // default should never be hit.
                Button headerNoValueView = new Button(parent.getContext());
                headerNoValueView.setText(R.string.sideColumnClearValue);
                headerNoValueView.setOnClickListener(new View.OnClickListener(){
                    public void onClick(View view) {
                        for (DieSides side : m_sides) {
                            side.side.setValue(null);
                        }
                        notifyDataSetChanged();
                    }
                });
                return new ViewHolderHeaderNoValue(headerNoValueView);
        }
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder vh, int position) {
        int i = getDiceSidesIndex(vh);
        DieSides sides;
        switch (vh.getItemViewType()) {
            case  REPEATS_VIEW_TYPE:
                sides = m_sides.get(i);
                int totalSidesBefore = 0;
                for (int j = 0; j < i; j++) {
                    totalSidesBefore += m_sides.get(j).repeatNumber;
                }
                ViewHolderRepeats vhRepeats = (ViewHolderRepeats) vh;
                RepeatAdapter adapter = (RepeatAdapter) vhRepeats.m_repeats.getAdapter();
                adapter.setMax(m_totalDiceSides - totalSidesBefore);
                vhRepeats.m_repeats.setSelection(sides.repeatNumber - 1);
                return;
            case SYMBOL_VIEW_TYPE:
                sides = m_sides.get(i);
                ViewHolderSymbol vhSymbol = (ViewHolderSymbol) vh;
                vhSymbol.m_symbol.setText(sides.side.symbol());
                return;
            case NOVALUE_VIEW_TYPE:
                // nothing need be done for this type
                return;
            case VALUE_VIEW_TYPE:
                sides = m_sides.get(i);
                ViewHolderValue vhValue = (ViewHolderValue) vh;
                if (sides.side.value() == null) {
                    vhValue.m_value.setText("");
                } else {
                    vhValue.m_value.setText(String.format(Locale.getDefault(), "%d", sides.side.value()));
                }
                return;
            case REROLL_VIEW_TYPE:
                sides = m_sides.get(i);
                ViewHolderReRoll vhReroll = (ViewHolderReRoll) vh;
                boolean reRollOn = sides.side.reRollOn();
                vhReroll.m_reRoll.setChecked(reRollOn);
                if (!reRollOn && maxHaveRerollChecked()) {
                    vhReroll.m_reRoll.setEnabled(false);
                } else {
                    vhReroll.m_reRoll.setEnabled(true);
                }
                return;
            case HEADER_VIEW_TYPE:
                ViewHolderHeader vhHeader = (ViewHolderHeader) vh;
                switch (position % m_nbrColumns) {
                    case 0:
                        vhHeader.m_textHeader.setText(R.string.sideColumnRepeats);
                        break;
                    case 1:
                        vhHeader.m_textHeader.setText(R.string.sideColumnSymbol);
                        break;
                    case 2:
                        // should not get here.  This column is special and uses HEADER_NOVALUE_VIEW_TYPE
                        break;
                    case 3:
                        vhHeader.m_textHeader.setText(R.string.sideColumnValue);
                        break;
                    case 4:
                        vhHeader.m_textHeader.setText(R.string.sideColumnReroll);
                        break;
                }
                return;
            case HEADER_NOVALUE_VIEW_TYPE:
                // nothing to do for this type
                return;
            default:
        }
    }

    @Override
    public int getItemCount() {
        return (m_sides.size() + 1) * m_nbrColumns;
    }
}
