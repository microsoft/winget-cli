// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>


// The escape character that begins all VT sequences
#define AICLI_VT_ESCAPE     "\x1b"

namespace AppInstaller::CLI::VirtualTerminal
{
    // RAII class to enable VT support and restore the console mode.
    struct ConsoleModeRestore
    {
        ~ConsoleModeRestore();

        ConsoleModeRestore(const ConsoleModeRestore&) = delete;
        ConsoleModeRestore& operator=(const ConsoleModeRestore&) = delete;

        ConsoleModeRestore(ConsoleModeRestore&&) = default;
        ConsoleModeRestore& operator=(ConsoleModeRestore&&) = default;

        // Gets the singleton.
        static const ConsoleModeRestore& Instance();

        // Returns true if VT support has been enabled for the console.
        bool IsVTEnabled() const { return m_token; }

    private:
        ConsoleModeRestore();

        DestructionToken m_token = false;
        DWORD m_previousMode = 0;
    };

    // The base for all VT sequences.
    struct Sequence
    {
        constexpr Sequence() = default;
        explicit constexpr Sequence(std::string_view c) : m_chars(c) {}

        std::string_view Get() const { return m_chars; }

    protected:
        void Set(const std::string& s) { m_chars = s; }

    private:
        std::string_view m_chars;
    };

    // A VT sequence that is constructed at runtime.
    struct ConstructedSequence : public Sequence
    {
        ConstructedSequence() { Set(m_str); }
        explicit ConstructedSequence(std::string s) : m_str(std::move(s)) { Set(m_str); }

        ConstructedSequence(const ConstructedSequence& other) : m_str(other.m_str) { Set(m_str); }
        ConstructedSequence& operator=(const ConstructedSequence& other) { m_str = other.m_str; Set(m_str); return *this; }

        ConstructedSequence(ConstructedSequence&& other) noexcept : m_str(std::move(other.m_str)) { Set(m_str); }
        ConstructedSequence& operator=(ConstructedSequence&& other) noexcept { m_str = std::move(other.m_str); Set(m_str); return *this; }

        void Append(const Sequence& sequence);

        void Clear();

    private:
        std::string m_str;
    };

    // Below are mapped to the sequences described here:
    // https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

    // Contains the response to a DA (Device Attributes) request.
    struct DeviceAttributes
    {
        // Queries the device attributes on creation.
        DeviceAttributes();

        // The set of extensions.
        enum class Extension
        {

        };

        // Determines if the given extension is supported.
        bool Supports(Extension extension) const;

    private:
        uint32_t m_conformanceLevel = 0;
        uint64_t m_extensions = 0;
    };

    namespace Cursor
    {
        namespace Position
        {
            ConstructedSequence Up(int16_t cells);
            ConstructedSequence Down(int16_t cells);
            ConstructedSequence Forward(int16_t cells);
            ConstructedSequence Backward(int16_t cells);
        }

        namespace Visibility
        {
            extern const Sequence EnableBlink;
            extern const Sequence DisableBlink;
            extern const Sequence EnableShow;
            extern const Sequence DisableShow;
        }
    }

    namespace TextFormat
    {
        // Returns all attributes to the default state prior to modification
        extern const Sequence Default;

        // Swaps foreground and background colors
        extern const Sequence Negative;

        // A color, used in constructed sequences.
        struct Color
        {
            uint8_t R;
            uint8_t G;
            uint8_t B;

            static Color GetAccentColor();
        };

        namespace Foreground
        {
            // Applies brightness/intensity flag to foreground color
            extern const Sequence Bright;
            // Removes brightness/intensity flag from foreground color
            extern const Sequence NoBright;

            extern const Sequence BrightRed;
            extern const Sequence BrightGreen;
            extern const Sequence BrightYellow;
            extern const Sequence BrightBlue;
            extern const Sequence BrightMagenta;
            extern const Sequence BrightCyan;
            extern const Sequence BrightWhite;

            ConstructedSequence Extended(const Color& color);
        }

        namespace Background
        {
            ConstructedSequence Extended(const Color& color);
        }

        ConstructedSequence Hyperlink(const std::string& text, const std::string& ref);
    }

    namespace TextModification
    {
        extern const Sequence EraseLineForward;
        extern const Sequence EraseLineBackward;
        extern const Sequence EraseLineEntirely;
    }

    namespace Progress
    {
        enum class State
        {
            None,
            Indeterminate,
            Normal,
            Paused,
            Error
        };

        ConstructedSequence Construct(State state, std::optional<uint32_t> percentage = std::nullopt);
    }
}

inline std::ostream& operator<<(std::ostream& o, const AppInstaller::CLI::VirtualTerminal::Sequence& s)
{
    return (o << s.Get());
}
