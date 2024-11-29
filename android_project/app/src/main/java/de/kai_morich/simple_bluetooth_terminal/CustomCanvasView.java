package de.kai_morich.simple_bluetooth_terminal;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import androidx.annotation.NonNull;

import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import java.util.ArrayList;
import java.util.List;

public class CustomCanvasView extends View {
    private final Paint paint;
    private float startX = 50, startY = 50;  // Starting point for drawing lines (initial bot position)
    private float currentAngle = 0;  // The bot's current angle, starting at 0 degrees
    private float previousX = startX, previousY = startY;  // To remember the previous position for drawing lines

    // To handle zooming and scrolling
    private float scaleFactor = 1.0f;  // Scale factor for zooming
    private float offsetX = 0, offsetY = 0;  // Offsets for scrolling
    private GestureDetector gestureDetector;
    private ScaleGestureDetector scaleGestureDetector;

    // List to store the positions
    private List<float[]> positions = new ArrayList<>();

    // Constructor for inflating from XML (with Context and AttributeSet)
    public CustomCanvasView(@NonNull Context context, AttributeSet attrs) {
        super(context, attrs);
        paint = new Paint();
        paint.setColor(Color.RED);  // Set the color for the lines
        paint.setStrokeWidth(5);  // Set the stroke width for the lines
        setWillNotDraw(false);  // Ensures that onDraw is called even when overridden
        // Ensure the first position is added to the positions list
        positions.add(new float[]{50, 50});  // Add the initial position (50, 50)
        currentAngle = -90;

        gestureDetector = new GestureDetector(context, new GestureListener());
        scaleGestureDetector = new ScaleGestureDetector(context, new ScaleListener());
    }

    // Constructor for creating the view programmatically (with Context)
    public CustomCanvasView(@NonNull Context context) {
        super(context);
        paint = new Paint();
        paint.setColor(Color.RED);  // Set the color for the lines
        paint.setStrokeWidth(5);  // Set the stroke width for the lines
        setWillNotDraw(false);  // Ensures that onDraw is called even when overridden

        gestureDetector = new GestureDetector(context, new GestureListener());
        scaleGestureDetector = new ScaleGestureDetector(context, new ScaleListener());

    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        canvas.save();  // Save the canvas state

        // Apply zoom and scroll transformations
        canvas.scale(scaleFactor, scaleFactor);
        canvas.translate(offsetX, offsetY);

        canvas.drawColor(Color.parseColor("#2A2D30"));

        Log.d("CustomCanvasView", "onDraw called");  // Add log to confirm onDraw is being triggered

        // Draw all the stored positions
        for (int i = 1; i < positions.size(); i++) {
            float[] previousPos = positions.get(i - 1);
            float[] currentPos = positions.get(i);

            // Draw the line between consecutive positions
            canvas.drawLine(previousPos[0], previousPos[1], currentPos[0], currentPos[1], paint);
        }

        canvas.restore();  // Restore the canvas state

    }

    // Method to update points based on received data
    public void updatePosition(float distance, float turnAngle) {
        // Update the angle by adding the turnAngle
        currentAngle = (currentAngle + turnAngle) % 360;  // Ensure the angle stays within 0 to 360 degrees

        // Convert the current angle to radians
        float angleInRadians = (float) Math.toRadians(currentAngle);

        // Update the position based on distance and angle
        float newX = (float) (startX + distance * Math.cos(angleInRadians));
        float newY = (float) (startY + distance * Math.sin(angleInRadians));

        // Store the new position
        positions.add(new float[]{newX, newY});

        // Log the new position for debugging
        Log.d("CustomCanvasView", "New Position: X = " + newX + ", Y = " + newY);

        // If you want to limit the number of positions stored (e.g., only store the last 10)
        if (positions.size() > 1000) {
            positions.remove(0);  // Remove the oldest position if more than 10 positions are stored
        }

        // Update startX and startY for the next update
        startX = newX;
        startY = newY;

        invalidate();  // This will call onDraw() again to redraw the view with updated positions
    }

    public void clearCanvas() {
        positions.clear();
        startX = 50;  // Reset to center of canvas
        startY = 50;
        currentAngle = 90;  // Reset angle if needed
        invalidate();  // Redraw the canvas
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        scaleGestureDetector.onTouchEvent(event);
        gestureDetector.onTouchEvent(event);
        return true;
    }

    // GestureDetector for handling scrolling
    private class GestureListener extends GestureDetector.SimpleOnGestureListener {
        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            offsetX -= distanceX;  // Scroll the canvas
            offsetY -= distanceY;
            invalidate();  // Redraw the view
            return true;
        }
    }

    // ScaleGestureDetector for handling zooming
    private class ScaleListener extends ScaleGestureDetector.SimpleOnScaleGestureListener {
        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            scaleFactor *= detector.getScaleFactor();  // Adjust scale factor
            scaleFactor = Math.max(0.1f, Math.min(scaleFactor, 5.0f));  // Limit zoom range
            invalidate();  // Redraw the view
            return true;
        }
    }
}