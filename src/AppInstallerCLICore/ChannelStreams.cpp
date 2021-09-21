// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ChannelStreams.h"


namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;

    BaseOutputStream::BaseOutputStream(std::ostream& out, bool enabled, bool VTEnabled, VirtualTerminal::Sequence sequence) :
        m_out(out), m_enabled(enabled), m_VTEnabled(VTEnabled), m_defaultSequence(sequence) {}

    BaseOutputStream& BaseOutputStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        if (m_enabled)
        {
            f(m_out);
        }
        return *this;
    }

    BaseOutputStream& BaseOutputStream::operator<<(const Sequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            m_out << sequence;
        }
        return *this;
    }

    BaseOutputStream& BaseOutputStream::operator<<(const ConstructedSequence& sequence)
    {
        if (m_enabled && m_VTEnabled)
        {
            m_out << sequence;
        }
        return *this;
    }

    void BaseOutputStream::Close() 
    {
        Disable();
        if (m_VTEnabled)
        {
            Write(m_defaultSequence, true);
        }
    }

    VTOutputStream::VTOutputStream(std::ostream& out, bool enabled) :
        BaseOutputStream(out, enabled, true) {}

    void VTOutputStream::AddFormat(const Sequence& sequence)
    {
        m_format.Append(sequence);
    }

    void VTOutputStream::ApplyFormat()
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

    VTOutputStream& VTOutputStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        m_out << f;
        return *this;
    }

    VTOutputStream& VTOutputStream::operator<<(const Sequence& sequence)
    {
        m_out << sequence;
        // An incoming sequence will be valid for 1 "standard" output after this one.
        // We set this to 2 to make that happen, because when it is 1, we will output
        // the format for the current OutputStream.
        m_applyFormatAtOne = 2;
        return *this;
    }

    VTOutputStream& VTOutputStream::operator<<(const ConstructedSequence& sequence)
    {
        m_out << sequence;
        // An incoming sequence will be valid for 1 "standard" output after this one.
        // We set this to 2 to make that happen, because when it is 1, we will output
        // the format for the current OutputStream.
        m_applyFormatAtOne = 2;
        return *this;
    }

    VTOutputStream& VTOutputStream::operator<<(const std::filesystem::path& path)
    {
        ApplyFormat();
        m_out << path.u8string();
        return *this;
    }

    NoVTOutputStream::NoVTOutputStream(std::ostream& out, bool enabled) :
        BaseOutputStream(out, enabled, false) {}

    NoVTOutputStream& NoVTOutputStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        m_out << f;
        return *this;
    }
}
