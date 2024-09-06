// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Sixel.h"
#include <vector>

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
                        THROW_IF_FAILED(sourceImage->GetSize(&sourceImageX, &sourceImageY));

                        double doubleTargetX = targetX;
                        double doubleTargetY = targetY;
                        double doubleSourceImageX = sourceImageX;
                        double doubleSourceImageY = sourceImageY;

                        double scaleFactorX = doubleTargetX / doubleSourceImageX;
                        double targetY_scaledForX = sourceImageY * scaleFactorX;
                        if (targetY_scaledForX > doubleTargetY)
                        {
                            // Scaling to make X fill would make Y to large, so we must scale to fill Y
                            targetX = sourceImageX * (doubleTargetY / doubleSourceImageY);
                        }
                        else
                        {
                            // Scaling to make X fill kept Y under target
                            targetY = targetY_scaledForX;
                        }
                    }

                    // Apply aspect ratio scaling
                    targetY /= AspectRatioMultiplier(renderControls.AspectRatio);

                    wil::com_ptr<IWICBitmapScaler> scaler;
                    THROW_IF_FAILED(factory->CreateBitmapScaler(&scaler));

                    THROW_IF_FAILED(scaler->Initialize(currentImage.get(), targetX, targetY, WICBitmapInterpolationModeHighQualityCubic));
                    currentImage = std::move(scaler);
                }

                // Force evaluation as we will need to read it at least twice more from here
                currentImage = CacheToBitmap(factory, currentImage.get());

                // Create a color palette
                wil::com_ptr<IWICPalette> palette;
                THROW_IF_FAILED(factory->CreatePalette(&palette));

                THROW_IF_FAILED(palette->InitializeFromBitmap(currentImage.get(), renderControls.ColorCount, renderControls.TransparencyEnabled));

                // Convert to 8bpp indexed
                wil::com_ptr<IWICFormatConverter> converter;
                THROW_IF_FAILED(factory->CreateFormatConverter(&converter));

                // TODO: Determine a better value or enable it to be set
                constexpr double s_alphaThreshold = 0.5;

                THROW_IF_FAILED(converter->Initialize(currentImage.get(), GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeErrorDiffusion, palette.get(), s_alphaThreshold, WICBitmapPaletteTypeCustom));
                m_source = std::move(converter);

                // Extract the palette for render use
                UINT colorCount = 0;
                THROW_IF_FAILED(palette->GetColorCount(&colorCount));

                m_palette.resize(colorCount);
                UINT actualColorCount = 0;
                THROW_IF_FAILED(palette->GetColors(colorCount, m_palette.data(), &actualColorCount));
            }

            enum class State
            {
                Initial,
                Palette,
                Pixels,
                Final,
                Terminated,
            };

            // Advances the render state machine, returning true if `Current` will return a new sequence and false when it will not.
            bool Advance()
            {

            }

            Sequence Current() const
            {
                return Sequence{ m_currentSequence.c_str() };
            }

        private:
            // Forces the given bitmap source to evaluate
            static wil::com_ptr<IWICBitmapSource> CacheToBitmap(IWICImagingFactory* factory, IWICBitmapSource* sourceImage)
            {
                wil::com_ptr<IWICBitmap> result;
                THROW_IF_FAILED(factory->CreateBitmapFromSource(sourceImage, WICBitmapCacheOnLoad, &result));
                return result;
            }

            wil::com_ptr<IWICBitmapSource> m_source;
            std::vector<WICColor> m_palette;
            State m_currentState = State::Initial;
            size_t m_currentPaletteIndex = 0;
            size_t m_currentSixel = 0;
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

        m_sourceImage = std::move(decodedFrame);
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

    }

    void SixelImage::RenderTo(Execution::OutputStream& stream)
    {
        // TODO: Optimize
        stream << Render();
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
