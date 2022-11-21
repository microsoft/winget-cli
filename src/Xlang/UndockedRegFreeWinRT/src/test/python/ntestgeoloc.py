import loader_native, unittest, asyncio

wf = loader_native.import_ns("Windows.Foundation")
wdg = loader_native.import_ns("Windows.Devices.Geolocation")

# async_test inspired by https://stackoverflow.com/a/23036785
def async_test(test):
    def wrapper(*args, **kwargs):
        original_loop = asyncio.get_event_loop()
        test_loop = asyncio.new_event_loop()
        asyncio.set_event_loop(test_loop)
        try:
            test_loop.run_until_complete(test(*args, **kwargs))
        finally:
            test_loop.close()
            asyncio.set_event_loop(original_loop)
    return wrapper

class TestGeolocation(unittest.TestCase):

    def test_pinterface_qi(self):
        locator = wdg.Geolocator()
        op = locator.get_geoposition_async()
        self.assertEqual(type(op), wf.IAsyncOperation)
        op.cancel()

    def test_struct_ctor(self):
        basic_pos = wdg.BasicGeoposition(latitude = 47.1, longitude = -122.1, altitude = 0.0)
        self.assertEqual(basic_pos.latitude, 47.1)
        self.assertEqual(basic_pos.longitude, -122.1)
        self.assertEqual(basic_pos.altitude, 0.0)

        geocircle = wdg.Geocircle(basic_pos, 10)
        center = geocircle.center

        self.assertEqual(10, geocircle.radius)
        for x in ["latitude", "longitude", "altitude"]:
            self.assertEqual(getattr(basic_pos, x), getattr(center, x))

    def test_struct_from_dict(self):
        basic_pos = {"latitude": 47.1, "longitude": -122.1, "altitude": 0.0}

        geocircle = wdg.Geocircle(basic_pos, 10)
        center = geocircle.center

        self.assertEqual(10, geocircle.radius)
        for x in ["latitude", "longitude", "altitude"]:
            self.assertEqual(basic_pos[x], getattr(center, x))

    def test_iiterable_wraping(self):
        basic_pos1 = wdg.BasicGeoposition(47.1, -122.1, 0.0)
        basic_pos2 = wdg.BasicGeoposition(47.2, -122.2, 0.0)

        box = wdg.GeoboundingBox.try_compute([basic_pos1, basic_pos2])
        nw = box.northwest_corner
        se = box.southeast_corner

        self.assertAlmostEqual(nw.latitude, basic_pos2.latitude)
        self.assertAlmostEqual(nw.longitude, basic_pos2.longitude)
        self.assertAlmostEqual(se.latitude, basic_pos1.latitude)
        self.assertAlmostEqual(se.longitude, basic_pos1.longitude)

    def test_GetGeopositionAsync(self):
        """test async method using IAsyncOperation Completed callback"""
        import threading

        complete_event = threading.Event()

        def callback(operation, status):
            self.assertEqual(status, 1)
            pos = operation.get_results()

            self.assertEqual(type(pos), wdg.Geoposition)

            coord = pos.coordinate
            self.assertEqual(type(coord.timestamp.universal_time), int)

            basic_pos = coord.point.position
            lat = basic_pos.latitude
            self.assertEqual(type(lat), float)

            complete_event.set()

        locator = wdg.Geolocator()
        op = locator.get_geoposition_async()
        op.completed = callback

        self.assertTrue(complete_event.wait(5))

    @async_test
    async def test_GetGeopositionAsync_await(self):
        """test async method by directly awaiting IAsyncOperation"""

        locator = wdg.Geolocator()
        pos = await locator.get_geoposition_async()
        self.assertEqual(type(pos), wdg.Geoposition)

        coord = pos.coordinate
        self.assertEqual(type(coord.timestamp.universal_time), int)

        basic_pos = coord.point.position
        lat = basic_pos.latitude
        self.assertEqual(type(lat), float)

if __name__ == '__main__':
    unittest.main()
