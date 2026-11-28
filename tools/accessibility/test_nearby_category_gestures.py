"""Native-category gesture filters for Android surroundings exploration.

These guards protect the native route-to-target action while introducing
categorical browsing without a separate filter button.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NATIVE = ROOT / "src/surroundings_menu.cpp"
JAVA = ROOT / "android/app/src/main/java/com/cleverraven/cataclysmdda"
VI = ROOT / "lang/po/accessibility/vi.po"


class NearbyCategoryGestures(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native = NATIVE.read_text(encoding="utf-8").split(
            "void surroundings_menu::execute_accessible()", 1)[1].split(
            "void surroundings_menu::execute()", 1)[0]
        cls.surface = (JAVA / "ReaderGestureSurface.java").read_text(encoding="utf-8")
        cls.reader = (JAVA / "GameReaderController.java").read_text(encoding="utf-8")
        cls.vi = VI.read_text(encoding="utf-8")

    def test_native_categories_with_uncategorized_still_in_all(self):
        for token in ('nearby_kind { unclassified, creature, environment, item }',
                      'category_indices[nearby_category::all].push_back( i )',
                      'category_indices[nearby_category::creatures].push_back( i )',
                      'category_indices[nearby_category::environment].push_back( i )',
                      'category_indices[nearby_category::items].push_back( i )',
                      'entry.first == nearby_category::all',
                      '!indices.empty()'):
            self.assertIn(token, self.native)
        self.assertIn('nearby_kind::unclassified', self.native)
        self.assertIn('nearby_kind::creature', self.native)
        self.assertIn('nearby_kind::item', self.native)
        self.assertIn('nearby_kind::environment', self.native)
        self.assertIn('sounds::sound_at( position )', self.native)
        self.assertIn('observed.special_perceptions', self.native)
        self.assertIn('const std::string &creature_name : tile.creatures', self.native)
        self.assertIn('for( const item &obj : here.i_at( position ) )', self.native)

    def test_item_categories_follow_inventory_and_have_vietnamese_labels(self):
        import json

        self.assertIn('group.categories.insert( &obj.get_category_of_contents() );', self.native)
        self.assertNotIn('item_category_for', self.native)
        self.assertNotIn('get_category_shallow()', self.native)
        self.assertIn('return *lhs < *rhs;', self.native)
        self.assertIn('category.name_header()', self.native)
        self.assertIn('pgettext( "nearby item category", id.c_str() )', self.native)
        for category in json.loads((ROOT / "data/json/item_category.json").read_text("utf-8")):
            self.assertIn('msgctxt "nearby item category"\nmsgid "' + category["id"] + '"',
                          self.vi)
        self.assertIn('msgctxt "corpse ownership qualifier"\nmsgid "%1$s of a %2$s"\n'
                      'msgstr "%1$s của %2$s"', self.vi)

    def test_vertical_swipes_switch_categories_only_in_surroundings(self):
        self.assertIn('default boolean handleSurroundingsSwipe(int horizontal, int vertical)', self.surface)
        self.assertIn('listener.handleSurroundingsSwipe(horizontal, vertical)', self.surface)
        self.assertIn('public boolean handleSurroundingsSwipe(int horizontal, int vertical)', self.reader)
        self.assertIn('!"surroundings".equals(snapshot.category)', self.reader)
        self.assertIn('vertical < 0', self.reader)
        self.assertIn('"nearby:filter:next" : "nearby:filter:previous"', self.reader)
        self.assertIn('selected_category +', self.native)
        self.assertIn(') % count', self.native)
        self.assertIn('const std::size_t count = active_categories.size();', self.native)
        self.assertIn('page = 0;', self.native)

    def test_left_right_focus_near_to_far_and_browse_pages(self):
        self.assertIn('navigate(horizontal > 0 ? 1 : -1);', self.reader)
        self.assertIn('nearbyFirstItemPending = true;', self.reader)
        self.assertIn('if (horizontal > 0 && nearbyFirstItemPending)', self.reader)
        self.assertIn('nearbyFirstItemPending = false;', self.reader)
        self.assertIn('readCurrent();', self.reader)
        self.assertIn('nearby:page:next', self.reader)
        self.assertIn('nearby:page:previous', self.reader)
        self.assertIn('index() == filtered.size() - 1', self.reader)
        self.assertIn('index() == 0', self.reader)
        self.assertIn('const std::size_t row_index = filtered[i];', self.native)
        self.assertIn('nearby[ row_index ]', self.native.replace('nearby[row_index]', 'nearby[ row_index ]'))
        self.assertIn('std::stable_sort( nearby.begin(), nearby.end()', self.native)
        self.assertIn('return lhs.distance < rhs.distance;', self.native)
        self.assertIn('constexpr std::size_t page_size = 80;', self.native)
        self.assertIn('const std::size_t page_count = ( filtered.size() + page_size - 1 ) / page_size;',
                      self.native)
        self.assertIn('page = ( page + 1 ) % page_count;', self.native)
        self.assertIn('page = ( page + page_count - 1 ) % page_count;', self.native)
        self.assertIn('if( filtered.size() > page_size )', self.native)
        self.assertIn('previous.role = "command";', self.native)
        self.assertIn('next.role = "command";', self.native)

    def test_speaks_category_counts_without_repeating_preamble(self):
        self.assertIn('snapshot.summary = filtered.empty() ?', self.native)
        self.assertIn('_( "%1$s. %2$d entries." )', self.native)
        self.assertIn('_( "%s. No entries." )', self.native)
        self.assertIn('boolean nearbyCategoryChanged =', self.reader)
        self.assertIn('focus = next.find(next.focus) != null ? next.focus', self.reader)
        self.assertIn('feedback((enteredMenu ? next.title + ". " : "") + next.summary);', self.reader)
        self.assertIn('node.value = relative_words( entry.relative );', self.native)
        for translation in ('Tất cả', 'Sinh vật và nhân vật không do người chơi điều khiển', 'Địa điểm và môi trường',
                            'Vật phẩm', 'Không có mục nào'):
            self.assertIn(translation, self.vi)

    def test_double_tap_route_to_selected_object_is_unchanged(self):
        # Filter and page handlers only set local UI state; the existing
        # hazard-aware game route is still the single activation path.
        self.assertIn('g->safe_route_to( you, target.target, target.stop_distance,', self.native)
        self.assertIn('you.set_destination( *route );', self.native)
        self.assertIn('if( !route )', self.native)
        self.assertIn('if( route->empty() )', self.native)
        self.assertIn('return std::string( "QUIT" );', self.native)
        self.assertIn('nearby:target:', self.native)
        self.assertIn('const nearby_row &target = nearby[row_index];', self.native)
        self.assertIn('tile.impassable ? 1 : 0', self.native)
        self.assertIn('add_row( creature_name, relative, 1, nearby_kind::creature,', self.native)
        self.assertIn('add_row( label, relative, 0, nearby_kind::item,', self.native)


if __name__ == "__main__":
    unittest.main()
