// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ChannelStreams.h"
#include "VTSupport.h"
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
        SixelImage(const std::filesystem::path& imageFilePath);

        void AspectRatio(SixelAspectRatio aspectRatio);
        void Transparency(bool transparencyEnabled);

        // Limit to 256 both as the defacto maximum supported colors and to enable always using 8bpp indexed pixel format.
        static constexpr size_t MaximumColorCount = 256;

        // If transparency is enabled, one of the colors will be reserved for it.
        void ColorCount(size_t colorCount);

        // The current aspect ratio will be used to convert to cell relative pixel size.
        // The resulting sixel image will render to this size in terminal cell pixels.
        void RenderSizeInPixels(size_t x, size_t y);

        // The current aspect ratio will be used to convert to cell relative pixel size.
        // The resulting sixel image will render to this size in terminal cells,
        // consuming as much as possible of the given size without going over.
        void RenderSizeInCells(size_t x, size_t y);

        // Only affects the scaling of the image that occurs when render size is set.
        // When true, the source image will be stretched to fill the target size.
        // When false, the source image will be scaled while keeping its original aspect ratio.
        void StretchSourceToFill(bool stretchSourceToFill);

        // Render to sixel format for storage / use multiple times.
        ConstructedSequence Render();

        // Renders to sixel format directly to the output stream.
        void RenderTo(Execution::OutputStream& stream);

    private:
        SixelAspectRatio m_aspectRatio = SixelAspectRatio::OneToOne;
        bool m_transparencyEnabled = false;
        bool m_stretchSourceToFill = false;

        size_t m_colorCount = MaximumColorCount;
        size_t m_renderSizeX = 0;
        size_t m_renderSizeY = 0;
    };
}
