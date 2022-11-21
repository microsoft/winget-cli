import sys
sys.path.append("./generated")

import unittest

import winrt.windows.data.json as wdj

class TestJson(unittest.TestCase):

    def test_activate_JsonArray(self):
        a = wdj.JsonArray()
        self.assertEqual(a.size, 0)
        self.assertEqual(a.value_type, wdj.JsonValueType.ARRAY)
        self.assertEqual(a.to_string(), "[]")
        self.assertEqual(a.stringify(), "[]")

    def test_JsonArray_dunder_str(self):
        a = wdj.JsonArray()
        self.assertEqual(str(a), "[]")

    def test_JsonArray_parse(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        self.assertEqual(a.size, 5)
        for x in range(0,4):
            self.assertEqual(a.get_number_at(x), x+1)

    def test_JsonArray_seq_len(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        self.assertEqual(len(a), 5)

    def test_JsonArray_seq_get_item(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        for x in range(0,4):
            v = a[x]
            self.assertEqual(v.value_type, wdj.JsonValueType.NUMBER)
            self.assertEqual(v.get_number(), x+1)

    def test_JsonArray_seq_set_item(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        a[2] = wdj.JsonValue.create_string_value("the larch")
        v = a.get_at(2)
        self.assertEqual(v.value_type, wdj.JsonValueType.STRING)
        self.assertEqual(v.get_string(), "the larch")

    def test_JsonArray_seq_enumerate(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        for x, v in enumerate(a):
            self.assertEqual(v.value_type, wdj.JsonValueType.NUMBER)
            self.assertEqual(v.get_number(), x+1)

    def test_JsonArray_remove_at(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        self.assertEqual(a.size, 5)
        a.remove_at(0)
        self.assertEqual(a.size, 4)
        for x in range(0,3):
            self.assertEqual(a.get_number_at(x), x+2)

    def test_JsonArray_remove_at_end(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        self.assertEqual(a.size, 5)
        a.remove_at_end()
        self.assertEqual(a.size, 4)
        for x in range(0,3):
            self.assertEqual(a.get_number_at(x), x+1)

    def test_JsonArray_try_parse(self):
        succeeded, a = wdj.JsonArray.try_parse("[1,2,3,4,5]")
        self.assertTrue(succeeded)
        self.assertEqual(a.size, 5)
        for x in range(0,4):
            self.assertEqual(a.get_number_at(x), x+1)

    def test_JsonArray_try_parse_fail(self):
        succeeded, a = wdj.JsonArray.try_parse("z[1,2,3,4,5]")
        self.assertFalse(succeeded)

    def test_JsonArray_get_array_at(self):
        a = wdj.JsonArray.parse("[true, [], false]")
        v1 = a.get_array_at(1)
        self.assertEqual(v1.size, 0)
        self.assertEqual(v1.value_type, wdj.JsonValueType.ARRAY)

    def test_JsonArray_clear(self):
        a = wdj.JsonArray.parse("[1,2,3,4,5]")
        self.assertEqual(a.size, 5)
        a.clear()
        self.assertEqual(a.size, 0)

    def test_JsonArray_get_array(self):
        a = wdj.JsonArray.parse("[true, [], false]")
        v1 = a.get_array()
        self.assertEqual(v1.size, 3)
        self.assertEqual(v1.value_type, wdj.JsonValueType.ARRAY)

    def test_JsonArray_get_boolean(self):
        a = wdj.JsonArray.parse("[true, [], false]")
        with self.assertRaises(RuntimeError):
            v1 = a.get_boolean()

    def test_JsonArray_get_number(self):
        a = wdj.JsonArray.parse("[true, [], false]")
        with self.assertRaises(RuntimeError):
            v1 = a.get_number()

    def test_JsonArray_get_string(self):
        a = wdj.JsonArray.parse("[true, [], false]")
        with self.assertRaises(RuntimeError):
            v1 = a.get_string()

    def test_JsonArray_get_array_at(self):
        a = wdj.JsonArray.parse("[true, [], false]")
        v1 = a.get_array_at(1)
        self.assertEqual(v1.size, 0)
        self.assertEqual(v1.value_type, wdj.JsonValueType.ARRAY)

    def test_JsonArray_get_object(self):
        a = wdj.JsonArray.parse("[true, {}, false]")
        with self.assertRaises(RuntimeError):
            v1 = a.get_object()

    def test_JsonArray_get_object_at(self):
        a = wdj.JsonArray.parse("[true, {}, false]")
        v1 = a.get_object_at(1)
        self.assertEqual(v1.size, 0)
        self.assertEqual(v1.value_type, wdj.JsonValueType.OBJECT)

    def test_JsonArray_get_string_at(self):
        a = wdj.JsonArray.parse("[true, \"spam\", false]")
        v1 = a.get_string_at(1)
        self.assertEqual(v1, "spam")

    def test_JsonArray_get_boolean_at(self):
        a = wdj.JsonArray.parse("[true, false]")
        v1 = a.get_boolean_at(0)
        v2 = a.get_boolean_at(1)
        self.assertTrue(v1)
        self.assertFalse(v2)

    def test_JsonArray_get_at(self):
        a = wdj.JsonArray.parse("[null, true, 42, \"spam\", [1,2,3], {\"scene\":24}]")
        v = a.get_at(0)
        self.assertEqual(v.value_type, wdj.JsonValueType.NULL)
        v = a.get_at(1)
        self.assertEqual(v.value_type, wdj.JsonValueType.BOOLEAN)
        v = a.get_at(2)
        self.assertEqual(v.value_type, wdj.JsonValueType.NUMBER)
        v = a.get_at(3)
        self.assertEqual(v.value_type, wdj.JsonValueType.STRING)
        v = a.get_at(4)
        self.assertEqual(v.value_type, wdj.JsonValueType.ARRAY)
        v = a.get_at(5)
        self.assertEqual(v.value_type, wdj.JsonValueType.OBJECT)

    def test_JsonArray_index_of(self):
        a = wdj.JsonArray.parse("[null, true, 42, \"spam\", [1,2,3], {\"scene\":24}]")
        v = a.get_at(3)
        found, index = a.index_of(v)
        self.assertTrue(found)
        self.assertEqual(index, 3)

    def test_JsonArray_append(self):
        a = wdj.JsonArray.parse("[null, true, 42, \"spam\", [1,2,3], {\"scene\":24}]")
        v = wdj.JsonValue.create_string_value("the larch")
        self.assertEqual(a.size, 6)
        a.append(v)
        self.assertEqual(a.size, 7)
        found, index = a.index_of(v)
        self.assertTrue(found)
        self.assertEqual(index, 6)

    def test_JsonArray_parse(self):
        a = wdj.JsonArray.parse('[true, false, 42, null, [], {}, "plugh"]')
        self.assertEqual(a.value_type, wdj.JsonValueType.ARRAY)
        self.assertEqual(a.size, 7)
        self.assertTrue(a.get_boolean_at(0))
        self.assertFalse(a.get_boolean_at(1))
        self.assertEqual(a.get_number_at(2), 42)
        self.assertEqual(a.get_at(3).value_type, wdj.JsonValueType.NULL)
        a2 = a.get_array_at(4)
        self.assertEqual(a2.size, 0)
        self.assertEqual(a2.value_type, wdj.JsonValueType.ARRAY)
        o = a.get_object_at(5)
        self.assertEqual(o.size, 0)
        self.assertEqual(o.value_type, wdj.JsonValueType.OBJECT)
        self.assertEqual(a.get_string_at(6), "plugh")

    def test_JsonArray_GetView(self):
        a = wdj.JsonArray.parse('[true, false, 42, null, [], {}, "plugh"]')
        view = a.get_view()

        self.assertEqual(view.size, 7)
        v0 = view.get_at(0)
        self.assertTrue(v0.get_boolean())
        v1 = view.get_at(1)
        self.assertFalse(v1.get_boolean())
        v2 = view.get_at(2)
        self.assertEqual(v2.get_number(), 42)
        v6 = view.get_at(6)
        self.assertEqual(v6.get_string(), "plugh")

        v4 = view.get_at(4)
        a4 = v4.get_array()
        self.assertEqual(a4.size, 0)

        v5 = view.get_at(5)
        o5 = v5.get_object()
        self.assertEqual(o5.size, 0)

    def test_JsonObject_parse(self):
        s = "{\"bool\": true,\"null\": null,\"number\": 42,\"string\": \"plugh\",\"array\": [1,2,3,4],\"object\": {}}"
        o = wdj.JsonObject.parse(s)
        self.assertEqual(o.value_type, wdj.JsonValueType.OBJECT)
        self.assertEqual(o.size, 6)
        self.assertTrue(o.get_named_boolean("bool"))
        self.assertEqual(o.get_named_value("null").value_type, wdj.JsonValueType.NULL)
        self.assertEqual(o.get_named_number("number"), 42)
        self.assertEqual(o.get_named_string("string"), "plugh")
        a2 = o.get_named_array("array")
        self.assertEqual(a2.size, 4)
        self.assertEqual(a2.value_type, wdj.JsonValueType.ARRAY)
        o2 = o.get_named_object("object")
        self.assertEqual(o2.size, 0)
        self.assertEqual(o2.value_type, wdj.JsonValueType.OBJECT)

    def test_JsonObject_GetView(self):
        s = "{\"bool\": true,\"null\": null,\"number\": 42,\"string\": \"plugh\",\"array\": [1,2,3,4],\"object\": {}}"
        o = wdj.JsonObject.parse(s)
        v = o.get_view()

        self.assertEqual(v.size, 6)
        
        self.assertTrue(v.lookup("bool").get_boolean())
        self.assertEqual(v.lookup("null").value_type, wdj.JsonValueType.NULL)
        self.assertEqual(v.lookup("number").get_number(), 42)
        self.assertEqual(v.lookup("string").get_string(), "plugh")
        a2 = v.lookup("array").get_array()
        self.assertEqual(a2.size, 4)
        self.assertEqual(a2.value_type, wdj.JsonValueType.ARRAY)
        o2 = v.lookup("object").get_object()
        self.assertEqual(o2.size, 0)
        self.assertEqual(o2.value_type, wdj.JsonValueType.OBJECT)

    def test_JsonObject_map_len(self):
        s = "{\"bool\": true,\"null\": null,\"number\": 42,\"string\": \"plugh\",\"array\": [1,2,3,4],\"object\": {}}"
        o = wdj.JsonObject.parse(s)
        self.assertEqual(len(o), 6)

    def test_JsonObject_map_item(self):
        s = "{\"bool\": true,\"null\": null,\"number\": 42,\"string\": \"plugh\",\"array\": [1,2,3,4],\"object\": {}}"
        o = wdj.JsonObject.parse(s)
        self.assertEqual(o["string"].get_string(), "plugh")

    def test_JsonObject_map_item_assign(self):
        s = "{\"bool\": true,\"null\": null,\"number\": 42,\"string\": \"plugh\",\"array\": [1,2,3,4],\"object\": {}}"
        o = wdj.JsonObject.parse(s)
        o["string"] = wdj.JsonValue.create_string_value("the larch")
        v = o.lookup("string")
        self.assertEqual(v.value_type, wdj.JsonValueType.STRING)
        self.assertEqual(v.get_string(), "the larch")

    def test_JsonValue_boolean(self):
        t = wdj.JsonValue.create_boolean_value(True)
        self.assertEqual(t.value_type, wdj.JsonValueType.BOOLEAN)
        self.assertTrue(t.get_boolean())

        f = wdj.JsonValue.create_boolean_value(False)
        self.assertEqual(f.value_type, wdj.JsonValueType.BOOLEAN)
        self.assertFalse(f.get_boolean())

    def test_JsonValue_null(self):
        n = wdj.JsonValue.create_null_value()
        self.assertEqual(n.value_type, wdj.JsonValueType.NULL)

    def test_JsonValue_number(self):
        t = wdj.JsonValue.create_number_value(42)
        self.assertEqual(t.value_type, wdj.JsonValueType.NUMBER)
        self.assertEqual(t.get_number(), 42)

    def test_JsonValue_string(self):
        t = wdj.JsonValue.create_string_value("Plugh")
        self.assertEqual(t.value_type, wdj.JsonValueType.STRING)
        self.assertEqual(t.get_string(), "Plugh")

    def test_JsonValue_parse(self):
        b = wdj.JsonValue.parse("true")
        self.assertEqual(b.value_type, wdj.JsonValueType.BOOLEAN)
        self.assertTrue(b.get_boolean())

        n = wdj.JsonValue.parse("16")
        self.assertEqual(n.value_type, wdj.JsonValueType.NUMBER)
        self.assertEqual(n.get_number(), 16)

        s = wdj.JsonValue.parse("\"plugh\"")
        self.assertEqual(s.value_type, wdj.JsonValueType.STRING)
        self.assertEqual(s.get_string(), "plugh")

    def test_invalid_param_count_instance(self):
        a = wdj.JsonArray()
        with self.assertRaises(TypeError):
            a.append(10, 20)

    def test_invalid_param_count_static(self):
        with self.assertRaises(TypeError):
            wdj.JsonArray.parse(10, 20)        

    def test_IJsonvalue_get_boolean(self):
        a = wdj.JsonArray.parse("[null, true, 42, \"spam\", [1,2,3], {\"scene\":24}]")
        v = a.get_at(1)
        self.assertTrue(v.get_boolean())

    def test_IJsonvalue_get_number(self):
        a = wdj.JsonArray.parse("[null, true, 42, \"spam\", [1,2,3], {\"scene\":24}]")
        v = a.get_at(2)
        self.assertEqual(v.get_number(), 42)

    def test_IJsonvalue_get_string(self):
        a = wdj.JsonArray.parse("[null, true, 42, \"spam\", [1,2,3], {\"scene\":24}]")
        v = a.get_at(3)
        self.assertEqual(v.get_string(), "spam")

    def test_JsonValue_create_number_value(self):
        v = wdj.JsonValue.create_number_value(42)
        self.assertEqual(v.value_type, wdj.JsonValueType.NUMBER)
        self.assertEqual(v.get_number(), 42)

    def test_JsonValue_create_boolean_value(self):
        v = wdj.JsonValue.create_boolean_value(True)
        self.assertEqual(v.value_type, wdj.JsonValueType.BOOLEAN)
        self.assertEqual(v.get_boolean(), True)

    def test_JsonValue_create_string_value(self):
        v = wdj.JsonValue.create_string_value("spam")
        self.assertEqual(v.value_type, wdj.JsonValueType.STRING)
        self.assertEqual(v.get_string(), "spam")

    def test_JsonValue_create_null_value(self):
        v = wdj.JsonValue.create_null_value()
        self.assertEqual(v.value_type, wdj.JsonValueType.NULL)

    def test_JsonObject_ctor(self):
        o = wdj.JsonObject()
        self.assertEqual(o.size, 0)
        self.assertEqual(o.value_type, wdj.JsonValueType.OBJECT)
        self.assertEqual(o.to_string(), "{}")
        self.assertEqual(o.stringify(), "{}")

    def test_JsonObject_get_named_boolean(self):
        o = wdj.JsonObject.parse("{ \"spam\": true }")
        v = o.get_named_boolean("spam")
        self.assertTrue(v)

    def test_JsonObject_str(self):
        a = wdj.JsonObject()
        self.assertEqual(str(a), "{}")

    def test_JsonObject_get_named_boolean_default(self):
        o = wdj.JsonObject.parse("{ \"spam\": true }")
        v = o.get_named_boolean("more-spam", True)
        self.assertTrue(v)

    def test_JsonObject_get_named_number(self):
        o = wdj.JsonObject.parse("{ \"spam\": 42 }")
        v = o.get_named_number("spam")
        self.assertEqual(v, 42)

    def test_JsonObject_get_named_number_default(self):
        o = wdj.JsonObject.parse("{ \"spam\": true }")
        v = o.get_named_number("more-spam", 16)
        self.assertEqual(v, 16)

# todo: GetMany, iterator, sequence

if __name__ == '__main__':
    unittest.main()
