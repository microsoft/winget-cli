// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ChannelStreams.h"


namespace AppInstaller::CLI::Execution
{
    using namespace VirtualTerminal;

    size_t GetConsoleWidth()
    {
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo{};
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo))
        {
            return static_cast<size_t>(consoleInfo.dwSize.X);
        }
        else
        {
            return 120;
        }
    }

    BaseStream::BaseStream(std::ostream& out, bool enabled, bool VTEnabled) :
        m_out(out), m_enabled(enabled), m_VTEnabled(VTEnabled) {}

    BaseStream& BaseStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        if (m_enabled)
        {
            f(m_out);
        }
        return *this;
    }

    BaseStream& BaseStream::operator<<(const Sequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            m_VTUpdated = true;
            m_out << sequence;
        }
        return *this;
    }

    BaseStream& BaseStream::operator<<(const ConstructedSequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            m_VTUpdated = true;
            m_out << sequence;
        }
        return *this;
    }

    void BaseStream::SetVTEnabled(bool enabled)
    {
        m_VTEnabled = enabled;
    }

    void BaseStream::RestoreDefault()
    {
        if (m_VTUpdated)
        {
            Write(TextFormat::Default, true);
        }
    }

    void BaseStream::Disable()
    {
        m_enabled = false;
    }

    std::ostream& BaseStream::Get()
    {
        return m_out;
    }

    OutputStream::OutputStream(BaseStream& out, bool enabled, bool VTEnabled) :
        m_out(out),
        m_enabled(enabled),
        m_VTEnabled(VTEnabled)
    {}

    void OutputStream::AddFormat(const Sequence& sequence)
    {
        m_format.Append(sequence);
    }

    void OutputStream::ClearFormat()
    {
        m_format.Clear();
    }

    void OutputStream::ApplyFormat()
    {
        // Only apply format if m_applyFormatAtOne == 1 coming into this function.
        if (m_applyFormatAtOne)
        {
            if (!--m_applyFormatAtOne)
            {
                m_out << m_format;
            }
        }
    }

    OutputStream& OutputStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        if (m_enabled)
        {
            m_out << f;
        }
        
        return *this;
    }

    OutputStream& OutputStream::operator<<(const Sequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            // Apply format as normal to ensure that any previous format doesn't bleed through.
            ApplyFormat();

            m_out << sequence;

            // An incoming sequence will be valid for 1 "standard" output after this one.
            // We set this to 2 to make that happen, because when it is 1, we will output
            // the format for the current OutputStream.
            m_applyFormatAtOne = 2;
        }
        
        return *this;
    }

    OutputStream& OutputStream::operator<<(const ConstructedSequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            // Apply format as normal to ensure that any previous format doesn't bleed through.
            ApplyFormat();

            m_out << sequence;
            // An incoming sequence will be valid for 1 "standard" output after this one.
            // We set this to 2 to make that happen, because when it is 1, we will output
            // the format for the current OutputStream.
            m_applyFormatAtOne = 2;
        }
        
        return *this;
    }

    OutputStream& OutputStream::operator<<(const std::filesystem::path& path)
    {
        if (m_enabled)
        {
            if (m_VTEnabled)
            {
                ApplyFormat();
            }
            m_out << path.u8string();
        }
        return *this;
        
    }
}
