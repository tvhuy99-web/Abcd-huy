package com.cleverraven.cataclysmdda;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Immutable semantic copy. No native object handles or hidden gameplay state. */
public final class ReaderSnapshot {
    public final long session;
    public final long frame;
    public final long generation;
    public final long revision;
    public final String title;
    public final String summary;
    public final String category;
    public final String focus;
    public final List<Node> nodes;

    public static final class Node {
        public final String id;
        public final String role;
        public final String label;
        public final String value;
        public final String detail;
        public final String disabledReason;
        public final boolean enabled;
        public final boolean selected;
        public final boolean checked;
        public final int position;
        public final int total;
        public final List<String> actions;

        private Node(JSONObject object) throws JSONException {
            id = object.getString("id");
            role = object.optString("role", "button");
            label = object.getString("label");
            value = object.optString("value");
            detail = object.optString("detail");
            disabledReason = object.optString("disabledReason");
            enabled = object.optBoolean("enabled", true);
            selected = object.optBoolean("selected");
            checked = object.optBoolean("checked");
            position = object.optInt("position");
            total = object.optInt("total");
            ArrayList<String> parsed = new ArrayList<>();
            JSONArray array = object.optJSONArray("actions");
            if (array != null) {
                for (int i = 0; i < array.length(); i++) {
                    parsed.add(array.getString(i));
                }
            }
            actions = Collections.unmodifiableList(parsed);
            if (id.isEmpty() || id.length() > 512) {
                throw new JSONException("Invalid semantic node identity");
            }
        }
    }

    public ReaderSnapshot(JSONObject object) throws JSONException {
        session = positive(object, "session");
        frame = positive(object, "frame");
        generation = positive(object, "generation");
        revision = positive(object, "revision");
        title = object.optString("title");
        summary = object.optString("summary");
        category = object.optString("category");
        focus = object.optString("focus");
        JSONArray array = object.getJSONArray("nodes");
        ArrayList<Node> parsed = new ArrayList<>();
        Set<String> identities = new HashSet<>();
        for (int i = 0; i < array.length(); i++) {
            Node node = new Node(array.getJSONObject(i));
            if (!identities.add(node.id)) {
                throw new JSONException("Duplicate semantic node identity");
            }
            parsed.add(node);
        }
        nodes = Collections.unmodifiableList(parsed);
    }

    private static long positive(JSONObject object, String key) throws JSONException {
        try {
            long value = Long.parseLong(object.getString(key));
            if (value > 0) {
                return value;
            }
        } catch (NumberFormatException ignored) {
            // Report a malformed snapshot, never truncate an identity.
        }
        throw new JSONException("Invalid semantic token: " + key);
    }

    public String contextKey() {
        return session + ":" + frame + ":" + generation;
    }

    public Node find(String id) {
        for (Node node : nodes) {
            if (node.id.equals(id)) {
                return node;
            }
        }
        return null;
    }
}
