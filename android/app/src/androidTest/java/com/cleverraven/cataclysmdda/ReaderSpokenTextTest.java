package com.cleverraven.cataclysmdda;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

/** Spoken cleanup only removes markup and accidental doubled punctuation. */
public final class ReaderSpokenTextTest {
    @Test
    public void removesDuplicateFullStopBetweenJoinedStatusSentences() {
        assertEquals("Tay trái: sức khỏe 5/5. Tay phải: 4/5.",
                ReaderSpokenText.clean("Tay trái: sức khỏe 5/5.. Tay phải: 4/5."));
    }

    @Test
    public void preservesIntentionalEllipsesDecimalValuesAndCaliber() {
        assertEquals("Đợi... 3.14 mét. Đạn .45 Colt.",
                ReaderSpokenText.clean("Đợi... 3.14 mét. Đạn .45 Colt."));
    }

    @Test
    public void preservesNamesCountsAndHealthPercentagesWhenCleaningMarkup() {
        assertEquals("Chó xác sống, Bắc 1 ô, sức khỏe 97%.",
                ReaderSpokenText.clean("<color_red>Chó xác sống</color>, Bắc 1 ô, sức khỏe 97%."));
    }

    @Test
    public void nullTextIsSafe() {
        assertEquals("", ReaderSpokenText.clean(null));
    }
}
