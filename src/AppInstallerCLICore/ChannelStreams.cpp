// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ChannelStreams.h"


namespace AppInstaller::CLI::Execution
{
    using namespace VirtualTerminal;

#ifndef AICLI_DISABLE_TEST_HOOKS
    static std::optional<size_t>* s_consoleWidthOverride = nullptr;

    void TestHook_SetConsoleWidth_Override(std::optional<size_t>* value)
    {
        s_consoleWidthOverride = value;
    }
#endif

    std::optional<size_t> GetConsoleWidth()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_consoleWidthOverride)
        {
            return *s_consoleWidthOverride;
        }
#endif
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo{};
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo))
        {
            return static_cast<size_t>(consoleInfo.dwSize.X);
        }
        else
        {
            return std::nullopt;
        }
    }

    DWORD GetStandardHandleType(DWORD standardHandle)
    {
        DWORD result = FILE_TYPE_UNKNOWN;

        HANDLE handle = GetStdHandle(standardHandle);
        if (handle != INVALID_HANDLE_VALUE && handle != nullptr)
        {
            result = GetFileType(handle);
        }

        return result;
    }

    bool ShouldTranscodeRedirectedOutput(DWORD standardOutputStreamType, UINT consumerCodePage)
    {
        if (standardOutputStreamType != FILE_TYPE_PIPE)
        {
            // A console renders UTF-8 itself, and a file keeps the UTF-8 that other tools read.
            return false;
        }

        // A code page of 0 means that there is no console to take a code page from, and CP_UTF8
        // is what is written without any conversion.
        return consumerCodePage != 0 && consumerCodePage != CP_UTF8;
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

    Utf8ToCodePageStreamBuf::Utf8ToCodePageStreamBuf(std::ostream& out, UINT codePage) :
        m_out(out), m_target(out.rdbuf()), m_codePage(codePage)
    {
        // Write anything already buffered before taking over the stream.
        m_out.flush();
        m_out.rdbuf(this);
    }

    Utf8ToCodePageStreamBuf::~Utf8ToCodePageStreamBuf()
    {
        Restore();
    }

    void Utf8ToCodePageStreamBuf::Restore()
    {
        if (m_target)
        {
            FlushAll();

            if (m_out.rdbuf() == this)
            {
                m_out.rdbuf(m_target);
            }

            m_target->pubsync();
            m_target = nullptr;
        }
    }

    void Utf8ToCodePageStreamBuf::SetBypass(bool bypass)
    {
        if (m_bypass == bypass)
        {
            return;
        }

        if (bypass)
        {
            // A held sequence can never be completed once the bytes that follow are forwarded unchanged.
            FlushAll();
        }

        m_bypass = bypass;
    }

    std::streambuf::int_type Utf8ToCodePageStreamBuf::overflow(int_type value)
    {
        if (!traits_type::eq_int_type(value, traits_type::eof()))
        {
            const char byte = traits_type::to_char_type(value);
            if (!Put(&byte, 1))
            {
                return traits_type::eof();
            }
        }

        return traits_type::not_eof(value);
    }

    std::streamsize Utf8ToCodePageStreamBuf::xsputn(const char* data, std::streamsize count)
    {
        // Report a failed write to the owning stream instead of claiming that it succeeded.
        return Put(data, static_cast<size_t>(count)) ? count : 0;
    }

    int Utf8ToCodePageStreamBuf::sync()
    {
        if (!Flush())
        {
            return -1;
        }

        return m_target ? m_target->pubsync() : 0;
    }

    // Gets the length of the buffer that ends on a complete UTF-8 sequence.
    static size_t GetCompleteUtf8Length(const std::string& value)
    {
        size_t sequenceStart = value.size();
        size_t continuationBytes = 0;

        while (sequenceStart > 0 && continuationBytes < 3 &&
            (static_cast<unsigned char>(value[sequenceStart - 1]) & 0xC0) == 0x80)
        {
            --sequenceStart;
            ++continuationBytes;
        }

        if (sequenceStart == 0)
        {
            // Braces the whole buffer; let the conversion handle it as invalid.
            return value.size();
        }

        // The lead byte determines how many bytes the sequence needs.
        const unsigned char leadByte = static_cast<unsigned char>(value[sequenceStart - 1]);
        size_t expectedBytes = 1;
        if ((leadByte & 0xE0) == 0xC0)
        {
            expectedBytes = 2;
        }
        else if ((leadByte & 0xF0) == 0xE0)
        {
            expectedBytes = 3;
        }
        else if ((leadByte & 0xF8) == 0xF0)
        {
            expectedBytes = 4;
        }

        return (value.size() - (sequenceStart - 1)) < expectedBytes ? sequenceStart - 1 : value.size();
    }

    bool Utf8ToCodePageStreamBuf::Put(const char* data, size_t count)
    {
        if (m_bypass)
        {
            // Machine readable output is forwarded as it was written, without conversion.
            return Write(data, count);
        }

        m_pending.append(data, count);
        return Flush();
    }

    bool Utf8ToCodePageStreamBuf::Flush()
    {
        if (m_pending.empty())
        {
            return true;
        }

        const size_t completeLength = GetCompleteUtf8Length(m_pending);
        if (completeLength == 0)
        {
            // The output ends with an incomplete UTF-8 sequence; wait for the rest of it.
            return true;
        }

        if (!Convert(std::string_view{ m_pending }.substr(0, completeLength)))
        {
            return false;
        }

        m_pending.erase(0, completeLength);
        return true;
    }

    bool Utf8ToCodePageStreamBuf::FlushAll()
    {
        if (m_pending.empty())
        {
            return true;
        }

        if (!Convert(m_pending))
        {
            return false;
        }

        m_pending.clear();
        return true;
    }

    // Writes all of the given bytes to the target, retrying a short write, and reports whether
    // every byte was written.
    bool Utf8ToCodePageStreamBuf::Write(const char* data, size_t count)
    {
        if (!m_target)
        {
            return false;
        }

        size_t written = 0;
        while (written < count)
        {
            const std::streamsize result = m_target->sputn(data + written, static_cast<std::streamsize>(count - written));
            if (result <= 0)
            {
                return false;
            }

            written += static_cast<size_t>(result);
        }

        return true;
    }

    bool Utf8ToCodePageStreamBuf::Convert(std::string_view value)
    {
        if (value.empty())
        {
            return true;
        }

        if (!m_target)
        {
            return false;
        }

        // Most CLI output is ASCII, which has the same bytes in every ANSI and OEM code page.
        bool ascii = true;
        for (const char c : value)
        {
            if (static_cast<unsigned char>(c) >= 0x80)
            {
                ascii = false;
                break;
            }
        }

        if (ascii)
        {
            return Write(value.data(), value.size());
        }

        DWORD flags = MB_ERR_INVALID_CHARS;
        int wideLength = MultiByteToWideChar(CP_UTF8, flags, value.data(), static_cast<int>(value.size()), nullptr, 0);

        if (wideLength == 0)
        {
            // Let Windows substitute a replacement character for the invalid sequence.
            flags = 0;
            wideLength = MultiByteToWideChar(CP_UTF8, flags, value.data(), static_cast<int>(value.size()), nullptr, 0);
        }

        if (wideLength <= 0)
        {
            return false;
        }

        m_wide.resize(static_cast<size_t>(wideLength));
        if (MultiByteToWideChar(CP_UTF8, flags, value.data(), static_cast<int>(value.size()), m_wide.data(), wideLength) != wideLength)
        {
            return false;
        }

        const int byteLength = WideCharToMultiByte(m_codePage, 0, m_wide.data(), wideLength, nullptr, 0, nullptr, nullptr);
        if (byteLength <= 0)
        {
            return false;
        }

        m_bytes.resize(static_cast<size_t>(byteLength));
        if (WideCharToMultiByte(m_codePage, 0, m_wide.data(), wideLength, m_bytes.data(), byteLength, nullptr, nullptr) != byteLength)
        {
            return false;
        }

        return Write(m_bytes.data(), m_bytes.size());
    }

    RedirectedOutputEncoding::RedirectedOutputEncoding(std::ostream& out, DWORD standardOutputStreamType, UINT consumerCodePage)
    {
        if (ShouldTranscodeRedirectedOutput(standardOutputStreamType, consumerCodePage))
        {
            m_conversion = std::make_unique<Utf8ToCodePageStreamBuf>(out, consumerCodePage);
        }
    }

    RedirectedOutputEncoding::~RedirectedOutputEncoding() = default;

    void RedirectedOutputEncoding::SetMachineReadable(bool machineReadable)
    {
        m_machineReadable = machineReadable;

        if (m_conversion)
        {
            m_conversion->SetBypass(machineReadable);
        }
    }

    bool RedirectedOutputEncoding::IsConverting() const
    {
        return m_conversion != nullptr && !m_machineReadable;
    }
}
