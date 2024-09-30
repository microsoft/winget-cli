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

    // Determines the algorithm used when resizing the image.
    enum class InterpolationMode
    {
        NearestNeighbor = WICBitmapInterpolationModeNearestNeighbor,
        Linear = WICBitmapInterpolationModeLinear,
        Cubic = WICBitmapInterpolationModeCubic,
        Fant = WICBitmapInterpolationModeFant,
        HighQualityCubic = WICBitmapInterpolationModeHighQualityCubic,
    };

    // Contains the palette used by a sixel image.
    struct Palette
    {
        // Limit to 256 both as the defacto maximum supported colors and to enable always using 8bpp indexed pixel format.
        static constexpr UINT MaximumColorCount = 256;

        // Creates an empty palette.
        Palette() = default;

        // Create a palette from the given source image, color count, transparency setting.
        Palette(IWICImagingFactory* factory, IWICBitmapSource* bitmapSource, UINT colorCount, bool transparencyEnabled);

        // Create a palette combining the two palettes. Throws an exception if there are more than MaximumColorCount unique
        // colors between the two. This can be avoided by intentionally dividing the available colors between the palettes
        // when creating them.
        Palette(const Palette& first, const Palette& second);

        // Gets the WIC palette object.
        IWICPalette* Get() const;

        // Gets the color count for the palette.
        size_t Size() const;

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
        // Creates a non-owning view using the given data.
        ImageView(UINT width, UINT height, UINT stride, UINT byteCount, BYTE* bytes);

        // Create a view by locking a bitmap.
        // This must be used from the same thread as the bitmap.
        static ImageView Lock(IWICBitmap* imageSource);

        // Create a view by copying the pixels from the image.
        static ImageView Copy(IWICBitmapSource* imageSource);

        // Translate the view by the given pixel counts.
        // The pixel at [0, 0] of the original will be at [x, y].
        // If tile is true, the view will % coordinates outside of its dimensions back into its own view.
        // If tile is false, coordinates outside of the view will be null.
        void Translate(INT x, INT y, bool tile);

        // Gets the pixel of the view at the given coordinate.
        // Returns null if the coordinate is outside of the view.
        const BYTE* GetPixel(UINT x, UINT y) const;

        // Get the dimensions of the view.
        UINT Width() const;
        UINT Height() const;

    private:
        ImageView() = default;

        bool m_tile = false;
        UINT m_translateX = 0;
        UINT m_translateY = 0;

        wil::com_ptr<IWICBitmapLock> m_lockedImage;
        std::unique_ptr<BYTE[]> m_copiedImage;

        UINT m_viewWidth = 0;
        UINT m_viewHeight = 0;
        UINT m_viewStride = 0;
        UINT m_viewByteCount = 0;
        BYTE* m_viewBytes = nullptr;
    };

    // The set of values that defines the rendered output.
    struct RenderControls
    {
        // Yes, its right there in the name but the compiler can't read...
        static constexpr UINT PixelsPerSixel = 6;

        // Each cell is always a height of 20 and a width of 10, regardless of the screen resolution of the terminal.
        static constexpr UINT CellHeightInPixels = 20;
        static constexpr UINT CellWidthInPixels = 10;

        Sixel::AspectRatio AspectRatio = AspectRatio::OneToOne;
        bool TransparencyEnabled = true;
        bool StretchSourceToFill = false;
        bool UseRepeatSequence = false;
        UINT ColorCount = Palette::MaximumColorCount;
        UINT PixelWidth = 0;
        UINT PixelHeight = 0;
        Sixel::InterpolationMode InterpolationMode = InterpolationMode::HighQualityCubic;

        // The resulting sixel image will render to this size in terminal cells,
        // consuming as much as possible of the given size without going over.
        void RenderSizeInCells(UINT width, UINT height);
    };

    // Contains an image that can be manipulated and rendered to sixels.
    struct ImageSource
    {
        // Create an image source from a file.
        explicit ImageSource(const std::filesystem::path& imageFilePath);

        // Create an empty image source.
        ImageSource() = default;

        // Create an image source from a stream.
        ImageSource(std::istream& imageStream, Manifest::IconFileTypeEnum imageEncoding);

        // Resize the image to the given width and height, factoring in the target aspect ratio for rendering.
        // If stretchToFill is true, the resulting image will be both the given width and height.
        // If false, the resulting image will be at most the given width or height while preserving the aspect ratio.
        void Resize(UINT pixelWidth, UINT pixelHeight, AspectRatio targetRenderRatio, bool stretchToFill = false, InterpolationMode interpolationMode = InterpolationMode::HighQualityCubic);

        // Resizes the image using the given render controls.
        void Resize(const RenderControls& controls);

        // Creates a palette from the current image.
        Palette CreatePalette(UINT colorCount, bool transparencyEnabled) const;

        // Creates a palette from the current image.
        Palette CreatePalette(const RenderControls& controls) const;

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
        // Create an empty compositor.
        Compositor() = default;

        // Set the palette to be used by the compositor.
        void Palette(Palette palette);

        // Adds a new view to the compositor. Each successive view will be behind all of the others.
        void AddView(ImageView&& view);

        // Gets the number of views in the compositor.
        size_t ViewCount() const;

        // Gets the color at the given index in the palette.
        ImageView& operator[](size_t index);
        const ImageView& operator[](size_t index) const;

        // Get the render controls for the compositor.
        RenderControls& Controls();
        const RenderControls& Controls() const;

        // Render to sixel format for storage / use multiple times.
        ConstructedSequence Render();

        // Renders to sixel format directly to the stream.
        void RenderTo(Execution::BaseStream& stream);

        // Renders to sixel format directly to the stream.
        void RenderTo(Execution::OutputStream& stream);

    private:
        RenderControls m_renderControls;
        Sixel::Palette m_palette;
        std::vector<ImageView> m_views;
    };

    // A helpful wrapper around the sixel image primitives that makes rendering a single image easier.
    struct Image
    {
        // Create an image from a file.
        Image(const std::filesystem::path& imageFilePath);

        // Create an image from a stream.
        Image(std::istream& imageStream, Manifest::IconFileTypeEnum imageEncoding);

        // Set the aspect ratio of the result.
        Image& AspectRatio(AspectRatio aspectRatio);

        // Determine whether transparency is enabled.
        // This will affect whether transparent pixels are rendered or not.
        Image& Transparency(bool transparencyEnabled);

        // If transparency is enabled, one of the colors will be reserved for it.
        Image& ColorCount(UINT colorCount);

        // The resulting sixel image will render to this size in terminal cell pixels.
        Image& RenderSizeInPixels(UINT width, UINT height);

        // The resulting sixel image will render to this size in terminal cells,
        // consuming as much as possible of the given size without going over.
        Image& RenderSizeInCells(UINT width, UINT height);

        // Only affects the scaling of the image that occurs when render size is set.
        // When true, the source image will be stretched to fill the target size.
        // When false, the source image will be scaled while keeping its original aspect ratio.
        Image& StretchSourceToFill(bool stretchSourceToFill);

        // Compresses the output using repeat sequences.
        Image& UseRepeatSequence(bool useRepeatSequence);

        // Render to sixel format for storage / use multiple times.
        ConstructedSequence Render();

        // Renders to sixel format directly to the output stream.
        void RenderTo(Execution::OutputStream& stream);

    private:
        // Creates a compositor for the image using the current render controls.
        std::pair<ImageSource, Compositor> CreateCompositor();

        ImageSource m_imageSource;
        RenderControls m_renderControls;
    };
}
