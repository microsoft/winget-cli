// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <ChannelStreams.h>
#include <ExecutionReporter.h>

#include <memory>
#include <sstream>

using namespace AppInstaller::CLI::Execution;

namespace
{
    // The UTF-8 encoding of "Имя" (name).
    constexpr const char* s_utf8Name = "\xD0\x98\xD0\xBC\xD1\x8F";

    // The same text in code page 866.
    constexpr const char* s_cp866Name = "\x88\xAC\xEF";

    // A target buffer that accepts at most the given number of bytes, one per call, so that a
    // conversion has to retry a short write and then observe a refused one.
    struct ShortWriteStreamBuf : public std::streambuf
    {
        explicit ShortWriteStreamBuf(size_t limit) : m_limit(limit) {}

        const std::string& Written() const { return m_written; }

        // Makes the target accept the given number of additional bytes, as a pipe whose reader
        // resumed would.
        void Recover(size_t additional = 16) { m_limit = m_written.size() + additional; }

    protected:
        std::streamsize xsputn(const char* data, std::streamsize count) override
        {
            if (count <= 0 || m_written.size() >= m_limit)
            {
                return 0;
            }

            m_written.push_back(*data);
            return 1;
        }

        int_type overflow(int_type value) override
        {
            if (traits_type::eq_int_type(value, traits_type::eof()))
            {
                return traits_type::not_eof(value);
            }

            const char byte = traits_type::to_char_type(value);
            return xsputn(&byte, 1) == 1 ? traits_type::not_eof(value) : traits_type::eof();
        }

    private:
        size_t m_limit = 0;
        std::string m_written;
    };
}

TEST_CASE("ShouldTranscodeRedirectedOutput", "[ChannelStreams]")
{
    REQUIRE(ShouldTranscodeRedirectedOutput(FILE_TYPE_PIPE, 866));
    REQUIRE(ShouldTranscodeRedirectedOutput(FILE_TYPE_PIPE, 1251));

    // A consumer that already decodes UTF-8 needs no conversion, and neither does one that has no
    // code page to offer (a process without a console).
    REQUIRE_FALSE(ShouldTranscodeRedirectedOutput(FILE_TYPE_PIPE, CP_UTF8));
    REQUIRE_FALSE(ShouldTranscodeRedirectedOutput(FILE_TYPE_PIPE, 0));

    // A console renders UTF-8 itself, and a file keeps the UTF-8 that other tools read.
    REQUIRE_FALSE(ShouldTranscodeRedirectedOutput(FILE_TYPE_CHAR, 866));
    REQUIRE_FALSE(ShouldTranscodeRedirectedOutput(FILE_TYPE_DISK, 866));
    REQUIRE_FALSE(ShouldTranscodeRedirectedOutput(FILE_TYPE_UNKNOWN, 866));
}

TEST_CASE("Utf8ToCodePageStreamBuf_ConvertsOutput", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);
        output << s_utf8Name;
        output.flush();
    }

    REQUIRE(output.str() == s_cp866Name);
}

TEST_CASE("Utf8ToCodePageStreamBuf_HoldsIncompleteSequence", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        // The first byte of a two byte sequence; the rest of it has not arrived yet.
        output.write("\xD0", 1);
        output.flush();
        REQUIRE(output.str().empty());

        output.write("\x98", 1);
        output.flush();
        REQUIRE(output.str() == "\x88");
    }
}

TEST_CASE("Utf8ToCodePageStreamBuf_HoldsSplitThreeByteSequence", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        // The truncation marker, written one byte at a time.
        output.write("\xE2", 1);
        output.flush();
        REQUIRE(output.str().empty());

        output.write("\x80", 1);
        output.flush();
        REQUIRE(output.str().empty());

        output.write("\xA6", 1);
        output.flush();
        REQUIRE(output.str() == ":");
    }
}

TEST_CASE("RedirectedOutputEncoding_SwitchesCodePage", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        RedirectedOutputEncoding encoding{ output, FILE_TYPE_PIPE, 866 };
        output << s_utf8Name;
        REQUIRE(output.str() == s_cp866Name);

        // A single invocation can pick the code page that the output is written in.
        encoding.SetConsumerCodePage(1251);
        output << s_utf8Name;
        REQUIRE(output.str() == std::string{ s_cp866Name } + "\xC8\xEC\xFF");

        // UTF-8 leaves the output as it is written.
        encoding.SetConsumerCodePage(CP_UTF8);
        output << s_utf8Name;
        REQUIRE(output.str() == std::string{ s_cp866Name } + "\xC8\xEC\xFF" + s_utf8Name);
    }
}

TEST_CASE("Utf8ToCodePageStreamBuf_RestoresOriginalBuffer", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);
        output << s_utf8Name;
    }

    // The stream is no longer converted, so the output is UTF-8 again.
    output << s_utf8Name;

    REQUIRE(output.str() == std::string(s_cp866Name) + s_utf8Name);
}

TEST_CASE("Utf8ToCodePageStreamBuf_ReplacesUnmappableCharacters", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        // The Ukrainian small letter i; code page 866 does not have a character for it.
        output << "\xD1\x96";
    }

    REQUIRE(output.str() == "?");
}

TEST_CASE("Utf8ToCodePageStreamBuf_UsesCodePageBestFit", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        // The truncation marker that the tables use; code page 866 represents it as a colon.
        output << "\xE2\x80\xA6";
    }

    REQUIRE(output.str() == ":");
}

TEST_CASE("Utf8ToCodePageStreamBuf_PassesAsciiThrough", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);
        output << "Name Id Version\n";
    }

    REQUIRE(output.str() == "Name Id Version\n");
}

TEST_CASE("Utf8ToCodePageStreamBuf_BypassesMachineReadableOutput", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);
        buffer.SetBypass(true);
        output << s_utf8Name;
        output.flush();
    }

    REQUIRE(output.str() == s_utf8Name);
}

TEST_CASE("Utf8ToCodePageStreamBuf_WritesHeldOutputWhenBypassed", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        output.write("\xD0", 1);
        output.flush();
        REQUIRE(output.str().empty());

        // A held sequence can never be completed once the following bytes bypass the conversion,
        // so it is written immediately instead of being lost.
        buffer.SetBypass(true);
        REQUIRE(output.str() == "?");

        output << "\xD0\x98";
        REQUIRE(output.str() == "?\xD0\x98");
    }
}

TEST_CASE("Utf8ToCodePageStreamBuf_ReportsWriteFailure", "[ChannelStreams]")
{
    // The target accepts three bytes in total and then refuses the rest.
    ShortWriteStreamBuf target{ 3 };
    std::ostream output{ &target };

    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        // A single byte goes through overflow; the target accepts it.
        output.put('N');
        REQUIRE_FALSE(output.bad());

        // The conversion produces three bytes. The short write is retried, and the refusal that
        // follows is reported to the stream instead of being discarded silently.
        const std::string utf8Name = s_utf8Name;
        output.write(utf8Name.data(), utf8Name.size());
        REQUIRE(output.bad());
    }

    REQUIRE(target.Written() == std::string{ "N" } + std::string{ s_cp866Name }.substr(0, 2));

    // Restoring the original buffer does not hide the failure from the caller.
    REQUIRE(output.bad());
}

TEST_CASE("Utf8ToCodePageStreamBuf_ReportsBypassWriteFailure", "[ChannelStreams]")
{
    // The target accepts two bytes in total and then refuses the rest.
    ShortWriteStreamBuf target{ 2 };
    std::ostream output{ &target };

    {
        Utf8ToCodePageStreamBuf buffer(output, 866);
        buffer.SetBypass(true);

        // Bypassed output is forwarded unchanged, and a refused write is still reported.
        const std::string utf8Name = s_utf8Name;
        output.write(utf8Name.data(), utf8Name.size());
        REQUIRE(output.bad());
    }

    REQUIRE(target.Written() == std::string{ s_utf8Name }.substr(0, 2));

    // Restoring the original buffer does not hide the failure from the caller.
    REQUIRE(output.bad());
}

TEST_CASE("Utf8ToCodePageStreamBuf_DoesNotReplayPartiallyWrittenOutput", "[ChannelStreams]")
{
    // The target accepts two bytes, refuses the rest, and then accepts output again.
    ShortWriteStreamBuf target{ 2 };
    std::ostream output{ &target };

    {
        Utf8ToCodePageStreamBuf buffer(output, 866);

        // 'A' and 'B' are written before the target refuses 'C'.
        output.write("ABCDE", 5);
        REQUIRE(output.bad());

        target.Recover();
    }

    // The bytes that were emitted before the failure are not written again when the held output is
    // flushed, so the recovery cannot turn "ABCDE" into "ABABCDE".
    REQUIRE(target.Written() == "AB");
    REQUIRE(output.bad());
}

TEST_CASE("RedirectedOutputEncoding_SkipsFileRedirect", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        RedirectedOutputEncoding encoding(output, FILE_TYPE_DISK, 866);
        REQUIRE_FALSE(encoding.IsConverting());

        output << s_utf8Name;
    }

    REQUIRE(output.str() == s_utf8Name);
}

TEST_CASE("RedirectedOutputEncoding_ConvertsPipeOutput", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        RedirectedOutputEncoding encoding(output, FILE_TYPE_PIPE, 866);
        REQUIRE(encoding.IsConverting());

        output << s_utf8Name;
    }

    REQUIRE(output.str() == s_cp866Name);
}

TEST_CASE("RedirectedOutputEncoding_BypassesMachineReadableOutput", "[ChannelStreams]")
{
    std::ostringstream output;
    {
        RedirectedOutputEncoding encoding(output, FILE_TYPE_PIPE, 866);
        encoding.SetMachineReadable(true);
        REQUIRE_FALSE(encoding.IsConverting());

        output << s_utf8Name;

        encoding.SetMachineReadable(false);
        REQUIRE(encoding.IsConverting());

        output << s_utf8Name;
    }

    REQUIRE(output.str() == std::string{ s_utf8Name } + s_cp866Name);
}

TEST_CASE("Reporter_AppliesEncodingToOutputChannelOnly", "[ChannelStreams]")
{
    std::ostringstream output;
    std::istringstream input;
    auto encoding = std::make_shared<RedirectedOutputEncoding>(output, FILE_TYPE_PIPE, 866);

    Reporter reporter{ output, input };
    reporter.SetStyle(AppInstaller::Settings::VisualStyle::NoVT);
    reporter.SetOutputEncoding(encoding);

    REQUIRE(encoding->IsConverting());

    reporter.Info() << s_utf8Name;
    REQUIRE(output.str() == s_cp866Name);

    // Machine readable output is passed through unchanged for the consumer that parses it.
    reporter.SetChannel(Reporter::Channel::Json);
    REQUIRE_FALSE(encoding->IsConverting());

    reporter.Json() << s_utf8Name;
    REQUIRE(output.str() == std::string{ s_cp866Name } + s_utf8Name);
}

TEST_CASE("Reporter_ReappliesEncodingAfterMachineReadableChannel", "[ChannelStreams]")
{
    std::ostringstream output;
    std::istringstream input;
    auto encoding = std::make_shared<RedirectedOutputEncoding>(output, FILE_TYPE_PIPE, 866);

    Reporter reporter{ output, input };
    reporter.SetStyle(AppInstaller::Settings::VisualStyle::NoVT);
    reporter.SetOutputEncoding(encoding);

    {
        // A clone that reports to a machine readable channel leaves the shared encoding bypassed.
        Reporter clone{ reporter, Reporter::clone_t{} };
        clone.SetChannel(Reporter::Channel::Completion);
        REQUIRE_FALSE(encoding->IsConverting());

        clone.Completion() << s_utf8Name;
    }

    // The original reporter applies its own channel again when it next writes.
    reporter.Info() << s_utf8Name;

    REQUIRE(output.str() == std::string{ s_utf8Name } + s_cp866Name);
}
