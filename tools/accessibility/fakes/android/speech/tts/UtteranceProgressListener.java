package android.speech.tts;

public abstract class UtteranceProgressListener {
    public abstract void onStart(String id);
    public abstract void onDone(String id);
    public abstract void onError(String id);
    public void onError(String id, int error) { }
    public void onStop(String id, boolean interrupted) { }
}
