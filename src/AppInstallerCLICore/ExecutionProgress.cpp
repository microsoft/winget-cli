// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionProgress.h"

namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;
    using namespace std::string_view_literals;

    namespace
    {
        struct BytesFormatData
        {
            uint64_t PowerOfTwo;
            std::string_view Name;
        };

        BytesFormatData s_bytesFormatData[] =
        {
            // Multi-terabyate installers should be fairly rare for the foreseeable future...
            { 40, "TB"sv },
            { 30, "GB"sv },
            { 20, "MB"sv },
            { 10, "KB"sv },
            { 0, "B"sv },
        };

        const BytesFormatData& GetFormatForSize(uint64_t bytes)
        {
            for (const auto& format : s_bytesFormatData)
            {
                if (bytes > (1ull << format.PowerOfTwo))
                {
                    return format;
                }
            }

            // Just to make the compiler happy, return the last in the list if we get here.
            return s_bytesFormatData[ARRAYSIZE(s_bytesFormatData) - 1];
        }

        void OutputBytes(std::ostream& out, uint64_t byteCount)
        {
            const BytesFormatData& bfd = GetFormatForSize(byteCount);

            uint64_t integralAmount = byteCount >> bfd.PowerOfTwo;
            uint64_t remainder = byteCount & ((1ull << bfd.PowerOfTwo) - 1);
            size_t remainderDigits = 0;

            if (integralAmount < 10)
            {
                remainder *= 100;
                remainderDigits = 2;
            }
            else if (integralAmount < 100)
            {
                remainder *= 10;
                remainderDigits = 1;
            }
            else if (integralAmount < 1000)
            {
                // Put an extra space to ensure a consistent 4 chars per numeric output
                out << ' ';
            }

            out << integralAmount;

            if (remainderDigits)
            {
                remainder = remainder >> bfd.PowerOfTwo;
                out << '.' << std::setw(remainderDigits) << std::setfill('0') << remainder;
            }

            out << ' ' << bfd.Name;
        }

        void SetColor(std::ostream& out, const TextFormat::Color& color, bool enabled)
        {
            if (enabled)
            {
                out << TextFormat::Foreground::Extended(color);
            }
            else
            {
                constexpr uint8_t divisor = 3;

                auto reduced = color;
                reduced.R /= divisor;
                reduced.G /= divisor;
                reduced.B /= divisor;

                out << TextFormat::Foreground::Extended(reduced);
            }
        }

        void SetRainbowColor(std::ostream& out, size_t i, size_t max, bool enabled)
        {
            TextFormat::Color rainbow[] =
            {
                { 0xff, 0x00, 0x00 },
                { 0xff, 0x77, 0x00 },
                { 0xff, 0xdd, 0x00 },
                { 0x00, 0xff, 0x00 },
                { 0x00, 0x00, 0xff },
                { 0x8a, 0x2b, 0xe2 },
                { 0xc7, 0x7d, 0xf3 },
            };

            double target = (static_cast<double>(i) / (max - 1)) * (ARRAYSIZE(rainbow) - 1);
            size_t lower = static_cast<size_t>(std::floor(target));
            const auto& lowerVal = rainbow[lower];
            TextFormat::Color result;

            if (lower == (ARRAYSIZE(rainbow) - 1))
            {
                result = lowerVal;
            }
            else
            {
                double upperContribution = target - lower;

#define AICLI_AVERAGE(v) static_cast<uint8_t>(((lowerVal.v * (1.0 - upperContribution)) + (rainbow[lower + 1].v * upperContribution)))
                result = { AICLI_AVERAGE(R), AICLI_AVERAGE(G), AICLI_AVERAGE(B) };
            }

            SetColor(out, result, enabled);
        }
    }

    namespace details
    {
        void ProgressVisualizerBase::ApplyStyle(size_t i, size_t max, bool enabled)
        {
            switch (m_style)
            {
            case VisualStyle::NoVT:
                // No VT means no style set
                break;
            case VisualStyle::Retro:
                if (enabled)
                {
                    m_out << TextFormat::Default;
                }
                else
                {
                    m_out << TextFormat::Negative;
                }
                break;
            case VisualStyle::Accent:
                SetColor(m_out, TextFormat::Color::GetAccentColor(), enabled);
                break;
            case VisualStyle::Rainbow:
                SetRainbowColor(m_out, i, max, enabled);
                break;
            default:
                LOG_HR(E_UNEXPECTED);
            }
        }
    }

    void IndefiniteSpinner::ShowSpinner()
    {
        if (!m_spinnerJob.valid() && !m_spinnerRunning && !m_canceled)
        {
            m_spinnerRunning = true;
            m_spinnerJob = std::async(std::launch::async, (UseVT() ? &IndefiniteSpinner::ShowSpinnerInternalWithVT : &IndefiniteSpinner::ShowSpinnerInternalNoVT), this);
        }
    }

    void IndefiniteSpinner::StopSpinner()
    {
        if (!m_canceled && m_spinnerJob.valid() && m_spinnerRunning)
        {
            m_canceled = true;
            m_spinnerJob.get();
        }
    }

    void IndefiniteSpinner::ShowSpinnerInternalNoVT()
    {
        char spinnerChars[] = { '-', '\\', '|', '/' };

        // First wait for a small amount of time to enable a fast task to skip
        // showing anything, or a progress task to skip straight to progress.
        Sleep(100);

        // Indent two spaces for the spinner, but three here so that we can overwrite it in the loop.
        m_out << "   ";

        for (size_t i = 0; !m_canceled; ++i)
        {
            constexpr size_t repetitionCount = 20;
            ApplyStyle(i % repetitionCount, repetitionCount, true);
            m_out << '\b' << spinnerChars[i % ARRAYSIZE(spinnerChars)] << std::flush;
            Sleep(250);
        }

        m_out << "\b \r";
        m_canceled = false;
        m_spinnerRunning = false;
    }

    void IndefiniteSpinner::ShowSpinnerInternalWithVT()
    {
        // Nothing special to do at the moment, can use NoVT version.
        ShowSpinnerInternalNoVT();
    }

    void ProgressBar::ShowProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        if (current < m_lastCurrent)
        {
            ClearLine();
        }

        if (UseVT())
        {
            ShowProgressWithVT(current, maximum, type);
        }
        else
        {
            ShowProgressNoVT(current, maximum, type);
        }

        m_lastCurrent = current;
        m_isVisible = true;
    }

    void ProgressBar::EndProgress(bool hideProgressWhenDone)
    {
        if (m_isVisible)
        {
            if (hideProgressWhenDone)
            {
                ClearLine();
            }
            else
            {
                m_out << std::endl;
            }
            m_isVisible = false;
        }
    }

    void ProgressBar::ClearLine()
    {
        if (UseVT())
        {
            m_out << TextModification::EraseLineEntirely << '\r';
        }
        else
        {
            // Best effort when no VT (arbitrary number of spaces that seems to work)
            m_out << "\r                                                              \r";
        }
    }

    void ProgressBar::ShowProgressNoVT(uint64_t current, uint64_t maximum, ProgressType type)
    {
        m_out << "\r  ";

        if (maximum)
        {
            const char* const blockOn = u8"\x2588";
            const char* const blockOff = u8"\x2592";
            constexpr size_t blockWidth = 30;

            double percentage = static_cast<double>(current) / maximum;
            size_t blocksOn = static_cast<size_t>(std::floor(percentage * blockWidth));

            for (size_t i = 0; i < blocksOn; ++i)
            {
                m_out << blockOn;
            }

            for (size_t i = 0; i < blockWidth - blocksOn; ++i)
            {
                m_out << blockOff;
            }

            m_out << "  ";

            switch (type)
            {
            case AppInstaller::ProgressType::Bytes:
                OutputBytes(m_out, current);
                m_out << " / ";
                OutputBytes(m_out, maximum);
                break;
            case AppInstaller::ProgressType::Percent:
            default:
                m_out << static_cast<int>(percentage * 100) << '%';
                break;
            }
        }
        else
        {
            switch (type)
            {
            case AppInstaller::ProgressType::Bytes:
                OutputBytes(m_out, current);
                break;
            case AppInstaller::ProgressType::Percent:
                m_out << current << '%';
                break;
            default:
                m_out << current << " unknowns";
                break;
            }
        }
    }

    void ProgressBar::ShowProgressWithVT(uint64_t current, uint64_t maximum, ProgressType type)
    {
        m_out << "\r  ";

        if (maximum)
        {
            const char* const blockOn = u8"\x2588";
            constexpr size_t blockWidth = 30;

            double percentage = static_cast<double>(current) / maximum;
            size_t blocksOn = static_cast<size_t>(std::floor(percentage * blockWidth));
            TextFormat::Color accent = TextFormat::Color::GetAccentColor();

            for (size_t i = 0; i < blockWidth; ++i)
            {
                ApplyStyle(i, blockWidth, i < blocksOn);
                m_out << blockOn;
            }

            m_out << TextFormat::Default;

            m_out << "  ";

            switch (type)
            {
            case AppInstaller::ProgressType::Bytes:
                OutputBytes(m_out, current);
                m_out << " / ";
                OutputBytes(m_out, maximum);
                break;
            case AppInstaller::ProgressType::Percent:
            default:
                m_out << static_cast<int>(percentage * 100) << '%';
                break;
            }
        }
        else
        {
            switch (type)
            {
            case AppInstaller::ProgressType::Bytes:
                OutputBytes(m_out, current);
                break;
            case AppInstaller::ProgressType::Percent:
                m_out << current << '%';
                break;
            default:
                m_out << current << " unknowns";
                break;
            }
        }
    }
}
