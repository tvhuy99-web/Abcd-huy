package com.cleverraven.cataclysmdda;

import android.content.Context;

import java.text.Normalizer;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * Sixteen spoken entry points over live, engine-published commands.
 * This is a presentation catalogue, never a second gameplay implementation:
 * activation still passes through the native snapshot/action validation gate.
 */
final class GameActionHub {
    static final int ROOT = -1;
    static final int SEARCH = 0;
    static final int OPEN_GROUP = 1;
    static final int EXECUTE = 2;
    static final int OPEN_ALL = 3;
    static final int OPEN_TTS = 4;
    static final int OPEN_READER_SETUP = 5;
    static final int OPEN_SEARCH = 6;

    static final class Entry {
        final String label;
        final String node;
        final int kind;
        final int group;

        Entry(String label, String node, int kind, int group) {
            this.label = label;
            this.node = node;
            this.kind = kind;
            this.group = group;
        }
    }

    // Root index zero is search. Groups 1..15 are the requested sections.
    static final int[] TITLES = {
            R.string.a11yHubSearch,
            R.string.a11yHubNearby,
            R.string.a11yHubExplore,
            R.string.a11yHubMovement,
            R.string.a11yHubInteraction,
            R.string.a11yHubInventory,
            R.string.a11yHubHealth,
            R.string.a11yHubEquipment,
            R.string.a11yHubCombat,
            R.string.a11yHubCrafting,
            R.string.a11yHubCharacter,
            R.string.a11yHubMissions,
            R.string.a11yHubVehicles,
            R.string.a11yHubAutomation,
            R.string.a11yHubReader,
            R.string.a11yHubSystem
    };

    private static final String[][] ACTIONS = {
            {},
            { "interact", "open", "close", "examine", "pickup", "pickup_all", "chat",
                    "move_up", "move_down", "grab", "butcher", "smash", "look" },
            { "look", "peek", "listitems", "list_items", "map", "sky", "center", "zones" },
            { "move_up", "move_down", "cycle_move", "cycle_move_reverse", "reset_move",
                    "toggle_run", "toggle_crouch", "toggle_prone", "wait", "pause",
                    "toggle_auto_travel_mode", "toggle_auto_travel" },
            { "interact", "open", "close", "examine", "examine_and_pickup",
                    "pickup", "pickup_all", "smash", "grab", "haul", "haul_toggle",
                    "butcher", "chat", "peek" },
            { "inventory", "advanced_inv", "pickup", "pickup_all", "drop", "drop_adj",
                    "compare", "organize", "use", "use_wielded", "item_action_menu",
                    "insert_item", "unload_container", "listitems", "list_items" },
            { "eat", "open_consume", "sleep", "wait", "medical", "body_status",
                    "workout", "use", "player_data" },
            { "wear", "take_off", "wield", "sort_armor", "mend", "compare", "reload",
                    "reload_weapon", "reload_wielded", "unload", "pick_style" },
            { "fire", "fire_burst", "throw", "throw_wielded", "autoattack",
                    "select_fire_mode", "select_default_ammo", "reload",
                    "reload_weapon", "reload_wielded", "unload", "pick_style",
                    "cast_spell", "recast_spell", "toggle_safemode", "toggle_autosafe",
                    "ignore_enemy", "whitelist_enemy", "safemode" },
            { "craft", "recraft", "longcraft", "construct", "disassemble",
                    "mend", "zones" },
            { "player_data", "mutations", "bionics", "morale", "medical",
                    "body_status", "cast_spell", "pick_style", "workout" },
            { "missions", "diary", "factions", "chat", "map" },
            { "control_vehicle", "toggle_auto_travel_mode", "map",
                    "grab", "examine", "move_up", "move_down" },
            { "zones", "loot", "haul", "haul_toggle", "autopickup", "autonotes",
                    "safemode", "toggle_autosafe", "toggle_auto_features",
                    "toggle_auto_pulp_butcher", "toggle_auto_mining",
                    "toggle_auto_foraging", "toggle_auto_pickup", "toggle_thief_mode",
                    "open_distraction_manager" },
            { "messages", "help", "HELP_KEYBINDINGS", "look", "map",
                    "player_data", "diary" },
            { "save", "quicksave", "quickload", "main_menu", "options",
                    "open_autopickup", "open_autonotes", "open_safemode",
                    "open_color", "open_world_mods", "open_distraction_manager",
                    "help", "HELP_KEYBINDINGS", "debug", "export_bug_report_archive",
                    "toggle_panel_adm", "toggle_pixel_minimap", "toggle_fullscreen",
                    "zoom_in", "zoom_out" }
    };

    private static String normalized(String source) {
        String plain = Normalizer.normalize(source, Normalizer.Form.NFD)
                .replaceAll("\\p{M}+", "")
                .replace('đ', 'd').replace('Đ', 'D');
        return plain.toLowerCase(Locale.ROOT).trim();
    }

    private static void addNode(List<Entry> result, ReaderSnapshot snapshot, String id,
                                Set<String> seen) {
        if (!seen.add(id)) { return; }
        ReaderSnapshot.Node node = snapshot.find(id);
        if (node != null && (node.actions.contains("activate") || "document".equals(node.role))) {
            result.add(new Entry(node.label, id, EXECUTE, 0));
        }
    }

    private static void addCommandMatches(List<Entry> result, ReaderSnapshot snapshot,
                                          String query) {
        String needle = normalized(query);
        for (ReaderSnapshot.Node node : snapshot.nodes) {
            if (!node.actions.contains("activate") && !"document".equals(node.role)) {
                continue;
            }
            if (!needle.isEmpty() && !normalized(node.label + " " + node.value + " "
                    + node.detail).contains(needle)) {
                continue;
            }
            result.add(new Entry(node.label, node.id, EXECUTE, 0));
        }
    }

    static List<Entry> entries(Context context, ReaderSnapshot snapshot, int section, String query) {
        List<Entry> result = new ArrayList<>();
        if (section == ROOT) {
            for (int i = 0; i < TITLES.length; i++) {
                result.add(new Entry(context.getString(TITLES[i]), "",
                        i == 0 ? OPEN_SEARCH : OPEN_GROUP, i));
            }
            return result;
        }
        if (snapshot == null) { return result; }
        if (section == SEARCH) {
            addCommandMatches(result, snapshot, query);
            return result;
        }

        Set<String> seen = new HashSet<>();
        if (section == 2) {
            for (String id : new String[] { "reader:scan:north", "reader:scan:south",
                    "reader:scan:west", "reader:scan:east", "world:here" }) {
                addNode(result, snapshot, id, seen);
            }
        } else if (section == 14) {
            for (String id : new String[] { "world:status", "world:messages",
                    "reader:coordinates", "world:here", "world:last_outcome",
                    "world:alert_history" }) {
                addNode(result, snapshot, id, seen);
            }
            result.add(new Entry(context.getString(R.string.a11yHubAllCommands),
                    "", OPEN_ALL, 0));
        } else if (section == 15) {
            result.add(new Entry(context.getString(R.string.a11yHubTts),
                    "", OPEN_TTS, 0));
            result.add(new Entry(context.getString(R.string.a11yHubReaderSetup),
                    "", OPEN_READER_SETUP, 0));
        }
        for (String id : ACTIONS[section]) { addNode(result, snapshot, id, seen); }

        // Any new native commands remain discoverable via the unfiltered
        // "All commands" catalogue even if they have no curated category.
        return result;
    }

    private GameActionHub() {}
}
