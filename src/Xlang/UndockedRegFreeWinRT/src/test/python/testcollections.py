import sys
sys.path.append("./generated")

import unittest
import asyncio

import winrt.windows.foundation.collections as wfc

class TestCollections(unittest.TestCase):

    def test_stringmap(self):
        m = wfc.StringMap()
        m.insert("hello", "world")

        self.assertTrue(m.has_key("hello"))
        self.assertFalse(m.has_key("world"))
        self.assertEqual(m.size, 1)
        self.assertEqual(m.lookup("hello"), "world")


    def test_stringmap_changed_event(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(None)
        
        async def async_test():
            future = loop.create_future()

            def onMapChanged(sender, args): 
                self.assertEqual(args.collection_change, wfc.CollectionChange.ITEM_INSERTED)
                self.assertEqual(args.key, "dr")

                self.assertEqual(sender.size, 2)
                self.assertTrue(sender.has_key("dr"))
                self.assertTrue(sender.has_key("hello"))
                
                loop.call_soon_threadsafe(asyncio.Future.set_result, future, True)

            m = wfc.StringMap()
            m.insert("hello", "world")
            token = m.add_map_changed(onMapChanged)
            m.insert("dr", "who")
            m.remove_map_changed(token)

            called = await future
            self.assertTrue(called)

        loop.run_until_complete(async_test())
        loop.close()


if __name__ == '__main__':
    unittest.main()
