// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Sixel.h"
#include <AppInstallerStrings.h>
#include <winget/UserSettings.h>
#include <vector>
#include <sstream>

namespace AppInstaller::CLI::VirtualTerminal::Sixel
{
    namespace anon
    {
        wil::com_ptr<IWICImagingFactory> CreateFactory()
        {
            wil::com_ptr<IWICImagingFactory> result;
            THROW_IF_FAILED(CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&result)));
            return result;
        }

        UINT AspectRatioMultiplier(AspectRatio aspectRatio)
        {
            switch (aspectRatio)
            {
            case AspectRatio::OneToOne:
                return 1;
            case AspectRatio::TwoToOne:
                return 2;
            case AspectRatio::ThreeToOne:
                return 3;
            case AspectRatio::FiveToOne:
                return 5;
            default:
                THROW_HR(E_INVALIDARG);
            }
        }

        // Forces the given bitmap source to evaluate
        wil::com_ptr<IWICBitmap> CacheToBitmap(IWICImagingFactory* factory, IWICBitmapSource* sourceImage)
        {
            wil::com_ptr<IWICBitmap> result;
            THROW_IF_FAILED(factory->CreateBitmapFromSource(sourceImage, WICBitmapCacheOnLoad, &result));
            return result;
        }

        // Convert [0, 255] => [0, 100]
        UINT32 ByteToPercent(BYTE input)
        {
            return (static_cast<UINT32>(input) * 100 + 127) / 255;
        }

        // Contains the state for a rendering pass.
        struct RenderState
        {
            RenderState(
                const Palette& palette,
                const std::vector<ImageView>& views,
                const RenderControls& renderControls) :
                m_palette(palette),
                m_views(views),
                m_renderControls(renderControls)
            {
                // Create render buffers
                m_enabledColors.resize(m_palette.Size());
                m_sixelBuffer.resize(m_palette.Size() * m_renderControls.PixelWidth);
            }

            enum class State
            {
                Initial,
                Pixels,
                Final,
                Terminated,
            };

            // Advances the render state machine, returning true if `Current` will return a new sequence and false when it will not.
            bool Advance()
            {
                std::stringstream stream;

                switch (m_currentState)
                {
                case State::Initial:
                    // Initial device control string
                    stream << AICLI_VT_ESCAPE << 'P' << ToIntegral(m_renderControls.AspectRatio) << ";1;q";

                    for (size_t i = 0; i < m_palette.Size(); ++i)
                    {
                        // 2 is RGB color space, with values from 0 to 100
                        stream << '#' << i << ";2;";

                        WICColor currentColor = m_palette[i];
                        BYTE red = (currentColor >> 16) & 0xFF;
                        BYTE green = (currentColor >> 8) & 0xFF;
                        BYTE blue = (currentColor) & 0xFF;

                        stream << ByteToPercent(red) << ';' << ByteToPercent(green) << ';' << ByteToPercent(blue);
                    }

                    m_currentState = State::Pixels;
                    break;
                case State::Pixels:
                {
                    // Disable all colors and set all characters to empty (0x3F)
                    memset(m_enabledColors.data(), 0, m_enabledColors.size());
                    memset(m_sixelBuffer.data(), 0x3F, m_sixelBuffer.size());

                    // Convert indexed pixel data into per-color sixel lines
                    UINT rowsToProcess = std::min(RenderControls::PixelsPerSixel, m_renderControls.PixelHeight - m_currentPixelRow);

                    for (UINT rowOffset = 0; rowOffset < rowsToProcess; ++rowOffset)
                    {
                        // The least significant bit is the top of the sixel
                        char sixelBit = 1 << rowOffset;
                        UINT currentRow = m_currentPixelRow + rowOffset;

                        for (UINT i = 0; i < m_renderControls.PixelWidth; ++i)
                        {
                            const BYTE* pixelPtr = nullptr;
                            size_t colorIndex = 0;

                            for (const ImageView& view : m_views)
                            {
                                pixelPtr = view.GetPixel(i, currentRow);

                                if (pixelPtr)
                                {
                                    colorIndex = *pixelPtr;

                                    // Stop on the first non-transparent pixel we find
                                    if (((m_palette[colorIndex] >> 24) & 0xFF) != 0)
                                    {
                                        break;
                                    }
                                }
                            }

                            if (pixelPtr)
                            {
                                m_enabledColors[colorIndex] = 1;
                                m_sixelBuffer[(colorIndex * m_renderControls.PixelWidth) + i] += sixelBit;
                            }
                        }
                    }

                    // Output all sixel color lines
                    bool firstOfRow = true;

                    for (size_t i = 0; i < m_enabledColors.size(); ++i)
                    {
                        if (m_enabledColors[i])
                        {
                            if (m_renderControls.TransparencyEnabled)
                            {
                                // Don't output color if transparent
                                WICColor currentColor = m_palette[i];
                                BYTE alpha = (currentColor >> 24) & 0xFF;
                                if (alpha == 0)
                                {
                                    continue;
                                }
                            }

                            if (firstOfRow)
                            {
                                firstOfRow = false;
                            }
                            else
                            {
                                // The carriage return operator resets for another color pass.
                                stream << '$';
                            }

                            stream << '#' << i;

                            const char* colorRow = &m_sixelBuffer[i * m_renderControls.PixelWidth];

                            if (m_renderControls.UseRepeatSequence)
                            {
                                char currentChar = colorRow[0];
                                UINT repeatCount = 1;

                                for (UINT j = 1; j <= m_renderControls.PixelWidth; ++j)
                                {
                                    // Force processing of a final null character to handle flushing the line
                                    const char nextChar = (j == m_renderControls.PixelWidth ? 0 : colorRow[j]);

                                    if (nextChar == currentChar)
                                    {
                                        ++repeatCount;
                                    }
                                    else
                                    {
                                        if (repeatCount > 2)
                                        {
                                            stream << '!' << repeatCount;
                                        }
                                        else if (repeatCount == 2)
                                        {
                                            stream << currentChar;
                                        }

                                        stream << currentChar;

                                        currentChar = nextChar;
                                        repeatCount = 1;
                                    }
                                }
                            }
                            else
                            {
                                stream << std::string_view{ colorRow, m_renderControls.PixelWidth };
                            }
                        }
                    }

                    // The new line operator sets up for the next sixel row
                    stream << '-';

                    m_currentPixelRow += rowsToProcess;
                    if (m_currentPixelRow >= m_renderControls.PixelHeight)
                    {
                        m_currentState = State::Final;
                    }
                }
                    break;
                case State::Final:
                    stream << AICLI_VT_ESCAPE << '\\';
                    m_currentState = State::Terminated;
                    break;
                case State::Terminated:
                    m_currentSequence.clear();
                    return false;
                }

                m_currentSequence = std::move(stream).str();
                return true;
            }

            Sequence Current() const
            {
                return Sequence{ m_currentSequence };
            }

        private:
            const Palette& m_palette;
            const std::vector<ImageView>& m_views;
            const RenderControls& m_renderControls;

            State m_currentState = State::Initial;
            std::vector<char> m_enabledColors;
            std::vector<char> m_sixelBuffer;
            UINT m_currentPixelRow = 0;
            // TODO-C++20: Replace with a view from the stringstream
            std::string m_currentSequence;
        };
    }

    Palette::Palette(IWICImagingFactory* factory, IWICBitmapSource* bitmapSource, UINT colorCount, bool transparencyEnabled) :
        m_factory(factory)
    {
        THROW_IF_FAILED(m_factory->CreatePalette(&m_paletteObject));

        THROW_IF_FAILED(m_paletteObject->InitializeFromBitmap(bitmapSource, colorCount, transparencyEnabled));

        // Extract the palette for render use
        UINT actualColorCount = 0;
        THROW_IF_FAILED(m_paletteObject->GetColorCount(&actualColorCount));

        m_palette.resize(actualColorCount);
        THROW_IF_FAILED(m_paletteObject->GetColors(actualColorCount, m_palette.data(), &actualColorCount));
    }

    Palette::Palette(const Palette& first, const Palette& second)
    {
        auto firstPalette = first.m_palette;
        auto secondPalette = second.m_palette;
        std::sort(firstPalette.begin(), firstPalette.end());
        std::sort(secondPalette.begin(), secondPalette.end());

        // Construct a union of the two palettes
        std::set_union(firstPalette.begin(), firstPalette.end(), secondPalette.begin(), secondPalette.end(), std::back_inserter(m_palette));
        THROW_HR_IF(E_INVALIDARG, m_palette.size() > MaximumColorCount);

        m_factory = first.m_factory;
        THROW_IF_FAILED(m_factory->CreatePalette(&m_paletteObject));
        THROW_IF_FAILED(m_paletteObject->InitializeCustom(m_palette.data(), static_cast<UINT>(m_palette.size())));
    }

    IWICPalette* Palette::Get() const
    {
        return m_paletteObject.get();
    }

    size_t Palette::Size() const
    {
        return m_palette.size();
    }

    WICColor& Palette::operator[](size_t index)
    {
        return m_palette[index];
    }

    WICColor Palette::operator[](size_t index) const
    {
        return m_palette[index];
    }

    ImageView::ImageView(UINT width, UINT height, UINT stride, UINT byteCount, BYTE* bytes) :
        m_viewWidth(width), m_viewHeight(height), m_viewStride(stride), m_viewByteCount(byteCount), m_viewBytes(bytes)
    {}

    ImageView ImageView::Lock(IWICBitmap* imageSource)
    {
        WICPixelFormatGUID pixelFormat{};
        THROW_IF_FAILED(imageSource->GetPixelFormat(&pixelFormat));
        THROW_HR_IF(ERROR_INVALID_STATE, GUID_WICPixelFormat8bppIndexed != pixelFormat);

        ImageView result;

        UINT sourceX = 0;
        UINT sourceY = 0;
        THROW_IF_FAILED(imageSource->GetSize(&sourceX, &sourceY));
        THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW,
            sourceX > static_cast<UINT>(std::numeric_limits<INT>::max()) || sourceY > static_cast<UINT>(std::numeric_limits<INT>::max()));

        WICRect rect{};
        rect.Width = static_cast<INT>(sourceX);
        rect.Height = static_cast<INT>(sourceY);

        THROW_IF_FAILED(imageSource->Lock(&rect, WICBitmapLockRead, &result.m_lockedImage));
        THROW_IF_FAILED(result.m_lockedImage->GetSize(&result.m_viewWidth, &result.m_viewHeight));
        THROW_IF_FAILED(result.m_lockedImage->GetStride(&result.m_viewStride));
        THROW_IF_FAILED(result.m_lockedImage->GetDataPointer(&result.m_viewByteCount, &result.m_viewBytes));

        return result;
    }

    ImageView ImageView::Copy(IWICBitmapSource* imageSource)
    {
        WICPixelFormatGUID pixelFormat{};
        THROW_IF_FAILED(imageSource->GetPixelFormat(&pixelFormat));
        THROW_HR_IF(ERROR_INVALID_STATE, GUID_WICPixelFormat8bppIndexed != pixelFormat);

        ImageView result;

        THROW_IF_FAILED(imageSource->GetSize(&result.m_viewWidth, &result.m_viewHeight));
        THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW,
            result.m_viewWidth > static_cast<UINT>(std::numeric_limits<INT>::max()) || result.m_viewHeight > static_cast<UINT>(std::numeric_limits<INT>::max()));

        result.m_viewStride = result.m_viewWidth;
        result.m_viewByteCount = result.m_viewStride * result.m_viewHeight;
        result.m_copiedImage = std::make_unique<BYTE[]>(result.m_viewByteCount);
        result.m_viewBytes = result.m_copiedImage.get();

        THROW_IF_FAILED(imageSource->CopyPixels(nullptr, result.m_viewStride, result.m_viewByteCount, result.m_viewBytes));

        return result;
    }

    void ImageView::Translate(INT x, INT y, bool tile)
    {
        m_tile = tile;

        if (m_tile)
        {
            m_translateX = static_cast<UINT>(m_viewWidth - (x % static_cast<INT>(m_viewWidth)));
            m_translateY = static_cast<UINT>(m_viewHeight - (y % static_cast<INT>(m_viewHeight)));
        }
        else
        {
            m_translateX = static_cast<UINT>(-x);
            m_translateY = static_cast<UINT>(-y);
        }
    }

    const BYTE* ImageView::GetPixel(UINT x, UINT y) const
    {
        UINT translatedX = x + m_translateX;
        UINT tileCountX = translatedX / m_viewWidth;
        UINT viewX = translatedX % m_viewWidth;
        if (tileCountX && !m_tile)
        {
            return nullptr;
        }

        UINT translatedY = y + m_translateY;
        UINT tileCountY = translatedY / m_viewHeight;
        UINT viewY = translatedY % m_viewHeight;
        if (tileCountY && !m_tile)
        {
            return nullptr;
        }

        return m_viewBytes + (static_cast<size_t>(viewY) * m_viewStride) + viewX;
    }

    UINT ImageView::Width() const
    {
        return m_viewWidth;
    }

    UINT ImageView::Height() const
    {
        return m_viewHeight;
    }

    void RenderControls::RenderSizeInCells(UINT width, UINT height)
    {
        PixelWidth = width * CellWidthInPixels;

        // We don't want to overdraw the row below, so our height must be the largest multiple of 6 that fits in Y cells.
        UINT yInPixels = height * CellHeightInPixels;
        PixelHeight = yInPixels - (yInPixels % PixelsPerSixel);
    }

    ImageSource::ImageSource(const std::filesystem::path& imageFilePath)
    {
        m_factory = anon::CreateFactory();

        wil::com_ptr<IWICBitmapDecoder> decoder;
        THROW_IF_FAILED(m_factory->CreateDecoderFromFilename(imageFilePath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));

        wil::com_ptr<IWICBitmapFrameDecode> decodedFrame;
        THROW_IF_FAILED(decoder->GetFrame(0, &decodedFrame));

        m_sourceImage = anon::CacheToBitmap(m_factory.get(), decodedFrame.get());
    }

    ImageSource::ImageSource(std::istream& imageStream, Manifest::IconFileTypeEnum imageEncoding)
    {
        m_factory = anon::CreateFactory();

        wil::com_ptr<IStream> stream;
        THROW_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));

        auto imageBytes = Utility::ReadEntireStreamAsByteArray(imageStream);

        ULONG written = 0;
        THROW_IF_FAILED(stream->Write(imageBytes.data(), static_cast<ULONG>(imageBytes.size()), &written));
        THROW_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));

        wil::com_ptr<IWICBitmapDecoder> decoder;
        bool initializeDecoder = true;

        switch (imageEncoding)
        {
        case Manifest::IconFileTypeEnum::Unknown:
            THROW_IF_FAILED(m_factory->CreateDecoderFromStream(stream.get(), NULL, WICDecodeMetadataCacheOnDemand, &decoder));
            initializeDecoder = false;
            break;
        case Manifest::IconFileTypeEnum::Jpeg:
            THROW_IF_FAILED(m_factory->CreateDecoder(GUID_ContainerFormatJpeg, NULL, &decoder));
            break;
        case Manifest::IconFileTypeEnum::Png:
            THROW_IF_FAILED(m_factory->CreateDecoder(GUID_ContainerFormatPng, NULL, &decoder));
            break;
        case Manifest::IconFileTypeEnum::Ico:
            THROW_IF_FAILED(m_factory->CreateDecoder(GUID_ContainerFormatIco, NULL, &decoder));
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        if (initializeDecoder)
        {
            THROW_IF_FAILED(decoder->Initialize(stream.get(), WICDecodeMetadataCacheOnDemand));
        }

        wil::com_ptr<IWICBitmapFrameDecode> decodedFrame;
        THROW_IF_FAILED(decoder->GetFrame(0, &decodedFrame));

        m_sourceImage = anon::CacheToBitmap(m_factory.get(), decodedFrame.get());
    }

    void ImageSource::Resize(UINT pixelWidth, UINT pixelHeight, AspectRatio targetRenderRatio, bool stretchToFill, InterpolationMode interpolationMode)
    {
        if ((pixelWidth && pixelHeight) || targetRenderRatio != AspectRatio::OneToOne)
        {
            UINT targetX = pixelWidth;
            UINT targetY = pixelHeight;

            if (!stretchToFill)
            {
                // We need to calculate which of the sizes needs to be reduced
                UINT sourceImageX = 0;
                UINT sourceImageY = 0;
                THROW_IF_FAILED(m_sourceImage->GetSize(&sourceImageX, &sourceImageY));

                double doubleTargetX = targetX;
                double doubleTargetY = targetY;
                double doubleSourceImageX = sourceImageX;
                double doubleSourceImageY = sourceImageY;

                double scaleFactorX = doubleTargetX / doubleSourceImageX;
                double targetY_scaledForX = sourceImageY * scaleFactorX;
                if (targetY_scaledForX > doubleTargetY)
                {
                    // Scaling to make X fill would make Y to large, so we must scale to fill Y
                    targetX = static_cast<UINT>(sourceImageX * (doubleTargetY / doubleSourceImageY));
                }
                else
                {
                    // Scaling to make X fill kept Y under target
                    targetY = static_cast<UINT>(targetY_scaledForX);
                }
            }

            // Apply aspect ratio scaling
            targetY /= anon::AspectRatioMultiplier(targetRenderRatio);

            wil::com_ptr<IWICBitmapScaler> scaler;
            THROW_IF_FAILED(m_factory->CreateBitmapScaler(&scaler));

            THROW_IF_FAILED(scaler->Initialize(m_sourceImage.get(), targetX, targetY, ToEnum<WICBitmapInterpolationMode>(ToIntegral(interpolationMode))));
            m_sourceImage = anon::CacheToBitmap(m_factory.get(), scaler.get());
        }
    }

    void ImageSource::Resize(const RenderControls& controls)
    {
        Resize(controls.PixelWidth, controls.PixelHeight, controls.AspectRatio, controls.StretchSourceToFill, controls.InterpolationMode);
    }

    Palette ImageSource::CreatePalette(UINT colorCount, bool transparencyEnabled) const
    {
        return { m_factory.get(), m_sourceImage.get(), colorCount, transparencyEnabled };
    }

    Palette ImageSource::CreatePalette(const RenderControls& controls) const
    {
        return CreatePalette(controls.ColorCount, controls.TransparencyEnabled);
    }

    void ImageSource::ApplyPalette(const Palette& palette)
    {
        // Convert to 8bpp indexed
        wil::com_ptr<IWICFormatConverter> converter;
        THROW_IF_FAILED(m_factory->CreateFormatConverter(&converter));

        // TODO: Determine a better value or enable it to be set
        constexpr double s_alphaThreshold = 0.5;

        THROW_IF_FAILED(converter->Initialize(m_sourceImage.get(), GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeErrorDiffusion, palette.Get(), s_alphaThreshold, WICBitmapPaletteTypeCustom));
        m_sourceImage = anon::CacheToBitmap(m_factory.get(), converter.get());
    }

    ImageView ImageSource::Lock() const
    {
        return ImageView::Lock(m_sourceImage.get());
    }

    ImageView ImageSource::Copy() const
    {
        return ImageView::Copy(m_sourceImage.get());
    }

    void Compositor::Palette(Sixel::Palette palette)
    {
        m_palette = std::move(palette);
    }

    void Compositor::AddView(ImageView&& view)
    {
        m_views.emplace_back(std::move(view));
    }

    size_t Compositor::ViewCount() const
    {
        return m_views.size();
    }

    ImageView& Compositor::operator[](size_t index)
    {
        return m_views[index];
    }

    const ImageView& Compositor::operator[](size_t index) const
    {
        return m_views[index];
    }

    RenderControls& Compositor::Controls()
    {
        return m_renderControls;
    }

    const RenderControls& Compositor::Controls() const
    {
        return m_renderControls;
    }

    ConstructedSequence Compositor::Render()
    {
        anon::RenderState renderState{ m_palette, m_views, m_renderControls };

        std::stringstream result;

        while (renderState.Advance())
        {
            result << renderState.Current().Get();
        }

        return ConstructedSequence{ std::move(result).str() };
    }

    void Compositor::RenderTo(Execution::BaseStream& stream)
    {
        anon::RenderState renderState{ m_palette, m_views, m_renderControls };

        while (renderState.Advance())
        {
            stream << renderState.Current();
        }
    }

    void Compositor::RenderTo(Execution::OutputStream& stream)
    {
        anon::RenderState renderState{ m_palette, m_views, m_renderControls };

        while (renderState.Advance())
        {
            stream << renderState.Current();
        }
    }

    Image::Image(const std::filesystem::path& imageFilePath) :
        m_imageSource(imageFilePath)
    {}

    Image::Image(std::istream& imageStream, Manifest::IconFileTypeEnum imageEncoding) :
        m_imageSource(imageStream, imageEncoding)
    {}

    Image& Image::AspectRatio(Sixel::AspectRatio aspectRatio)
    {
        m_renderControls.AspectRatio = aspectRatio;
        return *this;
    }

    Image& Image::Transparency(bool transparencyEnabled)
    {
        m_renderControls.TransparencyEnabled = transparencyEnabled;
        return *this;
    }

    Image& Image::ColorCount(UINT colorCount)
    {
        THROW_HR_IF(E_INVALIDARG, colorCount > Palette::MaximumColorCount || colorCount < 2);
        m_renderControls.ColorCount = colorCount;
        return *this;
    }

    Image& Image::RenderSizeInPixels(UINT width, UINT height)
    {
        m_renderControls.PixelWidth = width;
        m_renderControls.PixelHeight = height;
        return *this;
    }

    Image& Image::RenderSizeInCells(UINT width, UINT height)
    {
        m_renderControls.RenderSizeInCells(width, height);
        return *this;
    }

    Image& Image::StretchSourceToFill(bool stretchSourceToFill)
    {
        m_renderControls.StretchSourceToFill = stretchSourceToFill;
        return *this;
    }

    Image& Image::UseRepeatSequence(bool useRepeatSequence)
    {
        m_renderControls.UseRepeatSequence = useRepeatSequence;
        return *this;
    }

    ConstructedSequence Image::Render()
    {
        return CreateCompositor().second.Render();
    }

    void Image::RenderTo(Execution::OutputStream& stream)
    {
        CreateCompositor().second.RenderTo(stream);
    }

    std::pair<ImageSource, Compositor> Image::CreateCompositor()
    {
        ImageSource localSource{ m_imageSource };
        localSource.Resize(m_renderControls);

        Palette palette{ localSource.CreatePalette(m_renderControls) };
        localSource.ApplyPalette(palette);

        ImageView view{ localSource.Lock() };

        Compositor compositor;
        compositor.Palette(std::move(palette));
        compositor.AddView(std::move(view));
        compositor.Controls() = m_renderControls;

        return { std::move(localSource), std::move(compositor) };
    }
}
