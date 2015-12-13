package at.bitfire.gfxtablet;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;


public class SensorDataClient implements SensorEventListener {
    private SensorManager sensorMan;
    private NetworkClient netClient;
    private Sensor accel;
    // Maximum acceleration value that can be reported
    private static final float MAX_ACCEL = 9.81f;
    private static final String TAG = "GfxTablet.SensorDataClient";

    public SensorDataClient(SensorManager sensorMan, NetworkClient netClient) {
        this.sensorMan = sensorMan;
        this.netClient = netClient;
        accel = sensorMan.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
    }

    @Override
    public void onAccuracyChanged(Sensor s, int accuracy) {
        if (accuracy < SensorManager.SENSOR_STATUS_ACCURACY_MEDIUM) {
            Log.w("Accuracy is low");
        }
    }

    @Override
    public void onSensorChanged(SensorEvent e) {
        if (e.accuracy >= SensorManager.SENSOR_STATUS_ACCURACY_LOW) {
            NetEvent ne = new NetEvent(NetEvent.Type.TYPE_3DMOTION,
                                       (short) (e.values[0] / MAX_ACCEL * Short.MAX_VALUE),
                                       (short) (e.values[1] / MAX_ACCEL * Short.MAX_VALUE),
                                       (short) (e.values[2] / MAX_ACCEL * Short.MAX_VALUE));
            netClient.getQueue().add(ne);
        }
    }

    public void resume() {
        if (accel != null) {
            Log.d(TAG, "Registering");
            sensorMan.registerListener(this, accel, SensorManager.SENSOR_DELAY_GAME);
        } else {
            Log.e(TAG, "Could not get sensor");
        }
    }

    public void suspend() {
        Log.d(TAG, "Unregistering");
        sensorMan.unregisterListener(this);
    }
}
