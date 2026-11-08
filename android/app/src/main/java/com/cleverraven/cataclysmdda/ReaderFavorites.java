package com.cleverraven.cataclysmdda;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Reader-only shortcut list. Game saves and gameplay keybindings are untouched.
 * Store stable action/group targets, not menu positions or translated labels.
 */
final class ReaderFavorites {
    private static final String KEY = "hub_favorites_v1";
    private static final int MAX_FAVORITES = 64;
    private final SharedPreferences store;

    static final class Favorite {
        final String target;
        final String label;

        Favorite(String target, String label) {
            this.target = target;
            this.label = label;
        }
    }

    ReaderFavorites(Context context) {
        store = context.getApplicationContext()
                .getSharedPreferences("internal_reader", Context.MODE_PRIVATE);
    }

    private static boolean validTarget(String target) {
        return target != null && target.length() <= 256
                && ((target.startsWith("action:") && target.length() > "action:".length())
                    || (target.startsWith("hub:") && target.length() > "hub:".length()
                        && !"hub:favorites".equals(target)));
    }

    synchronized List<Favorite> items() {
        List<Favorite> result = new ArrayList<>();
        JSONArray array;
        try {
            array = new JSONArray(store.getString(KEY, "[]"));
        } catch (JSONException malformed) {
            return result;
        }
        Set<String> seen = new HashSet<>();
        for (int i = 0; i < array.length() && result.size() < MAX_FAVORITES; i++) {
            JSONObject entry = array.optJSONObject(i);
            if (entry == null) { continue; }
            String target = entry.optString("target", "");
            String label = entry.optString("label", "");
            if (!validTarget(target) || label.isEmpty() || label.length() > 200
                    || !seen.add(target)) {
                continue;
            }
            result.add(new Favorite(target, label));
        }
        return result;
    }

    synchronized boolean contains(String target) {
        for (Favorite favorite : items()) {
            if (favorite.target.equals(target)) { return true; }
        }
        return false;
    }

    synchronized boolean add(String target, String label) {
        if (!validTarget(target) || label == null || label.isEmpty() || label.length() > 200) {
            return false;
        }
        List<Favorite> favorites = items();
        for (Favorite favorite : favorites) {
            if (favorite.target.equals(target)) { return false; }
        }
        if (favorites.size() >= MAX_FAVORITES) { return false; }
        favorites.add(new Favorite(target, label));
        persist(favorites);
        return true;
    }

    synchronized boolean remove(String target) {
        List<Favorite> favorites = items();
        boolean changed = favorites.removeIf(favorite -> favorite.target.equals(target));
        if (changed) { persist(favorites); }
        return changed;
    }

    private void persist(List<Favorite> favorites) {
        JSONArray array = new JSONArray();
        for (Favorite favorite : favorites) {
            JSONObject entry = new JSONObject();
            try {
                entry.put("target", favorite.target);
                entry.put("label", favorite.label);
            } catch (JSONException impossible) {
                throw new IllegalStateException(impossible);
            }
            array.put(entry);
        }
        store.edit().putString(KEY, array.toString()).apply();
    }
}
