// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Sixel.h"
#include <vector>
#include <sstream>

namespace AppInstaller::CLI::VirtualTerminal
{
    namespace anon
    {
        UINT AspectRatioMultiplier(SixelAspectRatio aspectRatio)
        {
            switch (aspectRatio)
            {
            case SixelAspectRatio::OneToOne:
                return 1;
            case SixelAspectRatio::TwoToOne:
                return 2;
            case SixelAspectRatio::ThreeToOne:
                return 3;
            case SixelAspectRatio::FiveToOne:
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

        // Contains the state for a rendering pass.
        struct RenderState
        {
            RenderState(
                IWICImagingFactory* factory,
                wil::com_ptr<IWICBitmapSource>& sourceImage,
                const SixelImage::RenderControls& renderControls)
            {
                wil::com_ptr<IWICBitmapSource> currentImage = sourceImage;

                if ((renderControls.SizeX && renderControls.SizeY) || renderControls.AspectRatio != SixelAspectRatio::OneToOne)
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
                // TODO: See if we can keep a stringstream around to reuse its memory

                switch (m_currentState)
                {
                case State::Initial:
                    // TODO: Output sixel initialization sequence and palette
                    m_currentState = State::Pixels;
                    break;
                case State::Pixels:
                    // TODO: Output a row of sixels
                    break;
                case State::Final:
                    // TODO: Ouput the sixel termination sequence
                    m_currentState = State::Terminated;
                    break;
                case State::Terminated:
                    m_currentSequence.clear();
                    return false;
                }

                return true;
            }

            Sequence Current() const
            {
                return Sequence{ m_currentSequence };
            }

        private:
            wil::com_ptr<IWICBitmap> m_source;
            std::vector<WICColor> m_palette;
            State m_currentState = State::Initial;
            size_t m_currentSixelRow = 0;
            // TODO-C++20: Replace with a view from the stringstream
            std::string m_currentSequence;
        };
    }

    SixelImage::SixelImage(const std::filesystem::path& imageFilePath)
    {
        InitializeFactory();

        wil::com_ptr<IWICBitmapDecoder> decoder;
        THROW_IF_FAILED(m_factory->CreateDecoderFromFilename(imageFilePath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder));

        wil::com_ptr<IWICBitmapFrameDecode> decodedFrame;
        THROW_IF_FAILED(decoder->GetFrame(0, &decodedFrame));

        m_sourceImage = anon::CacheToBitmap(m_factory.get(), decodedFrame.get());
    }

    void SixelImage::AspectRatio(SixelAspectRatio aspectRatio)
    {
        m_renderControls.AspectRatio = aspectRatio;
    }

    void SixelImage::Transparency(bool transparencyEnabled)
    {
        m_renderControls.TransparencyEnabled = transparencyEnabled;
    }

    void SixelImage::ColorCount(UINT colorCount)
    {
        THROW_HR_IF(E_INVALIDARG, colorCount > MaximumColorCount || colorCount < 2);
        m_renderControls.ColorCount = colorCount;
    }

    void SixelImage::RenderSizeInPixels(UINT x, UINT y)
    {
        m_renderControls.SizeX = x;
        m_renderControls.SizeY = y;
    }

    void SixelImage::RenderSizeInCells(UINT x, UINT y)
    {
        // We don't want to overdraw the row below, so our height must be the largest multiple of 6 that fits in Y cells.
        UINT yInPixels = y * CellHeightInPixels;
        RenderSizeInPixels(x * CellWidthInPixels, yInPixels - (yInPixels % PixelsPerSixel));
    }

    void SixelImage::StretchSourceToFill(bool stretchSourceToFill)
    {
        m_renderControls.StretchSourceToFill = stretchSourceToFill;
    }

    ConstructedSequence SixelImage::Render()
    {
        anon::RenderState renderState{ m_factory.get(), m_sourceImage, m_renderControls };

        std::stringstream result;

        while (renderState.Advance())
        {
            result << renderState.Current().Get();
        }

        return ConstructedSequence{ std::move(result).str() };
    }

    void SixelImage::RenderTo(Execution::OutputStream& stream)
    {
        anon::RenderState renderState{ m_factory.get(), m_sourceImage, m_renderControls };

        while (renderState.Advance())
        {
            stream << renderState.Current();
        }
    }

    void SixelImage::InitializeFactory()
    {
        THROW_IF_FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_factory)));
    }
}
