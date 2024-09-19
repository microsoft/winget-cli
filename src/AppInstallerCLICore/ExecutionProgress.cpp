// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionProgress.h"
#include "VTSupport.h"

using namespace AppInstaller::Settings;
using namespace AppInstaller::CLI::VirtualTerminal;
using namespace std::string_view_literals;

namespace AppInstaller::CLI::Execution
{
    namespace
    {
        struct BytesFormatData
        {
            uint64_t PowerOfTwo;
            std::string_view Name;
        };

        BytesFormatData s_bytesFormatData[] =
        {
            // Multi-terabyte installers should be fairly rare for the foreseeable future...
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

        void OutputBytes(BaseStream& out, uint64_t byteCount)
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

        void SetColor(BaseStream& out, const TextFormat::Color& color, bool foregroundOnly)
        {
            out << TextFormat::Foreground::Extended(color);

            if (!foregroundOnly)
            {
                constexpr uint8_t divisor = 3;

                auto reduced = color;
                reduced.R /= divisor;
                reduced.G /= divisor;
                reduced.B /= divisor;

                out << TextFormat::Background::Extended(reduced);
            }
        }

        void SetRainbowColor(BaseStream& out, size_t i, size_t max, bool foregroundOnly)
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

            SetColor(out, result, foregroundOnly);
        }
    }

    // Shared functionality for progress visualizers.
    struct ProgressVisualizerBase
    {
        ProgressVisualizerBase(BaseStream& stream, bool enableVT) :
            m_out(stream), m_enableVT(enableVT) {}

        void SetMessage(std::string_view message)
        {
            std::atomic_store(&m_message, std::make_shared<Utility::NormalizedString>(message));
        }

        std::shared_ptr<Utility::NormalizedString> Message()
        {
            return std::atomic_load(&m_message);
        }

    protected:
        BaseStream& m_out;

        bool VT_Enabled() const { return m_enableVT; }

        void ClearLine()
        {
            if (VT_Enabled())
            {
                m_out << TextModification::EraseLineEntirely << '\r';
            }
            else
            {
                m_out << '\r' << std::string(GetConsoleWidth(), ' ') << '\r';
            }
        }

    private:
        bool m_enableVT = false;
        std::shared_ptr<Utility::NormalizedString> m_message;
    };

    // Shared functionality for progress visualizers.
    struct CharacterProgressVisualizerBase : public ProgressVisualizerBase
    {
        CharacterProgressVisualizerBase(BaseStream& stream, bool enableVT, VisualStyle style) :
            ProgressVisualizerBase(stream, enableVT && style != AppInstaller::Settings::VisualStyle::NoVT), m_style(style) {}

    protected:
        Settings::VisualStyle m_style = AppInstaller::Settings::VisualStyle::Accent;

        // Applies the selected visual style.
        void ApplyStyle(size_t i, size_t max, bool foregroundOnly)
        {
            if (!VT_Enabled())
            {
                // Either no style set or VT disabled
                return;
            }
            switch (m_style)
            {
            case VisualStyle::Retro:
                m_out << TextFormat::Default;
                break;
            case VisualStyle::Accent:
                SetColor(m_out, TextFormat::Color::GetAccentColor(), foregroundOnly);
                break;
            case VisualStyle::Rainbow:
                SetRainbowColor(m_out, i, max, foregroundOnly);
                break;
            default:
                LOG_HR(E_UNEXPECTED);
            }
        }
    };

    // Displays an indefinite spinner via a character.
    struct CharacterIndefiniteSpinner : public CharacterProgressVisualizerBase, public IIndefiniteSpinner
    {
        CharacterIndefiniteSpinner(BaseStream& stream, bool enableVT, VisualStyle style) :
            CharacterProgressVisualizerBase(stream, enableVT, style) {}

        void ShowSpinner() override
        {
            if (!m_spinnerJob.valid() && !m_spinnerRunning && !m_canceled)
            {
                m_spinnerRunning = true;
                m_spinnerJob = std::async(std::launch::async, &CharacterIndefiniteSpinner::ShowSpinnerInternal, this);
            }
        }

        void StopSpinner() override
        {
            if (!m_canceled && m_spinnerJob.valid() && m_spinnerRunning)
            {
                m_canceled = true;
                m_spinnerJob.get();
            }
        }

        void SetMessage(std::string_view message) override
        {
            ProgressVisualizerBase::SetMessage(message);
        }

        std::shared_ptr<Utility::NormalizedString> Message() override
        {
            return ProgressVisualizerBase::Message();
        }

    private:
        std::atomic<bool> m_canceled = false;
        std::atomic<bool> m_spinnerRunning = false;
        std::future<void> m_spinnerJob;

        void ShowSpinnerInternal()
        {
            char spinnerChars[] = { '-', '\\', '|', '/' };

            // First wait for a small amount of time to enable a fast task to skip
            // showing anything, or a progress task to skip straight to progress.
            Sleep(100);

            if (!m_canceled)
            {
                if (VT_Enabled())
                {
                    // Additional VT-based progress reporting, for terminals that support it
                    m_out << Progress::Construct(Progress::State::Indeterminate);
                }

                // Indent two spaces for the spinner, but three here so that we can overwrite it in the loop.
                std::string_view indent = "   ";
                std::shared_ptr<Utility::NormalizedString> message = ProgressVisualizerBase::Message();
                size_t messageLength = message ? Utility::UTF8ColumnWidth(*message) : 0;

                for (size_t i = 0; !m_canceled; ++i)
                {
                    constexpr size_t repetitionCount = 20;
                    ApplyStyle(i % repetitionCount, repetitionCount, true);
                    m_out << '\r' << indent << spinnerChars[i % ARRAYSIZE(spinnerChars)];
                    m_out.RestoreDefault();

                    std::shared_ptr<Utility::NormalizedString> newMessage = ProgressVisualizerBase::Message();
                    std::string eraser;
                    if (newMessage)
                    {
                        size_t newLength = Utility::UTF8ColumnWidth(*newMessage);

                        if (newLength < messageLength)
                        {
                            eraser = std::string(messageLength - newLength, ' ');
                        }

                        message = newMessage;
                        messageLength = newLength;
                    }

                    m_out << ' ' << (message ? *message : std::string{}) << eraser << std::flush;
                    Sleep(250);
                }

                ClearLine();

                if (VT_Enabled())
                {
                    m_out << Progress::Construct(Progress::State::None);
                }
            }

            m_canceled = false;
            m_spinnerRunning = false;
        }
    };

    // Displays progress 
    class CharacterProgressBar : public CharacterProgressVisualizerBase, public IProgressBar
    {
    public:
        CharacterProgressBar(BaseStream& stream, bool enableVT, VisualStyle style) :
            CharacterProgressVisualizerBase(stream, enableVT, style) {}

        void ShowProgress(uint64_t current, uint64_t maximum, ProgressType type) override
        {
            if (current < m_lastCurrent)
            {
                ClearLine();
            }

            // TODO: Progress bar does not currently use message
            if (VT_Enabled())
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

        void EndProgress(bool hideProgressWhenDone) override
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

                if (VT_Enabled())
                {
                    // We always clear the VT-based progress bar, even if hideProgressWhenDone is false
                    // since it would be confusing for users if progress continues to be shown after winget exits
                    // (it is typically not automatically cleared by terminals on process exit)
                    m_out << Progress::Construct(Progress::State::None);
                }

                m_isVisible = false;
            }
        }

    private:
        std::atomic<bool> m_isVisible = false;
        uint64_t m_lastCurrent = 0;

        void ShowProgressNoVT(uint64_t current, uint64_t maximum, ProgressType type)
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

        void ShowProgressWithVT(uint64_t current, uint64_t maximum, ProgressType type)
        {
            m_out << TextFormat::Default;

            m_out << "\r  ";

            if (maximum)
            {
                const char* const blocks[] =
                {
                    u8" ",      // block off
                    u8"\x258F", // block 1/8
                    u8"\x258E", // block 2/8
                    u8"\x258D", // block 3/8
                    u8"\x258C", // block 4/8
                    u8"\x258B", // block 5/8
                    u8"\x258A", // block 6/8
                    u8"\x2589", // block 7/8
                    u8"\x2588"  // block on
                };
                const char* const blockOn = blocks[8];
                const char* const blockOff = blocks[0];
                constexpr size_t blockWidth = 30;

                double percentage = static_cast<double>(current) / maximum;
                size_t blocksOn = static_cast<size_t>(std::floor(percentage * blockWidth));
                size_t partialBlockIndex = static_cast<size_t>((percentage * blockWidth - blocksOn) * 8);
                TextFormat::Color accent = TextFormat::Color::GetAccentColor();

                for (size_t i = 0; i < blockWidth; ++i)
                {
                    ApplyStyle(i, blockWidth, false);

                    if (i < blocksOn)
                    {
                        m_out << blockOn;
                    }
                    else if (i == blocksOn)
                    {
                        m_out << blocks[partialBlockIndex];
                    }
                    else
                    {
                        m_out << blockOff;
                    }
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

                // Additional VT-based progress reporting, for terminals that support it
                m_out << Progress::Construct(Progress::State::Normal, static_cast<int>(percentage * 100));
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
    };

    std::unique_ptr<IIndefiniteSpinner> IIndefiniteSpinner::CreateForStyle(BaseStream& stream, bool enableVT, VisualStyle style)
    {
        std::unique_ptr<IIndefiniteSpinner> result;

        switch (style)
        {
        case AppInstaller::Settings::VisualStyle::NoVT:
        case AppInstaller::Settings::VisualStyle::Retro:
        case AppInstaller::Settings::VisualStyle::Accent:
        case AppInstaller::Settings::VisualStyle::Rainbow:
            result = std::make_unique<CharacterIndefiniteSpinner>(stream, enableVT, style);
            break;
        case AppInstaller::Settings::VisualStyle::Sixel:
            // TODO: The magic
            break;
        case AppInstaller::Settings::VisualStyle::Disabled:
            break;
        default:
            THROW_HR(E_NOTIMPL);
        }

        return result;
    }

    std::unique_ptr<IProgressBar> IProgressBar::CreateForStyle(BaseStream& stream, bool enableVT, VisualStyle style)
    {
        std::unique_ptr<IProgressBar> result;

        switch (style)
        {
        case AppInstaller::Settings::VisualStyle::NoVT:
        case AppInstaller::Settings::VisualStyle::Retro:
        case AppInstaller::Settings::VisualStyle::Accent:
        case AppInstaller::Settings::VisualStyle::Rainbow:
            result = std::make_unique<CharacterProgressBar>(stream, enableVT, style);
            break;
        case AppInstaller::Settings::VisualStyle::Sixel:
            // TODO: The magic
            break;
        case AppInstaller::Settings::VisualStyle::Disabled:
            break;
        default:
            THROW_HR(E_NOTIMPL);
        }

        return result;
    }
}
