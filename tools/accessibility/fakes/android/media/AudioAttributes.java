package android.media;

/** Narrow test double for Android AudioAttributes. */
public final class AudioAttributes {
    public static final int USAGE_ASSISTANCE_ACCESSIBILITY = 11;
    public static final int CONTENT_TYPE_SPEECH = 1;

    public final int usage;
    public final int contentType;

    private AudioAttributes(int usage, int contentType) {
        this.usage = usage;
        this.contentType = contentType;
    }

    public static final class Builder {
        private int usage;
        private int contentType;

        public Builder setUsage(int usage) {
            this.usage = usage;
            return this;
        }

        public Builder setContentType(int contentType) {
            this.contentType = contentType;
            return this;
        }

        public AudioAttributes build() {
            return new AudioAttributes(usage, contentType);
        }
    }
}
