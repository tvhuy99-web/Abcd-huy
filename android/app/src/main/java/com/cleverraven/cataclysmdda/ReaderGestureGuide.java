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

    /** Shared controls are appended once, even when a screen has a more
     * precise instruction for the same gesture. */
    private static void addOnce(List<String> rows, Context c, int gesture, int meaning) {
        String prefix = c.getString(gesture) + ": ";
        for (String row : rows) {
            if (row.startsWith(prefix)) { return; }
        }
        add(rows, c, gesture, meaning);
    }

    /** Field options/number ranges override the ordinary menu's vertical
     * navigation instruction only when the native node exposes an edit API. */
    private static void describeFocusedField(List<String> rows, Context c,
                                             ReaderSnapshot snapshot) {
        if (snapshot == null) { return; }
        ReaderSnapshot.Node node = snapshot.find(snapshot.focus);
        if (node == null || !node.enabled) { return; }
        boolean choice = "choice".equals(node.fieldType)
                && node.actions.contains("select_choice") && !node.choices.isEmpty();
        boolean number = ("int".equals(node.fieldType) || "float".equals(node.fieldType))
                && node.actions.contains("set_value") && node.hasFieldRange
                && node.fieldStep > 0;
        boolean itemQuantity = "checkbox".equals(node.role) && number
                && node.fieldMin == 0.0 && node.fieldMax > 1.0
                && node.fieldStep == 1.0 && !node.actions.contains("apply")
                && ("inventory".equals(snapshot.category)
                    || "pickup-here".equals(snapshot.category));
        if ((!choice && !number) || itemQuantity) { return; }
        String prefix = c.getString(R.string.a11yGuideOneVertical) + ": ";
        rows.removeIf(row -> row.startsWith(prefix));
        add(rows, c, R.string.a11yGuideOneVertical, R.string.a11yGuideAdjustField);
    }

    /** Read the live shortcut map, not an obsolete table of default keys.
     * Combat disables old three-finger movement, but its other saved
     * shortcuts still reach executeShortcutTarget in the controller. */
    private static void appendLiveShortcuts(List<String> rows, Context context,
                                           ReaderSnapshot snapshot,
                                           ReaderGestureBindings bindings,
                                           boolean combat) {
        if (snapshot == null || bindings == null) { return; }
        for (String id : ReaderGestureBindings.CUSTOM_IDS) {
            if (combat && id.startsWith("swipe_3_")) { continue; }
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
            } else if ("hub:favorites".equals(target)) {
                action = context.getString(R.string.a11yHubFavorites);
            } else if ("hub:search".equals(target)) {
                action = context.getString(R.string.a11yHubSearch);
            } else if ("hub:all".equals(target)) {
                action = context.getString(R.string.a11yHubAllCommands);
            } else if ("hub:directions".equals(target)) {
                action = context.getString(R.string.a11yHubChooseDirection);
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

    private static void addSwipe(List<String> rows, Context c, String id, int meaning) {
        int index = ReaderGestureBindings.indexOf(id);
        if (index >= 0) {
            add(rows, c, ReaderGestureBindings.LABELS[index], meaning);
        }
    }

    static List<String> entries(Context context, ReaderSnapshot snapshot,
                                boolean driving, boolean combat,
                                boolean inHub, boolean traveling,
                                boolean editing, boolean debugging,
                                ReaderGestureBindings bindings) {
        return entries(context, snapshot, driving, combat, inHub, traveling,
                editing, debugging, bindings, false, 0);
    }

    static List<String> entries(Context context, ReaderSnapshot snapshot,
                                boolean driving, boolean combat,
                                boolean inHub, boolean traveling,
                                boolean editing, boolean debugging,
                                ReaderGestureBindings bindings,
                                boolean readingDocument, int pocketStage) {
        ArrayList<String> rows = new ArrayList<>();
        final String category = snapshot == null ? "" : snapshot.category;
        if (editing) {
            add(rows, context, R.string.a11yGuideTapTextField,
                    R.string.a11yGuideAndroidKeyboard);
            add(rows, context, R.string.a11yGuideTwoUp,
                    R.string.a11yGuideConfirmInput);
            add(rows, context, R.string.a11yGuideTwoDown,
                    R.string.a11yGuideCancelInput);
        } else if (debugging) {
            add(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideDebugChoice);
            add(rows, context, R.string.a11yGuideDoubleTap,
                    R.string.a11yGuideDebugConfirm);
            add(rows, context, R.string.a11yGuideTwoDown,
                    R.string.a11yGuideDebugContinue);
        } else if (readingDocument) {
            // DocumentReader intercepts swipes to move the reading cursor
            // through paragraphs, independent of the underlying category.
            add(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideBrowseParagraph);
            add(rows, context, R.string.a11yGuideOneVertical,
                    R.string.a11yGuideBrowseParagraph);
            add(rows, context, R.string.a11yGuideDoubleTap,
                    R.string.a11yGuideReadDocument);
            add(rows, context, R.string.a11yBindHold1,
                    R.string.a11yGuideScreenMenu);
        } else if (pocketStage != 0) {
            add(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideOneVertical,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap,
                    pocketStage == 1 ? R.string.a11yGuideChoosePocketAction
                    : pocketStage == 2 ? R.string.a11yGuidePickPocket
                    : R.string.a11yGuideChoosePocketItem);
            if (pocketStage == 3) {
                add(rows, context, R.string.a11yGuideTwoUp,
                        R.string.a11yGuideConfirmPocket);
            }
            add(rows, context, R.string.a11yBindHold1,
                    R.string.a11yGuideScreenMenu);
        } else if (traveling) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideTravelStatus);
        } else if (inHub) {
            // This overlay is itself a distinct screen even if the underlying
            // native snapshot still says DEFAULTMODE.
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideHubItemOptions);
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
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideDrivePause);
            add(rows, context, R.string.a11yBindTap22, R.string.a11yGuideRecentMessages);
            add(rows, context, R.string.a11yBindTap31, R.string.a11yGuideCoordinates);
            add(rows, context, R.string.a11yBindTap32, R.string.a11yGuidePlayerInfo);
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
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideCombatEnemy);
            add(rows, context, R.string.a11yBindTap22, R.string.a11yGuideRecentMessages);
            add(rows, context, R.string.a11yBindTap31, R.string.a11yGuideCoordinates);
            add(rows, context, R.string.a11yBindTap32, R.string.a11yGuidePlayerInfo);
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
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindTap22, R.string.a11yGuideRecentMessages);
            add(rows, context, R.string.a11yBindTap23, R.string.a11yGuideOpenHub);
            add(rows, context, R.string.a11yBindTap31, R.string.a11yGuideCoordinates);
            add(rows, context, R.string.a11yGuideTripleTap, R.string.a11yGuideOpenHub);
            add(rows, context, R.string.a11yBindTap32, R.string.a11yGuidePlayerInfo);
            add(rows, context, R.string.a11yBindTap33, R.string.a11yGuideChangeMode);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("inventory".equals(category) || "pickup".equals(category)
                || "pickup-here".equals(category) || "advinv".equals(category)
                || "item_action_menu".equals(category)
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
        } else if ("crafting".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseRecipes);
            add(rows, context, R.string.a11yGuideOneVertical, R.string.a11yGuideChangeCategory);
            add(rows, context, R.string.a11yGuideTwoLeft, R.string.a11yGuidePreviousCraftTab);
            add(rows, context, R.string.a11yGuideTwoRight, R.string.a11yGuideNextCraftTab);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideSelectItem);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("construction".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideSelectItem);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("targeting".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseTargets);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideSelectItem);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideTargetControls);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
        } else if ("look-around".equals(category) || "overmap".equals(category)
                || "overmap_search".equals(category) || "places".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if (category.startsWith("character-creator-step-")) {
            // Native creator publishes a separate category for EVERY tab.
            // The first two steps activate a selection; steps 3-6 use
            // NEXT_TAB to validate multiselect attributes/skills/traits.
            add(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideOneVertical,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap,
                    R.string.a11yGuideActivate);
            if ("character-creator-step-3".equals(category)
                    || "character-creator-step-4".equals(category)
                    || "character-creator-step-5".equals(category)
                    || "character-creator-step-6".equals(category)) {
                add(rows, context, R.string.a11yGuideTwoUp,
                        R.string.a11yGuideCreatorNextStep);
            } else {
                add(rows, context, R.string.a11yGuideTwoUp,
                        R.string.a11yGuideCreatorSelect);
            }
            add(rows, context, R.string.a11yBindHold1,
                    R.string.a11yGuideScreenMenu);
        } else if ("character-creator-calendar".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideAdjustCalendar);
            add(rows, context, R.string.a11yGuideOneVertical, R.string.a11yGuideChangeCalendarField);
            add(rows, context, R.string.a11yGuideTwoUp, R.string.a11yGuideCalendarConfirm);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("robot-finds-kitten".equals(category) || "snake".equals(category)
                || "sokoban".equals(category) || "lightson".equals(category)) {
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuidePlayMinigame);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
        } else if ("CHARACTER_CREATION".equals(category)) {
            // Final summary is a separate native category. Two-finger UP
            // requests NEXT_TAB (Finish), not the focused summary row.
            add(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideOneVertical,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap,
                    R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yGuideTwoUp,
                    R.string.a11yGuideCreatorFinish);
            add(rows, context, R.string.a11yBindHold1,
                    R.string.a11yGuideScreenMenu);
        } else if ("YESNO".equals(category) || "YESNOQUIT".equals(category)) {
            // Confirm selects Yes regardless of which answer has focus.
            add(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap,
                    R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yGuideTwoUp,
                    R.string.a11yGuideAcceptYes);
        } else {
            // Generic read-only guidance for dialog, diary, missions,
            // inventories, configuration screens and future native scopes.
            add(rows, context, R.string.a11yGuideOneHorizontal, R.string.a11yGuideBrowseItems);
            add(rows, context, R.string.a11yGuideDoubleTap, R.string.a11yGuideActivate);
            add(rows, context, R.string.a11yBindTap21, R.string.a11yGuideReadDetails);
            add(rows, context, R.string.a11yBindHold1, R.string.a11yGuideScreenMenu);
            if (snapshot != null) {
                ReaderSnapshot.Node current = snapshot.find(snapshot.focus);
                if (current != null &&
                        (("choice".equals(current.fieldType)
                          && current.actions.contains("select_choice")) ||
                         (("int".equals(current.fieldType)
                           || "float".equals(current.fieldType))
                          && current.actions.contains("set_value")
                          && current.hasFieldRange && current.fieldStep > 0))) {
                    add(rows, context, R.string.a11yGuideOneVertical,
                            R.string.a11yGuideAdjustField);
                }
            }
        }
        // The following mappings come from ReaderGestureSurface and
        // GameReaderController, and apply independently of native screen
        // labels. Do not copy them into dozens of screen-specific blocks.
        // Two-finger tap twice reads cached game messages in EVERY screen.
        addOnce(rows, context, R.string.a11yBindTap22,
                R.string.a11yGuideRecentMessages);
        // A single tap stops speech in both the world and native menus.
        addOnce(rows, context, R.string.a11yGuideSingleTap,
                R.string.a11yGuideStopSpeech);
        boolean liveWorld = "DEFAULTMODE".equals(category)
                && !inHub && !traveling && !editing && !debugging
                && !readingDocument && pocketStage == 0;
        if (liveWorld) {
            // The two hub gestures work in Map, Combat and Drive.
            addOnce(rows, context, R.string.a11yBindTap23,
                    R.string.a11yGuideOpenHub);
            addOnce(rows, context, R.string.a11yGuideTripleTap,
                    R.string.a11yGuideOpenHub);
            addOnce(rows, context, R.string.a11yBindTap21,
                    combat ? R.string.a11yGuideCombatEnemy
                           : R.string.a11yGuideReadDetails);
        } else {
            if (!debugging) {
                addOnce(rows, context, R.string.a11yBindTap21,
                        R.string.a11yGuideReadDetails);
            }
            // All modal screens share the same 2-finger UP confirm and
            // 2-finger DOWN Back dispatchers. A screen-specific definition
            // above takes precedence (creator, crafting, calendar, editor).
            if (!debugging && !readingDocument
                    && !("crafting".equals(category) && snapshot != null
                         && snapshot.find("craft:detail:qty:current") != null)) {
                int confirmation = "crafting".equals(category) && snapshot != null
                        && snapshot.find("craft:detail:confirm") != null
                        ? R.string.a11yGuideCraftConfirm
                        : R.string.a11yGuideConfirmSelection;
                addOnce(rows, context, R.string.a11yGuideTwoUp, confirmation);
            }
            addOnce(rows, context, R.string.a11yGuideTwoDown,
                    R.string.a11yGuideGoBack);
            if (!editing && !debugging) {
                addOnce(rows, context, R.string.a11yGuideDoubleTap,
                        R.string.a11yGuideActivate);
                addOnce(rows, context, R.string.a11yBindHold1,
                        R.string.a11yGuideScreenMenu);
            }
        }
        if (!liveWorld && !editing && !debugging && !readingDocument
                && pocketStage == 0 && !"character-creator-calendar".equals(category)
                && !"surroundings".equals(category) && !"crafting".equals(category)) {
            // All other ordinary menus receive configured focus navigation.
            addOnce(rows, context, R.string.a11yGuideOneHorizontal,
                    R.string.a11yGuideBrowseItems);
            addOnce(rows, context, R.string.a11yGuideOneVertical,
                    R.string.a11yGuideBrowseItems);
        }
        if (!liveWorld && !inHub && !traveling && !editing && !debugging
                && !readingDocument && pocketStage == 0
                && !"character-creator-calendar".equals(category)) {
            describeFocusedField(rows, context, snapshot);
        }
        if (liveWorld && !driving) {
            // Vehicle mode consumes all unknown swipes for safety, while
            // Map and Combat may execute live saved actions.
            appendLiveShortcuts(rows, context, snapshot, bindings, combat);
        }
        return rows;
    }
}
