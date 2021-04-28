// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>


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
        Sequence() = default;
        explicit Sequence(const char* c) : m_chars(c) {}

        const char* Get() const { return m_chars; }

    protected:
        void Set(const std::string& s) { m_chars = s.c_str(); }

    private:
        const char* m_chars = nullptr;
    };

    // A VT sequence that is constructed at runtime.
    struct ConstructedSequence : public Sequence
    {
        ConstructedSequence() { Set(m_str); }
        explicit ConstructedSequence(std::string s) : m_str(std::move(s)) { Set(m_str); }

        ConstructedSequence(const ConstructedSequence& other) : m_str(other.m_str) { Set(m_str); }
        ConstructedSequence& operator=(const ConstructedSequence& other) { m_str = other.m_str; Set(m_str); }

        ConstructedSequence(ConstructedSequence&& other) noexcept : m_str(std::move(other.m_str)) { Set(m_str); }
        ConstructedSequence& operator=(ConstructedSequence&& other) noexcept { m_str = std::move(other.m_str); Set(m_str); }

        void Append(const Sequence& sequence);

    private:
        std::string m_str;
    };

    // Below are mapped to the sequences described here:
    // https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

    namespace Cursor
    {
        namespace Position
        {
            extern const Sequence UpOne;
            extern const Sequence DownOne;
            extern const Sequence ForwardOne;
            extern const Sequence BackwardOne;
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
