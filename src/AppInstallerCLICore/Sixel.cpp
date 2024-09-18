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
            UINT32 result = static_cast<UINT32>(input);
            result *= 100;
            UINT32 fractional = result % 255;
            result /= 255;
            return result + (fractional >= 128 ? 1 : 0);
        }

        // Contains the state for a rendering pass.
        struct RenderState
        {
            RenderState(
                IWICImagingFactory* factory,
                wil::com_ptr<IWICBitmapSource>& sourceImage,
                const Image::RenderControls& renderControls) :
                m_renderControls(renderControls)
            {
                wil::com_ptr<IWICBitmapSource> currentImage = sourceImage;

                if ((renderControls.SizeX && renderControls.SizeY) || renderControls.AspectRatio != AspectRatio::OneToOne)
                {
                    UINT targetX = renderControls.SizeX;
                    UINT targetY = renderControls.SizeY;

                    if (!renderControls.StretchSourceToFill)
                    {
                        // We need to calculate which of the sizes needs to be reduced
                        UINT sourceImageX = 0;
                        UINT sourceImageY = 0;
                        THROW_IF_FAILED(currentImage->GetSize(&sourceImageX, &sourceImageY));

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
                    targetY /= AspectRatioMultiplier(renderControls.AspectRatio);

                    wil::com_ptr<IWICBitmapScaler> scaler;
                    THROW_IF_FAILED(factory->CreateBitmapScaler(&scaler));

                    THROW_IF_FAILED(scaler->Initialize(currentImage.get(), targetX, targetY, WICBitmapInterpolationModeHighQualityCubic));
                    currentImage = CacheToBitmap(factory, scaler.get());
                }

                // Create a color palette
                wil::com_ptr<IWICPalette> palette;
                THROW_IF_FAILED(factory->CreatePalette(&palette));

                THROW_IF_FAILED(palette->InitializeFromBitmap(currentImage.get(), renderControls.ColorCount, renderControls.TransparencyEnabled));

                // TODO: Determine if the transparent color is always at index 0
                //       If not, we should swap it to 0 before conversion to indexed

                // Extract the palette for render use
                UINT colorCount = 0;
                THROW_IF_FAILED(palette->GetColorCount(&colorCount));

                m_palette.resize(colorCount);
                UINT actualColorCount = 0;
                THROW_IF_FAILED(palette->GetColors(colorCount, m_palette.data(), &actualColorCount));

                // Convert to 8bpp indexed
                wil::com_ptr<IWICFormatConverter> converter;
                THROW_IF_FAILED(factory->CreateFormatConverter(&converter));

                // TODO: Determine a better value or enable it to be set
                constexpr double s_alphaThreshold = 0.5;

                THROW_IF_FAILED(converter->Initialize(currentImage.get(), GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeErrorDiffusion, palette.get(), s_alphaThreshold, WICBitmapPaletteTypeCustom));
                m_source = CacheToBitmap(factory, converter.get());

                // Lock the image for rendering
                UINT sourceX = 0;
                UINT sourceY = 0;
                THROW_IF_FAILED(currentImage->GetSize(&sourceX, &sourceY));
                THROW_WIN32_IF(ERROR_BUFFER_OVERFLOW,
                    sourceX > static_cast<UINT>(std::numeric_limits<INT>::max()) || sourceY > static_cast<UINT>(std::numeric_limits<INT>::max()));

                WICRect rect{};
                rect.Width = static_cast<INT>(sourceX);
                rect.Height = static_cast<INT>(sourceY);

                THROW_IF_FAILED(m_source->Lock(&rect, WICBitmapLockRead, &m_lockedSource));
                THROW_IF_FAILED(m_lockedSource->GetSize(&m_lockedImageWidth, &m_lockedImageHeight));
                THROW_IF_FAILED(m_lockedSource->GetStride(&m_lockedImageStride));
                THROW_IF_FAILED(m_lockedSource->GetDataPointer(&m_lockedImageByteCount, &m_lockedImageBytes));

                // Create render buffers
                m_enabledColors.resize(m_palette.size());
                m_sixelBuffer.resize(m_palette.size() * m_lockedImageWidth);
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

                    for (size_t i = 0; i < m_palette.size(); ++i)
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
                    UINT rowsToProcess = std::min(Image::PixelsPerSixel, m_lockedImageHeight - m_currentPixelRow);
                    size_t imageStride = static_cast<size_t>(m_lockedImageStride);
                    size_t imageWidth = static_cast<size_t>(m_lockedImageWidth);
                    const BYTE* currentRowPtr = m_lockedImageBytes + (imageStride * m_currentPixelRow);

                    for (UINT rowOffset = 0; rowOffset < rowsToProcess; ++rowOffset)
                    {
                        // The least significant bit is the top of the sixel
                        char sixelBit = 1 << rowOffset;

                        for (size_t i = 0; i < imageWidth; ++i)
                        {
                            BYTE colorIndex = currentRowPtr[i];
                            m_enabledColors[colorIndex] = 1;
                            m_sixelBuffer[(colorIndex * imageWidth) + i] += sixelBit;
                        }

                        currentRowPtr += imageStride;
                    }

                    // Output all sixel color lines
                    bool firstOfRow = true;

                    for (size_t i = 0; i < m_enabledColors.size(); ++i)
                    {
                        // Don't output color if transparent
                        WICColor currentColor = m_palette[i];
                        BYTE alpha = (currentColor >> 24) & 0xFF;
                        if (alpha == 0)
                        {
                            continue;
                        }

                        if (m_enabledColors[i])
                        {
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

                            const char* colorRow = &m_sixelBuffer[i * imageWidth];

                            if (m_renderControls.UseRepeatSequence)
                            {
                                char currentChar = colorRow[0];
                                UINT repeatCount = 1;

                                for (size_t j = 1; j <= imageWidth; ++j)
                                {
                                    // Force processing of a final null character to handle flushing the line
                                    const char nextChar = (j == imageWidth ? 0 : colorRow[j]);

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
                                stream << std::string_view{ colorRow, imageWidth };
                            }
                        }
                    }

                    // The new line operator sets up for the next sixel row
                    stream << '-';

                    m_currentPixelRow += rowsToProcess;
                    if (m_currentPixelRow >= m_lockedImageHeight)
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
            wil::com_ptr<IWICBitmap> m_source;
            wil::com_ptr<IWICBitmapLock> m_lockedSource;
            std::vector<WICColor> m_palette;
            Image::RenderControls m_renderControls;

            UINT m_lockedImageWidth = 0;
            UINT m_lockedImageHeight = 0;
            UINT m_lockedImageStride = 0;
            UINT m_lockedImageByteCount = 0;
            BYTE* m_lockedImageBytes = nullptr;

            State m_currentState = State::Initial;
            std::vector<char> m_enabledColors;
            std::vector<char> m_sixelBuffer;
            UINT m_currentPixelRow = 0;
            // TODO-C++20: Replace with a view from the stringstream
            std::string m_currentSequence;
        };
    }

    Image::Image(const std::filesystem::path& imageFilePath)
    {
        InitializeFactory();

        wil::com_ptr<IWICBitmapDecoder> decoder;
        THROW_IF_FAILED(m_factory->CreateDecoderFromFilename(imageFilePath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));

        wil::com_ptr<IWICBitmapFrameDecode> decodedFrame;
        THROW_IF_FAILED(decoder->GetFrame(0, &decodedFrame));

        m_sourceImage = anon::CacheToBitmap(m_factory.get(), decodedFrame.get());
    }

    Image::Image(std::istream& imageStream, Manifest::IconFileTypeEnum imageEncoding)
    {
        InitializeFactory();

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

    void Image::AspectRatio(Sixel::AspectRatio aspectRatio)
    {
        m_renderControls.AspectRatio = aspectRatio;
    }

    void Image::Transparency(bool transparencyEnabled)
    {
        m_renderControls.TransparencyEnabled = transparencyEnabled;
    }

    void Image::ColorCount(UINT colorCount)
    {
        THROW_HR_IF(E_INVALIDARG, colorCount > MaximumColorCount || colorCount < 2);
        m_renderControls.ColorCount = colorCount;
    }

    void Image::RenderSizeInPixels(UINT x, UINT y)
    {
        m_renderControls.SizeX = x;
        m_renderControls.SizeY = y;
    }

    void Image::RenderSizeInCells(UINT x, UINT y)
    {
        // We don't want to overdraw the row below, so our height must be the largest multiple of 6 that fits in Y cells.
        UINT yInPixels = y * CellHeightInPixels;
        RenderSizeInPixels(x * CellWidthInPixels, yInPixels - (yInPixels % PixelsPerSixel));
    }

    void Image::StretchSourceToFill(bool stretchSourceToFill)
    {
        m_renderControls.StretchSourceToFill = stretchSourceToFill;
    }

    void Image::UseRepeatSequence(bool useRepeatSequence)
    {
        m_renderControls.UseRepeatSequence = useRepeatSequence;
    }

    ConstructedSequence Image::Render()
    {
        anon::RenderState renderState{ m_factory.get(), m_sourceImage, m_renderControls };

        std::stringstream result;

        while (renderState.Advance())
        {
            result << renderState.Current().Get();
        }

        return ConstructedSequence{ std::move(result).str() };
    }

    void Image::RenderTo(Execution::OutputStream& stream)
    {
        anon::RenderState renderState{ m_factory.get(), m_sourceImage, m_renderControls };

        while (renderState.Advance())
        {
            stream << renderState.Current();
        }
    }

    void Image::InitializeFactory()
    {
        THROW_IF_FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_factory)));
    }

    bool SixelsEnabled()
    {
        // TODO: Detect support for sixels in current terminal
        // You can send a DA1 request("\x1b[c") and you'll get "\x1b[?61;1;...;4;...;41c" back. The "61" is the "conformance level" (61-65 = VT100-500, in that order), but you should ignore that because modern terminals lie about their level. The "4" tells you that the terminal supports sixels and I'd recommend testing for that.
        return Settings::User().Get<Settings::Setting::EnableSixelDisplay>();
    }
}
