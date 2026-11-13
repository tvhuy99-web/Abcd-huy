package com.cleverraven.cataclysmdda;

import static org.junit.Assert.assertEquals;
import org.junit.Test;

/** Announce a menu title once, keeping every fact in the selected row. */
public final class ReaderMenuSpeechTest {
    @Test public void identicalMenuTitleIsNotRepeated() {
        assertEquals("Tải game. Mục 1 trên 3",
                GameReaderController.menuEntrySpeech("Tải game", "Tải game. Mục 1 trên 3"));
    }

    @Test public void differentItemKeepsPositionAndContext() {
        assertEquals("Tải game. Matoaka, mục 1 trên 3",
                GameReaderController.menuEntrySpeech("Tải game", "Matoaka, mục 1 trên 3"));
    }

    @Test public void questionAndSelectionAreBothSpoken() {
        assertEquals("Dừng nạp đạn? Có, mục 1 trên 4",
                GameReaderController.menuEntrySpeech("Dừng nạp đạn?", "Có, mục 1 trên 4"));
    }

    @Test public void blankTitleDoesNotHideSelection() {
        assertEquals("Bắc 1 ô, sức khỏe 97%",
                GameReaderController.menuEntrySpeech("", "Bắc 1 ô, sức khỏe 97%"));
    }
}
