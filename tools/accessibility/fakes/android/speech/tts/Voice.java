package android.speech.tts;

import java.util.Locale;

public final class Voice {
    private final Locale locale;
    private final boolean network;
    private final String name;

    public Voice(Locale locale, boolean network) {
        this(locale, network, null);
    }

    public Voice(Locale locale, boolean network, String name) {
        this.locale = locale;
        this.network = network;
        this.name = name != null ? name
                : (locale == null ? "voice" : locale.toLanguageTag())
                + (network ? "-network" : "-local");
    }

    public String getName() {
        return name;
    }

    public Locale getLocale() {
        return locale;
    }

    public boolean isNetworkConnectionRequired() {
        return network;
    }
}
