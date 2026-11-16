package com.cleverraven.cataclysmdda;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.lang.reflect.Field;

import static org.junit.Assert.assertEquals;

/** Validate real item/creature SoundPool channel gating without playing sounds. */
@RunWith(AndroidJUnit4.class)
public final class DirectionalSonarChannelsTest {
    private static int distanceFor(AlignmentSonarPlayer sonar, String category)
            throws ReflectiveOperationException {
        Field field = AlignmentSonarPlayer.class.getDeclaredField(category);
        field.setAccessible(true);
        return ((int[]) field.get(sonar))[0];
    }

    @Test public void independentCategorySwitchesFilterTargetsAndClearOldSounds() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            AlignmentSonarPlayer sonar = new AlignmentSonarPlayer(context);
            int[] creatures = { 2, -1, -1, -1, -1, -1, -1, -1 };
            int[] items = { 3, -1, -1, -1, -1, -1, -1, -1 };
            try {
                sonar.setChannels(true, false);
                sonar.updateTargets(creatures, items);
                assertEquals(2, distanceFor(sonar, "creatures"));
                assertEquals(-1, distanceFor(sonar, "items"));

                sonar.setChannels(false, true);
                sonar.updateTargets(creatures, items);
                assertEquals(-1, distanceFor(sonar, "creatures"));
                assertEquals(3, distanceFor(sonar, "items"));

                sonar.setChannels(false, false);
                sonar.updateTargets(creatures, items);
                assertEquals(-1, distanceFor(sonar, "creatures"));
                assertEquals(-1, distanceFor(sonar, "items"));
            } catch (ReflectiveOperationException error) {
                throw new AssertionError(error);
            } finally {
                sonar.close();
            }
        });
    }
}
