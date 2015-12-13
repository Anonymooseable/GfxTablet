package at.bitfire.gfxtablet;

import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.support.v7.app.AppCompatActivity;

public class SettingsActivity extends AppCompatActivity {
    public static final String
        KEY_PREF_HOST = "host_preference",
        KEY_PREF_STYLUS_ONLY = "stylus_only_preference",
        KEY_DARK_CANVAS = "dark_canvas_preference",
        KEY_KEEP_DISPLAY_ACTIVE = "keep_display_active_preference",
        KEY_TEMPLATE_IMAGE = "key_template_image",
        KEY_RELATIVE_MODE = "relative_mode_preference",
        KEY_BUTTON_PRESSURE = "button_pressure_preference",
        KEY_REL_SENSITIVITY = "rel_sensitivity_preference";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        setContentView(R.layout.activity_settings);
    }

}
