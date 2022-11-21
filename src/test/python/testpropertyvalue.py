import sys
sys.path.append("./generated")

from uuid import UUID

import unittest

import winrt.windows.foundation as wf

class TestPropertyValue(unittest.TestCase):

    # TODO: CreateEmpty seems to be failing @ the C++/WinRT layer. 

    def test_create_uint8(self):
        o = wf.PropertyValue.create_uint8(250)
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.UINT8)
        self.assertTrue(ipv.get_uint8(), 250)

    def test_create_int16(self):
       o = wf.PropertyValue.create_int16(-32000)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.INT16)
       self.assertTrue(ipv.get_int16(), -32000)

    def test_create_uint16(self):
       o = wf.PropertyValue.create_uint16(65000)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.UINT16)
       self.assertTrue(ipv.get_uint16(), 65000)

    def test_create_int32(self):
       o = wf.PropertyValue.create_int32(-2147483640)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.INT32)
       self.assertTrue(ipv.get_int32(), -2147483640)

    def test_create_uint32(self):
       o = wf.PropertyValue.create_uint32(4294967290)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.UINT32)
       self.assertTrue(ipv.get_uint32(), 4294967290)

    def test_create_int64(self):
       o = wf.PropertyValue.create_int64(-9223372036854775800)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.INT64)
       self.assertTrue(ipv.get_int64(), -9223372036854775800)

    def test_create_uint64(self):
       o = wf.PropertyValue.create_uint64(18446744073709551610)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.UINT64)
       self.assertTrue(ipv.get_uint64(), 18446744073709551610)

    def test_create_single(self):
        o = wf.PropertyValue.create_single(3.14)
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.SINGLE)
        self.assertAlmostEqual(ipv.get_single(), 3.14, 5)

    def test_create_double(self):
        o = wf.PropertyValue.create_double(3.14)
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.DOUBLE)
        self.assertEqual(ipv.get_double(), 3.14)

    # # TODO: CreateChar16

    def test_create_boolean(self):
       o = wf.PropertyValue.create_boolean(True)
       ipv = wf.IPropertyValue._from(o)
       self.assertEqual(ipv.type, wf.PropertyType.BOOLEAN)
       self.assertTrue(ipv.get_boolean())

    def test_create_string(self):
        o = wf.PropertyValue.create_string("Ni!")
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.STRING)
        self.assertEqual(ipv.get_string(), "Ni!")

    # # TODO: CreateInspectable

    def test_create_datetime(self):
        o = wf.PropertyValue.create_date_time(wf.DateTime(0))
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.DATE_TIME)
        self.assertEqual(ipv.get_date_time().universal_time, 0)

    def test_create_TimeSpan(self):
        o = wf.PropertyValue.create_time_span(wf.TimeSpan(0))
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.TIME_SPAN)
        self.assertEqual(ipv.get_time_span().duration, 0)        

    def test_create_Guid(self):
        u = UUID('01234567-89ab-cdef-0123456789abcdef')
        o = wf.PropertyValue.create_guid(u)
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.GUID)
        self.assertEqual(ipv.get_guid(), u)

    def test_create_Point(self):
        o = wf.PropertyValue.create_point(wf.Point(2, 4))
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.POINT)
        s = ipv.get_point()
        self.assertEqual(s.x, 2)
        self.assertEqual(s.y, 4)

    def test_create_Size(self):
        o = wf.PropertyValue.create_size(wf.Size(2, 4))
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.SIZE)
        s = ipv.get_size()
        self.assertEqual(s.width, 2)
        self.assertEqual(s.height, 4)

    def test_create_Rect(self):
        o = wf.PropertyValue.create_rect(wf.Rect(2, 4, 6, 8))
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.RECT)
        s = ipv.get_rect()
        self.assertEqual(s.x, 2)
        self.assertEqual(s.y, 4)
        self.assertEqual(s.width, 6)
        self.assertEqual(s.height, 8)

    def test_create_uint8_array(self):
        o = wf.PropertyValue.create_uint8_array([1,2,3,4,5])
        ipv = wf.IPropertyValue._from(o)
        self.assertEqual(ipv.type, wf.PropertyType.UINT8_ARRAY)
        a = ipv.get_uint8_array()
        self.assertEqual(len(a), 5)
        for x in range(0,5):
            self.assertEqual(a[x], x+1)

if __name__ == '__main__':
    unittest.main()
