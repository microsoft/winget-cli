// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ChannelStreams.h"
#include "VTSupport.h"
#include <winget/ManifestLocalization.h>
#include <wincodec.h>
#include <wil/com.h>
#include <filesystem>

namespace AppInstaller::CLI::VirtualTerminal::Sixel
{
    // Determines the height to width ratio of the pixels that make up a sixel (a sixel is 6 pixels tall and 1 pixel wide).
    // Note that each cell is always a height of 20 and a width of 10, regardless of the screen resolution of the terminal.
    // The 2:1 ratio will then result in each sixel being 12 of the 20 pixels of a cell.
    enum class AspectRatio
    {
        OneToOne = 7,
        TwoToOne = 0,
        ThreeToOne = 3,
        FiveToOne = 2,
    };

    // Contains the palette used by a sixel image.
    struct Palette
    {
        // Limit to 256 both as the defacto maximum supported colors and to enable always using 8bpp indexed pixel format.
        static constexpr UINT MaximumColorCount = 256;

        // Create a palette from the given source image, color count, transparency setting.
        Palette(IWICImagingFactory* factory, IWICBitmapSource* bitmapSource, UINT colorCount, bool transparencyEnabled);

        // Create a palette combining the two palettes. Throws an exception if there are more than MaximumColorCount unique
        // colors between the two. This can be avoided by intentionally dividing the available colors between the palettes
        // when creating them.
        Palette(const Palette& first, const Palette& second);

        // Gets the WIC palette object.
        IWICPalette* get() const;

        // Gets the color count for the palette.
        size_t size() const;

        // Gets the color at the given index in the palette.
        WICColor& operator[](size_t index);
        WICColor operator[](size_t index) const;

    private:
        wil::com_ptr<IWICImagingFactory> m_factory;
        wil::com_ptr<IWICPalette> m_paletteObject;
        std::vector<WICColor> m_palette;
    };

    // Allows access to the pixel data of an image source.
    // Can be configured to translate and/or tile the view.
    struct ImageView
    {
        // Create a view by locking a bitmap.
        // This must be used from the same thread as the bitmap.
        static ImageView Lock(IWICBitmap* imageSource);

        // Create a view by copying the pixels from the image.
        static ImageView Copy(IWICBitmapSource* imageSource);

        // If set to true, the view will % coordinates outside of its dimensions back into its own view.
        // If set to false, coordinates outside of the view will be null.
        void Tile(bool tile);

        // Translate the view by the given pixel counts.
        void Translate(INT x, INT y);

        // Gets the pixel of the view at the given coordinate.
        // Returns null if the coordinate is outside of the view.
        BYTE* GetPixel(UINT x, UINT y);

    private:
        ImageView() = default;

        wil::com_ptr<IWICBitmapLock> m_lockedImage;
        std::unique_ptr<BYTE[]> m_copiedImage;

        UINT m_viewWidth = 0;
        UINT m_viewHeight = 0;
        UINT m_viewStride = 0;
        UINT m_viewByteCount = 0;
        BYTE* m_viewBytes = nullptr;
    };

    // Contains an image that can be manipulated and rendered to sixels.
    struct ImageSource
    {
        // Create an image source from a file.
        ImageSource(const std::filesystem::path& imageFilePath);

        // Create an image source from a stream.
        ImageSource(std::istream& imageBytes, Manifest::IconFileTypeEnum imageEncoding);

        // Resize the image to the given width and height, factoring in the target aspect ratio for rendering.
        // If stretchToFill is true, the resulting image will be both the given width and height.
        // If false, the resulting image will be at most the given width or height while preserving the aspect ratio.
        void Resize(UINT pixelWidth, UINT pixelHeight, AspectRatio targetRenderRatio, bool stretchToFill = false);

        // Creates a palette from the current image.
        Palette CreatePalette(UINT colorCount, bool transparencyEnabled) const;

        // Converts the image to be 8bpp indexed for the given palette.
        void ApplyPalette(const Palette& palette);

        // Create a view by locking the image source.
        // This must be used from the same thread as the image source.
        ImageView Lock() const;

        // Create a view by copying the pixels from the image source.
        ImageView Copy() const;

    private:
        wil::com_ptr<IWICImagingFactory> m_factory;
        wil::com_ptr<IWICBitmap> m_sourceImage;
    };

    // Allows one or more image sources to be rendered to a sixel output.
    struct Compositor
    {
        Compositor() = default;
    };

    // A helpful wrapper around the sixel image primitives that makes rendering a single image easier.
    struct Image
    {
        // Limit to 256 both as the defacto maximum supported colors and to enable always using 8bpp indexed pixel format.
        static constexpr UINT MaximumColorCount = Palette::MaximumColorCount;

        // Yes, its right there in the name but the compiler can't read...
        static constexpr UINT PixelsPerSixel = 6;

        // Each cell is always a height of 20 and a width of 10, regardless of the screen resolution of the terminal.
        static constexpr UINT CellHeightInPixels = 20;
        static constexpr UINT CellWidthInPixels = 10;

        Image(const std::filesystem::path& imageFilePath);
        Image(std::istream& imageBytes, Manifest::IconFileTypeEnum imageEncoding);

        void AspectRatio(AspectRatio aspectRatio);
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
            Sixel::AspectRatio AspectRatio = AspectRatio::OneToOne;
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
