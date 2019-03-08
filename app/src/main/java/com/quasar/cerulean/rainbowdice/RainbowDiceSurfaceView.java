package com.quasar.cerulean.rainbowdice;

import android.content.Context;
import android.util.AttributeSet;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SurfaceView;

public class RainbowDiceSurfaceView extends SurfaceView {
    private class GestureListener extends GestureDetector.SimpleOnGestureListener {
        @Override
        public boolean onScroll(MotionEvent ev1, MotionEvent ev2, float distanceX, float distanceY) {
            Draw.scroll(distanceX, distanceY);
            return true;
        }

        @Override
        public boolean onDown(MotionEvent ev) {
            return true;
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent ev) {
            return RainbowDiceSurfaceView.this.performClick();
        }
    }
    private class ScaleGesterListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            Draw.scale(detector.getScaleFactor());

            return true;
        }

    }
    GestureDetector gestureDetector;
    ScaleGestureDetector scaleGestureDetector;

    RainbowDiceSurfaceView(Context ctx) {
        super(ctx);
        gestureDetector = new GestureDetector(ctx, new GestureListener());
        scaleGestureDetector = new ScaleGestureDetector(ctx, new ScaleGesterListener());
    }

    RainbowDiceSurfaceView(Context ctx, AttributeSet attrs) {
        super(ctx, attrs);
        gestureDetector = new GestureDetector(ctx, new GestureListener());
        scaleGestureDetector = new ScaleGestureDetector(ctx, new ScaleGesterListener());
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        boolean gest1res = scaleGestureDetector.onTouchEvent(ev);

        boolean gest2res = gestureDetector.onTouchEvent(ev);

        return gest1res || gest2res;
    }

    @Override
    public boolean performClick() {
        return super.performClick();
    }
}
