"""Check upstream recursive item reconciliation and fork's map-load optimizations."""

from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[2] / "src"


class MapLoadedItemReconciliationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = (SRC / "map.cpp").read_text(encoding="utf-8")
        begin = text.index("void map::reconcile_loaded_items(")
        end = text.index("void map::shift_traps(", begin)
        cls.method = text[begin:end]

    def test_visit_nested_map_items_without_recursing_into_tinymaps(self):
        self.assertIn("item_location( cur, &it ).visit_items( reconcile_recursive );",
                      self.method)
        self.assertNotIn("map_cursor( this, p ).visit_items(", self.method)
        self.assertNotIn("walk_recursive", self.method)

    def test_skip_empty_map_tiles_before_constructing_cursor(self):
        self.assertIn("auto &tile_items = sm->get_items( { sx, sy } );",
                      self.method)
        self.assertIn("if( tile_items.empty() )", self.method)
        self.assertIn("for( item &it : tile_items )", self.method)
        self.assertIn("item_location( cur, &it ).visit_items( reconcile_recursive );",
                      self.method)

    def test_every_vehicle_part_and_nested_item_is_visited(self):
        self.assertIn("for( const vpart_reference &vpr : wv.v->get_all_parts() )", self.method)
        self.assertIn("vehicle_cursor( *wv.v, vpr.part_index() ).visit_items( reconcile_recursive );",
                      self.method)

    def test_preserve_uniform_submap_skip_and_craft_reservations(self):
        for token in ("sm == nullptr || sm->is_uniform()",
                      "index.rebuild_for_craft( loc );",
                      "wakeups.rebuild_for_item( loc );",
                      "reconcile_scope::full_rebuild",
                      "std::vector<item_location> carried;",
                      "VisitResponse::NEXT"):
            with self.subTest(token=token):
                self.assertIn(token, self.method)


if __name__ == "__main__":
    unittest.main()
