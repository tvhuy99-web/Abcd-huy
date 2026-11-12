package com.cleverraven.cataclysmdda;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

/** Legacy entry point: old duplicate reader setup UI is no longer displayed. */
public final class ReaderSetupActivity extends Activity {
    public static final String EXTRA_DISABLE_SELF_VOICE =
            "com.cleverraven.cataclysmdda.extra.DISABLE_SETUP_SELF_VOICE";
    static final String BACKUP_JOURNAL_FILE = "reader_backup_journal.txt";

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        // Compatibility with old shortcuts without recreating the old setup UI.
        startActivity(new Intent(this, AccessibilitySettingsActivity.class));
        finish();
    }
}
