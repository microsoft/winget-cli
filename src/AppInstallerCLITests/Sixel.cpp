// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Sixel.h>

using namespace AppInstaller::CLI::VirtualTerminal::Sixel;

void ValidateGetPixel(std::string_view info, UINT_PTR offset, UINT byteCount, UINT_PTR expected)
{
    INFO(info);
    REQUIRE(offset < byteCount);
    REQUIRE(offset == expected);
}

TEST_CASE("ImageView_GetPixel", "[sixel]")
{
    UINT width = 20;
    UINT height = 20;
    UINT stride = 32;
    UINT byteCount = height * stride;
    BYTE* byteBase = reinterpret_cast<BYTE*>(100);

    ImageView view{ width, height, stride, byteCount, byteBase };

    ValidateGetPixel("No translation", view.GetPixel(3, 17) - byteBase, byteCount, 17 * stride + 3);

    view.Translate(14, 8, true);
    ValidateGetPixel("Positive translation (tile)", view.GetPixel(3, 17) - byteBase, byteCount, 9 * stride + 9);

    view.Translate(-14, 8, true);
    ValidateGetPixel("Negative translation (tile)", view.GetPixel(3, 17) - byteBase, byteCount, 9 * stride + 17);

    view.Translate(14, -8, false);
    REQUIRE(view.GetPixel(3, 17) == nullptr);
    ValidateGetPixel("Negative translation (no tile)", view.GetPixel(15, 1) - byteBase, byteCount, 9 * stride + 1);
}

TEST_CASE("Image_Render", "[sixel]")
{
    Image image{ TestCommon::TestDataFile("notepad.ico") };
    REQUIRE(!image.Render().Get().empty());

    image.AspectRatio(AspectRatio::ThreeToOne);
    image.ColorCount(64);
    image.RenderSizeInCells(2, 1);
    image.UseRepeatSequence(true);
    REQUIRE(!image.Render().Get().empty());
}
