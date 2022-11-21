import sys
sys.path.append("./generated")

import unittest

import winrt.windows.graphics.directx as wgd
import winrt.windows.graphics.directx.direct3d11 as wgdd

class TestDirectX(unittest.TestCase):

    def test_struct_containing_enum_pos(self):
        msd = wgdd.Direct3DMultisampleDescription(1, 2)
        sd = wgdd.Direct3DSurfaceDescription(4, 8, wgd.DirectXPixelFormat.R16_G16_B16_A16_FLOAT, msd)

        self.assertEqual(sd.width, 4)
        self.assertEqual(sd.height, 8)
        self.assertEqual(sd.format, wgd.DirectXPixelFormat.R16_G16_B16_A16_FLOAT)

        msd2 = sd.multisample_description
        self.assertEqual(msd.count, 1)
        self.assertEqual(msd.quality, 2)
    
    def test_struct_containing_enum_kwd(self):
        msd = wgdd.Direct3DMultisampleDescription(1, 2)
        sd = wgdd.Direct3DSurfaceDescription(format=wgd.DirectXPixelFormat.R16_G16_B16_A16_FLOAT, width=4, multisample_description=msd, height=8)

        self.assertEqual(sd.width, 4)
        self.assertEqual(sd.height, 8)
        self.assertEqual(sd.format, wgd.DirectXPixelFormat.R16_G16_B16_A16_FLOAT)

        msd2 = sd.multisample_description
        self.assertEqual(msd.count, 1)
        self.assertEqual(msd.quality, 2)

if __name__ == '__main__':
    unittest.main()
