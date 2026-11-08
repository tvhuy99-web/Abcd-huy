package com.cleverraven.cataclysmdda;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** Immutable semantic copy. No native object handles or hidden gameplay state. */
public final class ReaderSnapshot {
    // Structural guardrails mirroring a11y_semantics.h (R02.4). A snapshot
    // beyond these limits is malformed, never silently truncated.
    public static final int MAX_NODES = 4096;
    public static final int MAX_TABLES = 16;
    public static final int MAX_COLUMNS = 16;
    public static final int MAX_CELLS = 16;
    public static final int MAX_CHOICES = 128;
    public static final int MAX_TREE_DEPTH = 16;

    public final long session;
    public final long frame;
    public final long generation;
    public final long revision;
    public final String title;
    public final String summary;
    public final String category;
    public final String focus;
    /** Engine permission for walking swipe gestures (R00.2); absent means denied. */
    public final boolean swipeMovementAllowed;
    public final List<Table> tables;
    public final List<Node> nodes;
    private final Map<String, Table> tablesById;
    private final Map<String, Node> nodesById;

    /** One table column declaration (R02.2). */
    public static final class Column {
        public final String id;
        public final String header;
        public final String unit;

        private Column(JSONObject object) throws JSONException {
            id = object.getString("id");
            header = object.optString("header");
            unit = object.optString("unit");
        }
    }

    /** A declared table: its rows are nodes whose tableId names it. */
    public static final class Table {
        public final String id;
        public final List<Column> columns;
        private final Set<String> columnIds;

        private Table(JSONObject object) throws JSONException {
            id = object.getString("id");
            JSONArray array = object.optJSONArray("columns");
            ArrayList<Column> parsed = new ArrayList<>();
            HashSet<String> ids = new HashSet<>();
            if (array != null) {
                if (array.length() > MAX_COLUMNS) {
                    throw new JSONException("Too many table columns");
                }
                for (int i = 0; i < array.length(); i++) {
                    Column column = new Column(array.getJSONObject(i));
                    if (column.id.isEmpty() || !ids.add(column.id)) {
                        throw new JSONException("Invalid table column identity");
                    }
                    parsed.add(column);
                }
            }
            columns = Collections.unmodifiableList(parsed);
            columnIds = Collections.unmodifiableSet(ids);
        }

        public Column column(String columnId) {
            for (Column column : columns) {
                if (column.id.equals(columnId)) {
                    return column;
                }
            }
            return null;
        }
    }

    /** One presentation cell of a table row node (R02.2). Never actionable. */
    public static final class Cell {
        public final String column;
        public final String value;
        public final String unit;
        public final String compare;

        private Cell(JSONObject object) throws JSONException {
            column = object.getString("column");
            value = object.optString("value");
            unit = object.optString("unit");
            compare = object.optString("compare");
        }
    }

    /** One selectable value of a form field (R02.3). */
    public static final class Choice {
        public final String value;
        public final String label;

        private Choice(JSONObject object) throws JSONException {
            value = object.getString("value");
            label = object.optString("label");
        }
    }

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
        // Tree contract (R02.1); depth -1 means a flat node.
        public final String parentId;
        public final int depth;
        public final boolean hasChildren;
        public final boolean expanded;
        // Table contract (R02.2); tableId empty means not a row.
        public final String tableId;
        public final int rowIndex;
        public final List<Cell> cells;
        // Form contract (R02.3); fieldType empty means not a field.
        public final String fieldType;
        public final String appliedValue;
        public final boolean hasFieldRange;
        public final double fieldMin;
        public final double fieldMax;
        public final double fieldStep;
        public final List<Choice> choices;
        public final boolean hasDraft;
        public final String draftValue;
        public final String fieldError;
        public final List<String> dependsOn;

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
            parentId = object.optString("parentId");
            depth = object.optInt("depth", -1);
            hasChildren = object.optBoolean("hasChildren");
            expanded = object.optBoolean("expanded");
            tableId = object.optString("tableId");
            rowIndex = object.optInt("rowIndex", -1);
            fieldType = object.optString("fieldType");
            appliedValue = object.optString("appliedValue");
            hasFieldRange = object.has("fieldMin") && object.has("fieldMax");
            fieldMin = object.optDouble("fieldMin", 0);
            fieldMax = object.optDouble("fieldMax", 0);
            fieldStep = object.optDouble("fieldStep", 0);
            hasDraft = object.has("draftValue");
            draftValue = object.optString("draftValue");
            fieldError = object.optString("fieldError");
            ArrayList<String> parsed = new ArrayList<>();
            JSONArray array = object.optJSONArray("actions");
            if (array != null) {
                for (int i = 0; i < array.length(); i++) {
                    parsed.add(array.getString(i));
                }
            }
            actions = Collections.unmodifiableList(parsed);
            ArrayList<Cell> parsedCells = new ArrayList<>();
            JSONArray cellArray = object.optJSONArray("cells");
            if (cellArray != null) {
                if (cellArray.length() > MAX_CELLS) {
                    throw new JSONException("Too many table cells");
                }
                for (int i = 0; i < cellArray.length(); i++) {
                    parsedCells.add(new Cell(cellArray.getJSONObject(i)));
                }
            }
            cells = Collections.unmodifiableList(parsedCells);
            ArrayList<Choice> parsedChoices = new ArrayList<>();
            JSONArray choiceArray = object.optJSONArray("choices");
            if (choiceArray != null) {
                if (choiceArray.length() > MAX_CHOICES) {
                    throw new JSONException("Too many form choices");
                }
                HashSet<String> choiceValues = new HashSet<>();
                for (int i = 0; i < choiceArray.length(); i++) {
                    Choice choice = new Choice(choiceArray.getJSONObject(i));
                    if (!choiceValues.add(choice.value)) {
                        throw new JSONException("Duplicate form choice value");
                    }
                    parsedChoices.add(choice);
                }
            }
            choices = Collections.unmodifiableList(parsedChoices);
            ArrayList<String> parsedDependencies = new ArrayList<>();
            JSONArray dependencyArray = object.optJSONArray("dependsOn");
            if (dependencyArray != null) {
                for (int i = 0; i < dependencyArray.length(); i++) {
                    parsedDependencies.add(dependencyArray.getString(i));
                }
            }
            dependsOn = Collections.unmodifiableList(parsedDependencies);
            if (id.isEmpty() || id.length() > 512) {
                throw new JSONException("Invalid semantic node identity");
            }
            if (depth < -1 || depth > MAX_TREE_DEPTH) {
                throw new JSONException("Invalid tree depth");
            }
            if (!fieldType.isEmpty()
                    && !"bool".equals(fieldType) && !"int".equals(fieldType)
                    && !"float".equals(fieldType) && !"choice".equals(fieldType)
                    && !"text".equals(fieldType)) {
                throw new JSONException("Unknown form field type");
            }
            if (hasFieldRange && (fieldMin > fieldMax || fieldStep < 0)) {
                throw new JSONException("Invalid form field range");
            }
        }

        /** The raw value a draft edit starts from: the draft if one is pending. */
        public String effectiveRawValue() {
            return hasDraft ? draftValue : appliedValue;
        }

        public Choice choiceFor(String rawValue) {
            for (Choice choice : choices) {
                if (choice.value.equals(rawValue)) {
                    return choice;
                }
            }
            return null;
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
        swipeMovementAllowed = object.optBoolean("swipeMovementAllowed", false);
        ArrayList<Table> parsedTables = new ArrayList<>();
        HashMap<String, Table> tableIndex = new HashMap<>();
        JSONArray tableArray = object.optJSONArray("tables");
        if (tableArray != null) {
            if (tableArray.length() > MAX_TABLES) {
                throw new JSONException("Too many tables");
            }
            for (int i = 0; i < tableArray.length(); i++) {
                Table table = new Table(tableArray.getJSONObject(i));
                if (table.id.isEmpty() || tableIndex.put(table.id, table) != null) {
                    throw new JSONException("Duplicate table identity");
                }
                parsedTables.add(table);
            }
        }
        tables = Collections.unmodifiableList(parsedTables);
        tablesById = Collections.unmodifiableMap(tableIndex);
        JSONArray array = object.getJSONArray("nodes");
        if (array.length() > MAX_NODES) {
            throw new JSONException("Snapshot has too many nodes");
        }
        ArrayList<Node> parsed = new ArrayList<>();
        HashMap<String, Node> index = new HashMap<>();
        for (int i = 0; i < array.length(); i++) {
            Node node = new Node(array.getJSONObject(i));
            if (index.put(node.id, node) != null) {
                throw new JSONException("Duplicate semantic node identity");
            }
            parsed.add(node);
        }
        nodes = Collections.unmodifiableList(parsed);
        nodesById = Collections.unmodifiableMap(index);
        validateStructure();
    }

    // Mirrors a11y::validate_snapshot: a structurally impossible snapshot is
    // malformed here too, so the controller can never navigate a tree with
    // a missing parent, address a row of an undeclared table, or edit a
    // draft that references an unknown field (R02.4).
    private void validateStructure() throws JSONException {
        Set<String> parentsWithChildren = new HashSet<>();
        for (Node node : nodes) {
            if (!node.parentId.isEmpty()) {
                Node parent = nodesById.get(node.parentId);
                if (parent == null) {
                    throw new JSONException("Missing tree parent");
                }
                if (!parent.expanded) {
                    throw new JSONException("Child of a collapsed parent");
                }
                if (parent.depth < 0 || node.depth != parent.depth + 1) {
                    throw new JSONException("Inconsistent tree depth");
                }
                parentsWithChildren.add(node.parentId);
            } else if (node.depth > 0) {
                throw new JSONException("Inconsistent tree depth");
            }
        }
        for (Node node : nodes) {
            Set<String> chain = new HashSet<>();
            Node cursor = node;
            while (!cursor.parentId.isEmpty()) {
                if (!chain.add(cursor.id)) {
                    throw new JSONException("Tree cycle");
                }
                cursor = nodesById.get(cursor.parentId);
            }
            if (node.hasChildren && node.expanded && !parentsWithChildren.contains(node.id)) {
                throw new JSONException("Expanded parent without children");
            }
            if (!node.tableId.isEmpty()) {
                Table table = tablesById.get(node.tableId);
                if (table == null) {
                    throw new JSONException("Undeclared table");
                }
                if (node.rowIndex < 0) {
                    throw new JSONException("Invalid table row index");
                }
                for (Cell cell : node.cells) {
                    if (!table.columnIds.contains(cell.column)) {
                        throw new JSONException("Undeclared table column");
                    }
                }
            }
            for (String dependency : node.dependsOn) {
                if (dependency.equals(node.id) || !nodesById.containsKey(dependency)) {
                    throw new JSONException("Invalid field dependency");
                }
            }
        }
        Set<String> seenRows = new HashSet<>();
        for (Node node : nodes) {
            if (!node.tableId.isEmpty()
                    && !seenRows.add(node.tableId + "\n" + node.rowIndex)) {
                throw new JSONException("Duplicate table row");
            }
        }
    }

    public Table table(String id) {
        return tablesById.get(id);
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
        return nodesById.get(id);
    }
}
