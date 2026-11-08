package android.content;

/** Host-side test double; never packaged in the Android app. */
public class Context {
    public Context getApplicationContext() {
        return this;
    }
}
