package com.cleverraven.cataclysmdda;

import android.content.Context;
import java.util.ArrayList;
import java.util.List;

/**
 * One focused, short explanation per gesture. These rows are NEVER actions:
 * the help layer only speaks their labels and returns to the original focus.
 *
 * The native snapshot owns the screen identity; world driving/combat modes
 * refine that identity without changing any user-assigned world bindings.
 */
final class ReaderGestureGuide {
    private ReaderGestureGuide() {}

    private static void add(List<String> rows, Context c, int gesture, int meaning) {
        rows.add(c.getString(gesture) + ": " + c.getString(meaning));
    }

    private static void addSwipe(List<String> rows, Context c, String id, int meaning) {
        int index = ReaderGestureBindings.indexOf(id);
        if (index >= 0) {
            add(rows, c, ReaderGestureBindings.LABELS[index], meaning);
        }
    }

    static List<String> entries(Context context, ReaderSnapshot snapshot,
                                boolean driving, boolean combat,
                                boolean inHub, boolean traveling,
                                ReaderGestureBindings bindings) {
        ArrayList<String> rows = new ArrayList<>();
        final String category = snapshot == null ? "" : snapshot.category;
        if (traveling) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideTravelStatus);
        } else if (driving && "DEFAULTMODE".equals(category)) {
            addSwipe(rows, context, "swipe_up", R.string.a11yGuideDriveForward);
            addSwipe(rows, context, "swipe_down", R.string.a11yGuideDriveReverse);
            addSwipe(rows, context, "swipe_left", R.string.a11yGuideSteerLeft);
            addSwipe(rows, context, "swipe_right", R.string.a11yGuideSteerRight);
            addSwipe(rows, context, "swipe_up_down", R.string.a11yGuideFaster);
            addSwipe(rows, context, "swipe_down_up", R.string.a11yGuideSlower);
            addSwipe(rows, context, "swipe_left_right", R.string.a11yGuideHeadingLeft);
            addSwipe(rows, context, "swipe_right_left", R.string.a11yGuideHeadingRight);
            addSwipe(rows, context, "swipe_up_right", R.string.a11yGuideEngine);
            addSwipe(rows, context, "swipe_down_left", R.string.a11yGuideSpeed);
            addSwipe(rows, context, "swipe_down_right", R.string.a11yGuideCondition);
            addSwipe(rows, context, "swipe_up_left", R.string.a11yGuideBrake);
            addSwipe(rows, context, "swipe_right_down", R.string.a11yGuideHorn);
            add(rows, context, R.string.a11yGuideTwoUp, R.string.a11yGuideScanFront);
            add(rows, context, R.string.a11yGuideTwoDown, R.string.a11yGuideScanBack);
            add(rows, context, R.string.a11yGuideTwoLeft, R.string.a11yGuideScanLeft);
            add(rows, context, R.string.a11yGuideTwoRight, R.string.a11yGuideScanRight);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideVehicleMenu);
            add(rows, context, R.string.a11yBindTap33, R.string.a11yGuideChangeMode);
        } else if (combat && "DEFAULTMODE".equals(category)) {
            addSwipe(rows, context, "swipe_up", R.string.a11yGuideAttackNorth);
            addSwipe(rows, context, "swipe_down", R.string.a11yGuideAttackSouth);
            addSwipe(rows, context, "swipe_left", R.string.a11yGuideAttackWest);
            addSwipe(rows, context, "swipe_right", R.string.a11yGuideAttackEast);
            addSwipe(rows, context, "swipe_up_left", R.string.a11yGuideAttackNorthwest);
            addSwipe(rows, context, "swipe_up_right", R.string.a11yGuideAttackNortheast);
            addSwipe(rows, context, "swipe_down_left", R.string.a11yGuideAttackSouthwest);
            addSwipe(rows, context, "swipe_down_right", R.string.a11yGuideAttackSoutheast);
            addSwipe(rows, context, "swipe_left_right", R.string.a11yGuideReload);
            addSwipe(rows, context, "swipe_right_left", R.string.a11yGuideWeaponDetails);
            addSwipe(rows, context, "swipe_up_down", R.string.a11yGuideNextWeapon);
            addSwipe(rows, context, "swipe_down_up", R.string.a11yGuidePreviousWeapon);
            add(rows, context, R.string.a11yGuideTwoUp, R.string.a11yGuideWalkNorth);
            add(rows, context, R.string.a11yGuideTwoDown, R.string.a11yGuideWalkSouth);
            add(rows, context, R.string.a11yGuideTwoLeft, R.string.a11yGuideWalkWest);
            add(rows, context, R.string.a11yGuideTwoRight, R.string.a11yGuideWalkEast);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideCombatMenu);
            add(rows, context, R.string.a11yBindTap33, R.string.a11yGuideChangeMode);
        } else if ("DEFAULTMODE".equals(category) && !inHub) {
            addSwipe(rows, context, "swipe_up", R.string.a11yGuideWalkNorth);
            addSwipe(rows, context, "swipe_down", R.string.a11yGuideWalkSouth);
            addSwipe(rows, context, "swipe_left", R.string.a11yGuideWalkWest);
            addSwipe(rows, context, "swipe_right", R.string.a11yGuideWalkEast);
            add(rows, context, R.string.a11yGuideTwoUp, R.string.a11yGuideScanNorth);
            add(rows, context, R.string.a11yGuideTwoDown, R.string.a11yGuideScanSouth);
            add(rows, context, R.string.a11yGuideTwoLeft, R.string.a11yGuideScanWest);
            add(rows, context, R.string.a11yGuideTwoRight, R.string.a11yGuideScanEast);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindTap22, R.string.a11yGuideRecentMessages);
            add(rows, context, R.string.a11yBindTap32, R.string.a11yGuidePlayerInfo);
            add(rows, context, R.string.a11yBindTap33, R.string.a11yGuideChangeMode);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
            // Only default world gestures use user-assignable shortcuts.
            if (snapshot != null && bindings != null) {
                for (String id : ReaderGestureBindings.CUSTOM_IDS) {
                    final String target = bindings.targetFor(id);
                    if (target == null) { continue; }
                    String action = "";
                    if (target.startsWith("action:")) {
                        ReaderSnapshot.Node node = snapshot.find(target.substring(7));
                        if (node == null || !node.enabled) { continue; }
                        action = node.label;
                    } else if (target.startsWith("hub:group:")) {
                        try {
                            int group = Integer.parseInt(target.substring(10));
                            if (group > 0 && group < GameActionHub.TITLES.length) {
                                action = context.getString(GameActionHub.TITLES[group]);
                            }
                        } catch (NumberFormatException ignored) { continue; }
                    }
                    if (!action.isEmpty()) {
                        int index = ReaderGestureBindings.indexOf(id);
                        if (index >= 0) {
                            rows.add(context.getString(ReaderGestureBindings.LABELS[index])
                                    + ": " + action);
                        }
                    }
                }
            }
        } else if ("inventory".equals(category) || "pickup".equals(category)
                || "advinv".equals(category) || "item_action_menu".equals(category)
                || "drop".equals(category) || "wear".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideOneVertical, R.string.a11yGuideBrowsePockets);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideSelectItem);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideItemActions);
        } else if ("surroundings".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideOneVertical, R.string.a11yGuideChangeFilter);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("crafting".equals(category) || "construction".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseRecipes);
            add(rows, context, R.string.a11yGuideOneVertical, R.string.a11yGuideChangeCategory);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideSelectItem);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("targeting".equals(category) || "look-around".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseTargets);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideSelectItem);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideTargetControls);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
        } else if ("CHARACTER_CREATION".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideOneVertical, R.string.a11yGuideChangeCategory);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else {
            // Safe generic fallback for native dialogs, settings, missions,
            // journals, minigames, action screens and future screen types.
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        }
        if (!"DEFAULTMODE".equals(category) || inHub) {
            add(rows, context, R.string.a11yGuideTwoDown, R.string.a11yGuideGoBack);
        }
        return rows;
    }
}
