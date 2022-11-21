import sys
sys.path.append("./generated")

import unittest

import winrt.windows.foundation as wf

class TestUri(unittest.TestCase):

    def test_activate_uri(self):
        u = wf.Uri("http://microsoft.com")
        self.assertEqual(u.domain, "microsoft.com")
        self.assertEqual(u.absolute_canonical_uri, "http://microsoft.com/")
        self.assertEqual(u.port, 80)
        self.assertEqual(u.scheme_name, "http")
        self.assertEqual(u.suspicious, False)
        self.assertEqual(u.path, "/")
        self.assertEqual(u.query, "")
        self.assertEqual(u.query_parsed.size, 0)


    def test_activate_uri2(self):
        u = wf.Uri("http://microsoft.com", "surface/studio")
        self.assertEqual(u.domain, "microsoft.com")
        self.assertEqual(u.absolute_canonical_uri, "http://microsoft.com/surface/studio")
        self.assertEqual(u.port, 80)
        self.assertEqual(u.scheme_name, "http")
        self.assertEqual(u.suspicious, False)
        self.assertEqual(u.path, "/surface/studio")
        self.assertEqual(u.query, "")
        self.assertEqual(u.query_parsed.size, 0)


    def test_combine_uri(self):
        u1 = wf.Uri("http://microsoft.com")
        u = u1.combine_uri("surface/studio")
        self.assertEqual(u.domain, "microsoft.com")
        self.assertEqual(u.absolute_canonical_uri, "http://microsoft.com/surface/studio")
        self.assertEqual(u.port, 80)
        self.assertEqual(u.scheme_name, "http")
        self.assertEqual(u.suspicious, False)
        self.assertEqual(u.path, "/surface/studio")
        self.assertEqual(u.query, "")
        self.assertEqual(u.query_parsed.size, 0)

    def test_activate_query_parsed(self):
        u = wf.Uri("http://microsoft.com?projection=python&platform=windows")
        self.assertEqual(u.query, "?projection=python&platform=windows")

        qp = u.query_parsed
        self.assertEqual(qp.size, 2)

        self.assertEqual(qp.get_first_value_by_name("projection"), "python")
        self.assertEqual(qp.get_first_value_by_name("platform"), "windows")

        e0 = qp.get_at(0)
        self.assertEqual(e0.name, "projection")
        self.assertEqual(e0.value, "python")

        e1 = qp.get_at(1)
        self.assertEqual(e1.name, "platform")
        self.assertEqual(e1.value, "windows")

        # t = qp.IndexOf(e0)
        # self.assertTrue(t[0])
        # self.assertEqual(t[1], 0)

    

if __name__ == '__main__':
    unittest.main()
