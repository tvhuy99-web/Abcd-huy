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
    static final int OPEN_DIRECTIONS = 7;
    static final int OPEN_FAVORITES = 8;
    // Keep native group numbers stable for existing saved gesture targets.
    static final int DIRECTIONS = 16;
    static final int FAVORITES = 17;

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

        /** Stable identity shared by the same native command in all groups. */
        String shortcutTarget() {
            switch (kind) {
                case OPEN_GROUP: return "hub:group:" + group;
                case OPEN_SEARCH: return "hub:search";
                case OPEN_ALL: return "hub:all";
                case OPEN_TTS: return "hub:tts";
                case OPEN_READER_SETUP: return "hub:reader_setup";
                case OPEN_DIRECTIONS: return "hub:directions";
                case OPEN_FAVORITES: return "hub:favorites";
                case EXECUTE: return node.isEmpty() ? null : "action:" + node;
                default: return null;
            }
        }
    }

    // Group IDs 1..15 and SEARCH=0 remain stable for saved shortcuts.
    // The root *display order* is Favorites, groups 1..15, then Search.
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

    private static final String[] DIRECTION_ACTIONS = {
            "UP", "RIGHTUP", "RIGHT", "RIGHTDOWN",
            "DOWN", "LEFTDOWN", "LEFT", "LEFTUP"
    };
    private static final int[] DIRECTION_LABELS = {
            R.string.a11yHubMoveNorth, R.string.a11yHubMoveNortheast,
            R.string.a11yHubMoveEast, R.string.a11yHubMoveSoutheast,
            R.string.a11yHubMoveSouth, R.string.a11yHubMoveSouthwest,
            R.string.a11yHubMoveWest, R.string.a11yHubMoveNorthwest
    };

    private static final String[][] ACTIONS = {
            {},
            { "interact", "open", "close", "examine", "pickup", "pickup_all", "chat",
                    "LEVEL_UP", "LEVEL_DOWN", "grab", "butcher", "smash", "look" },
            { "listitems", "reader:places", "map", "sky", "center", "zones" },
            { "LEVEL_UP", "LEVEL_DOWN", "open_movement", "cycle_move",
                    "cycle_move_reverse", "reset_move", "toggle_run", "toggle_crouch",
                    "toggle_prone", "wait", "pause", "auto_travel_mode" },
            { "interact", "open", "close", "examine", "examine_and_pickup",
                    "reader:pickup_here", "pickup", "pickup_all", "smash", "grab", "haul", "haul_toggle",
                    "butcher", "chat", "peek" },
            { "inventory", "advinv", "pickup", "pickup_all", "drop", "drop_adj",
                    "compare", "organize", "apply", "apply_wielded", "item_action_menu",
                    "insert", "unload_container", "listitems" },
            { "eat", "open_consume", "sleep", "wait", "medical", "bodystatus",
                    "workout", "apply", "player_data" },
            { "wear", "take_off", "wield", "sort_armor", "mend", "compare", "reload_item",
                    "reload_weapon", "reload_wielded", "unload", "pick_style" },
            { "fire", "fire_burst", "throw", "throw_wielded", "autoattack",
                    "select_fire_mode", "select_default_ammo", "reload_item",
                    "reload_weapon", "reload_wielded", "unload", "pick_style",
                    "cast_spell", "recast_spell", "safemode", "autosafe",
                    "ignore_enemy", "whitelist_enemy", "safemode" },
            { "craft", "recraft", "long_craft", "construct", "disassemble",
                    "mend", "zones" },
            { "player_data", "mutations", "bionics", "morale", "medical",
                    "bodystatus", "cast_spell", "pick_style", "workout" },
            { "missions", "diary", "factions", "chat", "map" },
            { "control_vehicle", "auto_travel_mode", "map",
                    "grab", "examine", "LEVEL_UP", "LEVEL_DOWN" },
            { "zones", "loot", "haul", "haul_toggle", "open_autopickup", "open_autonotes",
                    "safemode", "autosafe", "toggle_auto_features",
                    "toggle_auto_pulp_butcher", "toggle_auto_mining",
                    "toggle_auto_foraging", "toggle_auto_pickup", "toggle_thief_mode",
                    "open_distraction_manager" },
            { "messages", "help", "HELP_KEYBINDINGS", "look", "map",
                    "player_data", "diary" },
            { "save", "quicksave", "quickload", "main_menu", "open_options",
                    "open_autopickup", "open_autonotes", "open_safemode",
                    "open_color", "open_world_mods", "open_distraction_manager",
                    "help", "HELP_KEYBINDINGS", "debug", "debug",
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
        if (node != null && (node.actions.contains("activate")
                || "document".equals(node.role) || "world:here".equals(id))) {
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

    private static Entry fromFavorite(Context context, ReaderSnapshot snapshot,
                                      ReaderFavorites.Favorite favorite) {
        String target = favorite.target;
        if (target.startsWith("action:")) {
            String action = target.substring("action:".length());
            ReaderSnapshot.Node node = snapshot == null ? null : snapshot.find(action);
            // Preserve saved favorites outside the context where they were
            // selected. Do not claim that an absent command is executable.
            String label = node == null ? favorite.label : node.label;
            if (node == null || !node.enabled || (!node.actions.contains("activate")
                    && !"document".equals(node.role))) {
                label += " (" + context.getString(R.string.a11yHubFavoriteUnavailable) + ")";
            }
            return new Entry(label, action, EXECUTE, 0);
        }
        if (target.startsWith("hub:group:")) {
            try {
                int group = Integer.parseInt(target.substring("hub:group:".length()));
                if (group > 0 && group < TITLES.length) {
                    return new Entry(context.getString(TITLES[group]), "", OPEN_GROUP, group);
                }
            } catch (NumberFormatException ignored) {
                return null;
            }
        }
        switch (target) {
            case "hub:search":
                return new Entry(context.getString(TITLES[SEARCH]), "", OPEN_SEARCH, SEARCH);
            case "hub:all":
                return new Entry(context.getString(R.string.a11yHubAllCommands), "", OPEN_ALL, 0);
            case "hub:tts":
                return new Entry(context.getString(R.string.a11yHubTts), "", OPEN_TTS, 0);
            case "hub:reader_setup":
                return new Entry(context.getString(R.string.a11yHubReaderSetup), "", OPEN_READER_SETUP, 0);
            case "hub:directions":
                return new Entry(context.getString(R.string.a11yHubChooseDirection), "",
                        OPEN_DIRECTIONS, 0);
            default:
                return null;
        }
    }

    static List<Entry> entries(Context context, ReaderSnapshot snapshot, int section,
                               String query, ReaderFavorites favorites) {
        List<Entry> result = new ArrayList<>();
        if (section == ROOT) {
            result.add(new Entry(context.getString(R.string.a11yHubFavorites),
                    "", OPEN_FAVORITES, FAVORITES));
            for (int i = 1; i < TITLES.length; i++) {
                result.add(new Entry(context.getString(TITLES[i]), "", OPEN_GROUP, i));
            }
            result.add(new Entry(context.getString(TITLES[SEARCH]), "", OPEN_SEARCH, SEARCH));
            return result;
        }
        if (section == FAVORITES) {
            for (ReaderFavorites.Favorite favorite : favorites.items()) {
                Entry entry = fromFavorite(context, snapshot, favorite);
                if (entry != null) { result.add(entry); }
            }
            return result;
        }
        if (snapshot == null) { return result; }
        if (section == SEARCH) {
            addCommandMatches(result, snapshot, query);
            return result;
        }
        if (section == DIRECTIONS) {
            for (int i = 0; i < DIRECTION_ACTIONS.length; i++) {
                ReaderSnapshot.Node node = snapshot.find(DIRECTION_ACTIONS[i]);
                if (node != null && node.enabled && node.actions.contains("activate")) {
                    result.add(new Entry(context.getString(DIRECTION_LABELS[i]),
                            DIRECTION_ACTIONS[i], EXECUTE, 0));
                }
            }
            return result;
        }

        Set<String> seen = new HashSet<>();
        if (section == 3) {
            for (String direction : DIRECTION_ACTIONS) {
                ReaderSnapshot.Node available = snapshot.find(direction);
                if (available != null && available.enabled
                        && available.actions.contains("activate")) {
                    result.add(new Entry(context.getString(R.string.a11yHubChooseDirection),
                            "", OPEN_DIRECTIONS, 0));
                    break;
                }
            }
        }
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
