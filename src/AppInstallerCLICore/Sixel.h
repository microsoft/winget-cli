// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ChannelStreams.h"
#include "VTSupport.h"
#include <winget/ManifestLocalization.h>
#include <wincodec.h>
#include <wil/com.h>
#include <filesystem>

namespace AppInstaller::CLI::VirtualTerminal
{
    // Determines the height to width ratio of the pixels that make up a sixel (a sixel is 6 pixels tall and 1 pixel wide).
    // Note that each cell is always a height of 20 and a width of 10, regardless of the screen resolution of the terminal.
    // The 2:1 ratio will then result in each sixel being 12 of the 20 pixels of a cell.
    enum class SixelAspectRatio
    {
        OneToOne = 7,
        TwoToOne = 0,
        ThreeToOne = 3,
        FiveToOne = 2,
    };

    // Contains an image that can be manipulated and rendered to sixels.
    struct SixelImage
    {
        // Limit to 256 both as the defacto maximum supported colors and to enable always using 8bpp indexed pixel format.
        static constexpr UINT MaximumColorCount = 256;

        // Yes, its right there in the name but the compiler can't read...
        static constexpr UINT PixelsPerSixel = 6;

        // Each cell is always a height of 20 and a width of 10, regardless of the screen resolution of the terminal.
        static constexpr UINT CellHeightInPixels = 20;
        static constexpr UINT CellWidthInPixels = 10;

        SixelImage(const std::filesystem::path& imageFilePath);
        SixelImage(std::istream& imageBytes, Manifest::IconFileTypeEnum imageEncoding);

        void AspectRatio(SixelAspectRatio aspectRatio);
        void Transparency(bool transparencyEnabled);

        // If transparency is enabled, one of the colors will be reserved for it.
        void ColorCount(UINT colorCount);

        // The resulting sixel image will render to this size in terminal cell pixels.
        void RenderSizeInPixels(UINT x, UINT y);

        // The resulting sixel image will render to this size in terminal cells,
        // consuming as much as possible of the given size without going over.
        void RenderSizeInCells(UINT x, UINT y);

        // Only affects the scaling of the image that occurs when render size is set.
        // When true, the source image will be stretched to fill the target size.
        // When false, the source image will be scaled while keeping its original aspect ratio.
        void StretchSourceToFill(bool stretchSourceToFill);

        // Compresses the output using repeat sequences.
        void UseRepeatSequence(bool useRepeatSequence);

        // Render to sixel format for storage / use multiple times.
        ConstructedSequence Render();

        // Renders to sixel format directly to the output stream.
        void RenderTo(Execution::OutputStream& stream);

        // The set of values that defines the rendered output.
        struct RenderControls
        {
            SixelAspectRatio AspectRatio = SixelAspectRatio::OneToOne;
            bool TransparencyEnabled = true;
            bool StretchSourceToFill = false;
            bool UseRepeatSequence = false;
            UINT ColorCount = MaximumColorCount;
            UINT SizeX = 0;
            UINT SizeY = 0;
        };

    private:
        void InitializeFactory();

        wil::com_ptr<IWICImagingFactory> m_factory;
        wil::com_ptr<IWICBitmapSource> m_sourceImage;

        RenderControls m_renderControls;
    };

    // Determines if sixels are enabled.
    bool SixelsEnabled();
}
