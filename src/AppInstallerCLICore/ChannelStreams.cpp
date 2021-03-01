// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ChannelStreams.h"


namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;

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
            m_out << sequence;
        }
        return *this;
    }

    BaseStream& BaseStream::operator<<(const ConstructedSequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            m_out << sequence;
        }
        return *this;
    }

    OutputStream::OutputStream(std::ostream& out, bool enabled, bool VTEnabled) :
        m_out(out, enabled, VTEnabled) {}

    void OutputStream::AddFormat(const Sequence& sequence)
    {
        m_format.Append(sequence);
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
        m_out << f;
        return *this;
    }

    OutputStream& OutputStream::operator<<(const Sequence& sequence)
    {
        m_out << sequence;
        // An incoming sequence will be valid for 1 "standard" output after this one.
        // We set this to 2 to make that happen, because when it is 1, we will output
        // the format for the current OutputStream.
        m_applyFormatAtOne = 2;
        return *this;
    }

    OutputStream& OutputStream::operator<<(const ConstructedSequence& sequence)
    {
        m_out << sequence;
        // An incoming sequence will be valid for 1 "standard" output after this one.
        // We set this to 2 to make that happen, because when it is 1, we will output
        // the format for the current OutputStream.
        m_applyFormatAtOne = 2;
        return *this;
    }

    OutputStream& OutputStream::operator<<(const std::filesystem::path& path)
    {
        ApplyFormat();
        m_out << path.u8string();
        return *this;
    }

    NoVTStream::NoVTStream(std::ostream& out, bool enabled) :
        m_out(out, enabled, false) {}

    NoVTStream& NoVTStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        m_out << f;
        return *this;
    }
}
