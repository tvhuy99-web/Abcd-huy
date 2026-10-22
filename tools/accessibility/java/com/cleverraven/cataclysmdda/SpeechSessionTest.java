package com.cleverraven.cataclysmdda;

/** Host-side lifecycle/race tests; no Android runtime or network required. */
public final class SpeechSessionTest {
    private static int assertions;

    private static void check(boolean condition, String reason) {
        assertions++;
        if (!condition) {
            throw new AssertionError(reason);
        }
    }

    public static void main(String[] args) {
        SpeechSession session = new SpeechSession();
        check(session.begin("before-init") == null, "Not ready must reject");
        long oldEngine = session.newEngine();
        long engine = session.newEngine();
        check(!session.markReady(oldEngine), "Stale initialization must be ignored");
        check(session.begin("still-not-ready") == null, "Stale ready must not enable speech");
        check(session.markReady(engine), "Current engine becomes ready");
        check(session.begin(null) == null, "Null caller id must be rejected");
        check(session.begin("") == null, "Empty caller id must be rejected");
        String first = session.begin("same-label");
        check(first != null, "Ready backend accepts one request");
        check(session.begin("second") == null, "No backend backlog");
        check("same-label".equals(session.start(first)), "Start preserves caller id");
        check(session.start(first) == null, "Duplicate start is ignored");
        check("same-label".equals(session.cancel()), "Cancel reports pending request once");
        check(session.cancel() == null, "Cancel is idempotent");
        String second = session.begin("same-label");
        check(second != null && !second.equals(first), "Reused label needs a distinct engine token");
        check(session.complete(first) == null, "Late done after cancel cannot complete replacement");
        check(session.start(first) == null, "Late start after cancel is ignored");
        check(session.isActive(second), "Replacement is still pending");
        check("same-label".equals(session.complete(second)), "Current request completes");
        check(session.complete(second) == null, "Duplicate completion is ignored");
        String beforeReload = session.begin("menu");
        long restarted = session.newEngine();
        check(session.complete(beforeReload) == null, "Reload invalidates active speech");
        check(!session.markReady(engine), "Old engine cannot become ready after restart");
        check(session.markReady(restarted), "Restarted engine becomes ready");
        String afterReload = session.begin("menu");
        check(!afterReload.equals(beforeReload), "Restart tokens cannot collide");
        check(session.start(null) == null, "Null Android callback cannot match an active utterance");
        check(session.isActive(afterReload), "Malformed callback does not affect current speech");
        System.out.println("SpeechSession: " + assertions + " assertions passed");
    }
}
