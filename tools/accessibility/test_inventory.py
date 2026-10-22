import unittest

from inventory import scan


class InventoryTest(unittest.TestCase):
    def test_comments_and_quoted_documentation_are_not_registrations(self):
        source = '''// register_action( "BAD1" );
        /* register_action( "BAD2" ); */
        const char *doc = "register_action( \\\"BAD3\\\" )";
        const char *raw = R"doc(register_action( "BAD4" ))doc";
        ctxt.register_action( "GOOD" );'''
        entries = list(scan('src/test.cpp', source))
        self.assertEqual([(x['value'], x['line']) for x in entries], [('GOOD', 5)])

    def test_dynamic_registration_is_not_certified(self):
        entries = list(scan('src/test.cpp', 'ctxt.register_action( action_id );'))
        self.assertEqual(entries[0]['resolution'], 'manual_review')
        self.assertEqual(entries[0]['support'], 'not_assessed')

    def test_comment_markers_inside_strings_preserve_following_code(self):
        entries = list(scan('src/test.cpp', 'auto s = "https://example/*";\nctxt.register_action( "UP" );'))
        self.assertEqual(entries[0]['value'], 'UP')
        self.assertEqual(entries[0]['line'], 2)

    def test_context_and_native_screen_candidates(self):
        source = 'input_context ctxt( "WORLD" );\nnew AlertDialog.Builder(activity);'
        entries = list(scan('screen', source))
        self.assertEqual([x['kind'] for x in entries], ['input_context', 'native_dialog'])


if __name__ == '__main__':
    unittest.main()
