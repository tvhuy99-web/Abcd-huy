package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.os.Looper;
import android.speech.tts.TextToSpeech;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Runs the real driver with deterministic Android API test doubles. */
public final class AndroidSpeechTest {
    private static int assertions;
    private static AndroidSpeech speech;
    private static final List<String> events = new ArrayList<>();
    private static final List<AndroidSpeech.State> states = new ArrayList<>();

    private static void check(boolean condition, String reason) {
        assertions++;
        if (!condition) {
            throw new AssertionError(reason + ": " + events + " / " + states);
        }
    }

    private static void fresh() {
        Looper.reset();
        events.clear();
        states.clear();
        speech = new AndroidSpeech(new Context(), new AndroidSpeech.Listener() {
            public void onState(AndroidSpeech.State state) {
                states.add(state);
            }
            public void onUtterance(String id, AndroidSpeech.Event event) {
                events.add(id + ":" + event);
            }
        });
    }

    private static TextToSpeech initialize() {
        speech.initialize(null, new Locale("vi", "VN"), 1.0f);
        Looper.runReady();
        return TextToSpeech.latest;
    }

    private static TextToSpeech ready() {
        TextToSpeech engine = initialize();
        engine.finishInit(TextToSpeech.SUCCESS);
        Looper.runReady();
        check(speech.getState() == AndroidSpeech.State.READY, "Voice should be ready");
        return engine;
    }

    public static void main(String[] args) {
        fresh();
        speech.speak("early", "Không được xếp chờ.");
        Looper.runReady();
        check(events.equals(List.of("early:REJECTED")), "Not-ready request must be rejected");
        TextToSpeech engine = ready();
        speech.speak("focus", "Xin chào.");
        Looper.runReady();
        String token = engine.token;
        engine.progress.onStart(token);
        engine.progress.onStart(token);
        engine.progress.onDone(token);
        engine.progress.onDone(token);
        Looper.runReady();
        check(events.equals(List.of("early:REJECTED", "focus:STARTED", "focus:DONE")),
                "Callbacks must be unique and ordered");
        Looper.advance(10000);
        check(speech.getState() == AndroidSpeech.State.READY, "Completed speech cancels watchdog");

        fresh();
        engine = ready();
        speech.speak("same", "Mục cũ");
        Looper.runReady();
        String old = engine.token;
        speech.cancel();
        speech.speak("same", "Mục mới");
        Looper.runReady();
        String current = engine.token;
        engine.progress.onDone(old);
        engine.progress.onError(old);
        engine.progress.onStop(old, true);
        engine.progress.onStart(current);
        engine.progress.onDone(current);
        Looper.runReady();
        check(events.equals(List.of("same:CANCELLED", "same:STARTED", "same:DONE")),
                "Cancelled callback must not affect replacement with same label");
        check(speech.getState() == AndroidSpeech.State.READY, "Old errors must not fault replacement");

        fresh();
        TextToSpeech first = initialize();
        TextToSpeech second = initialize();
        check(first.closed, "Replacing engine must shut down the old engine");
        first.finishInit(TextToSpeech.ERROR);
        second.finishInit(TextToSpeech.SUCCESS);
        Looper.runReady();
        check(speech.getState() == AndroidSpeech.State.READY, "Late initialization must be ignored");

        fresh();
        engine = initialize();
        Looper.advance(8000);
        check(speech.getState() == AndroidSpeech.State.FAILED && engine.closed, "Init timeout must release engine");
        engine.finishInit(TextToSpeech.SUCCESS);
        Looper.runReady();
        check(speech.getState() == AndroidSpeech.State.FAILED, "Late init cannot recover implicitly");

        fresh();
        engine = ready();
        speech.speak("timeout", "Không bắt đầu");
        Looper.runReady();
        Looper.advance(6000);
        check(events.equals(List.of("timeout:ERROR")), "Start timeout emits one error");
        check(speech.getState() == AndroidSpeech.State.FAILED && engine.closed, "Timeout must fault and release engine");

        fresh();
        engine = initialize();
        engine.languageResult = TextToSpeech.LANG_MISSING_DATA;
        engine.finishInit(TextToSpeech.SUCCESS);
        Looper.runReady();
        check(speech.getState() == AndroidSpeech.State.MISSING_LANGUAGE, "Missing language is explicit");
        check(engine.closed, "Missing-language engine must be released");

        fresh();
        engine = ready();
        engine.speakResult = TextToSpeech.ERROR;
        speech.speak("error", "Lỗi xếp hàng");
        Looper.runReady();
        check(events.equals(List.of("error:ERROR")), "Immediate speak error is not a success");
        check(speech.getState() == AndroidSpeech.State.FAILED, "Speak error must fault backend");

        fresh();
        engine = ready();
        speech.speak("too-long", "x".repeat(201));
        speech.speak("empty", "");
        Looper.runReady();
        check(engine.speakCalls == 0, "Invalid text must not reach engine");
        speech.speak("one", "Một");
        speech.speak("two", "Hai");
        Looper.runReady();
        check(engine.speakCalls == 1 && events.contains("two:REJECTED"), "Driver must not build its own backlog");
        speech.close();
        Looper.runReady();
        int before = events.size();
        engine.progress.onDone(engine.token);
        speech.initialize(null, Locale.ENGLISH, 1.0f);
        speech.speak("closed", "Không đọc");
        Looper.runReady();
        check(speech.getState() == AndroidSpeech.State.CLOSED && engine.closed, "Close is terminal");
        check(events.size() == before, "Closed owner must not receive late callbacks");

        fresh();
        engine = ready();
        engine.stopThrows = true;
        speech.cancel();
        Looper.runReady();
        check(speech.getState() == AndroidSpeech.State.FAILED && engine.closed, "Disconnected stop must fault backend");
        System.out.println("AndroidSpeech driver (test doubles): " + assertions + " assertions passed");
    }
}
