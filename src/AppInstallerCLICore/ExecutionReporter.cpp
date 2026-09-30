// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionReporter.h"
#include <AppInstallerErrors.h>
#include <AppInstallerTelemetry.h>
#include <json/json.h>

#include <mutex>


namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;

    struct Reporter::StructuredOutputState
    {
        struct Message
        {
            std::string Code;
            std::string MessageText;
            std::optional<std::string> Source;
            HRESULT Result = S_OK;
        };

        std::mutex Lock;
        std::string Command;
        std::optional<StructuredOutput::Mode> Mode;
        StructuredOutput::PackageResult Result;
        std::vector<Message> Warnings;
        std::vector<Message> Errors;
        bool Finalized = false;
    };

    const Sequence& HelpCommandEmphasis = TextFormat::Foreground::Bright;
    const Sequence& HelpArgumentEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ManifestInfoEmphasis = TextFormat::Foreground::Bright;
    const Sequence& SourceInfoEmphasis = TextFormat::Foreground::Bright;
    const Sequence& NameEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& IdEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& UrlEmphasis = TextFormat::Foreground::BrightBlue;
    const Sequence& PromptEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ConvertToUpgradeFlowEmphasis = TextFormat::Foreground::BrightYellow;
    const Sequence& ConfigurationIntentEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ConfigurationUnitEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& AuthenticationEmphasis = TextFormat::Foreground::BrightYellow;

    namespace
    {
        constexpr std::string_view s_SchemaVersion = "1.0";

        std::string_view ModeToString(StructuredOutput::Mode mode)
        {
            switch (mode)
            {
            case StructuredOutput::Mode::Installed:
                return "installed";
            case StructuredOutput::Mode::AvailableUpgrades:
                return "availableUpgrades";
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        std::string GetErrorCode(HRESULT value)
        {
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << static_cast<uint32_t>(value);
            return stream.str();
        }

        template <typename T>
        Json::Value MessageToJson(const T& message)
        {
            Json::Value result{ Json::ValueType::objectValue };
            result["code"] = message.Code;
            result["message"] = message.MessageText;
            result["source"] = message.Source ? Json::Value{ *message.Source } : Json::Value{};
            return result;
        }

        DWORD GetStdHandleType(DWORD stdHandle)
        {
            DWORD result = FILE_TYPE_UNKNOWN;

            HANDLE handle = GetStdHandle(stdHandle);
            if (handle != INVALID_HANDLE_VALUE && handle != NULL)
            {
                result = GetFileType(handle);
            }

            return result;
        }
    }

    Reporter::Reporter() :
        Reporter(std::cout, std::cin)
    {
        m_outStreamFileType = GetStdHandleType(STD_OUTPUT_HANDLE);
        m_inStreamFileType = GetStdHandleType(STD_INPUT_HANDLE);
    }

    Reporter::Reporter(std::ostream& outStream, std::istream& inStream) :
        Reporter(std::make_shared<BaseStream>(outStream, true, ConsoleModeRestore::Instance().IsVTEnabled()), inStream)
    {
        SetProgressSink(this);
    }

    Reporter::Reporter(std::shared_ptr<BaseStream> outStream, std::istream& inStream) :
        m_out(outStream),
        m_in(inStream)
    {
        // Only create spinner and progress bar when stdout is attached to a console.
        // When output is redirected to a file or pipe, suppress all progress output
        // so it does not appear in the redirected stream.
        if (GetConsoleWidth().has_value())
        {
            auto sixelSupported = [&]() { return SixelsSupported(); };
            m_spinner = IIndefiniteSpinner::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), VisualStyle::Accent, sixelSupported);
            m_progressBar = IProgressBar::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), VisualStyle::Accent, sixelSupported);
        }

        SetProgressSink(this);
    }

    Reporter::~Reporter()
    {
        this->CloseOutputStream();
    }

    Reporter::Reporter(const Reporter& other, clone_t) :
        Reporter(other.m_out, other.m_in)
    {
        m_outStreamFileType = other.m_outStreamFileType;
        m_inStreamFileType = other.m_inStreamFileType;

        SetChannel(other.m_channel);
        m_structuredOutput = other.m_structuredOutput;

        if (other.m_style.has_value())
        {
            SetStyle(*other.m_style);
        }
    }

    std::optional<PrimaryDeviceAttributes> Reporter::GetPrimaryDeviceAttributes()
    {
        if (ConsoleModeRestore::Instance().IsVTEnabled())
        {
            return PrimaryDeviceAttributes{ m_out->Get(), m_in };
        }
        else
        {
            return std::nullopt;
        }
    }

    OutputStream Reporter::GetOutputStream(Level level)
    {
        // If the level is not enabled, return a default stream which is disabled
        if (WI_AreAllFlagsClear(m_enabledLevels, level))
        {
            return OutputStream(*m_out, false, false);
        }

        OutputStream result = GetBasicOutputStream();

        switch (level)
        {
        case Level::Verbose:
            result.AddFormat(TextFormat::Default);
            break;
        case Level::Info:
            result.AddFormat(TextFormat::Default);
            break;
        case Level::Warning:
            result.AddFormat(TextFormat::Foreground::BrightYellow);
            break;
        case Level::Error:
            result.AddFormat(TextFormat::Foreground::BrightRed);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    OutputStream Reporter::GetBasicOutputStream()
    {
        return { *m_out, m_channel == Channel::Output };
    }

    void Reporter::SetChannel(Channel channel)
    {
        m_channel = channel;

        if (m_channel != Channel::Output)
        {
            // Disable progress for non-output channels
            m_spinner.reset();
            m_progressBar.reset();
        }
    }

    void Reporter::BeginStructuredOutput(std::string_view command, std::optional<StructuredOutput::Mode> mode)
    {
        if (!m_structuredOutput)
        {
            m_structuredOutput = std::make_shared<StructuredOutputState>();
            m_structuredOutput->Command = command;
            m_structuredOutput->Mode = mode;
        }

        SetChannel(Channel::Json);
    }

    bool Reporter::IsStructuredOutputEnabled() const
    {
        return static_cast<bool>(m_structuredOutput);
    }

    void Reporter::SetStructuredOutputResult(StructuredOutput::PackageResult result)
    {
        if (!m_structuredOutput)
        {
            return;
        }

        std::scoped_lock lock{ m_structuredOutput->Lock };
        m_structuredOutput->Result = std::move(result);
    }

    void Reporter::AddStructuredOutputWarning(std::string_view code, std::string_view message, std::optional<std::string_view> source)
    {
        if (!m_structuredOutput)
        {
            return;
        }

        std::scoped_lock lock{ m_structuredOutput->Lock };
        m_structuredOutput->Warnings.push_back({ std::string{ code }, std::string{ message }, source ? std::optional<std::string>{ *source } : std::nullopt });
    }

    void Reporter::AddStructuredOutputError(HRESULT code, std::string_view message, std::optional<std::string_view> source)
    {
        if (!m_structuredOutput)
        {
            return;
        }

        std::scoped_lock lock{ m_structuredOutput->Lock };
        m_structuredOutput->Errors.push_back({ GetErrorCode(code), std::string{ message }, source ? std::optional<std::string>{ *source } : std::nullopt, code });
    }

    bool Reporter::HasStructuredOutputErrors() const
    {
        if (!m_structuredOutput)
        {
            return false;
        }

        std::scoped_lock lock{ m_structuredOutput->Lock };
        return !m_structuredOutput->Errors.empty();
    }

    HRESULT Reporter::GetStructuredOutputError() const
    {
        if (!m_structuredOutput)
        {
            return S_OK;
        }

        std::scoped_lock lock{ m_structuredOutput->Lock };
        return m_structuredOutput->Errors.empty() ? S_OK : m_structuredOutput->Errors.front().Result;
    }

    void Reporter::FinalizeStructuredOutput()
    {
        if (!m_structuredOutput)
        {
            return;
        }

        std::scoped_lock lock{ m_structuredOutput->Lock };
        if (m_structuredOutput->Finalized)
        {
            return;
        }

        Json::Value document{ Json::ValueType::objectValue };
        if (m_structuredOutput->Mode)
        {
            document["$schema"] = "https://aka.ms/winget-cli-output." + m_structuredOutput->Command + ".1.0.schema.json";
        }
        else
        {
            document["$schema"] = "https://aka.ms/winget-cli-output.error.1.0.schema.json";
        }

        document["schemaVersion"] = std::string{ s_SchemaVersion };
        document["command"] = m_structuredOutput->Command;

        if (m_structuredOutput->Mode)
        {
            document["mode"] = std::string{ ModeToString(*m_structuredOutput->Mode) };

            Json::Value result{ Json::ValueType::objectValue };
            Json::Value packages{ Json::ValueType::arrayValue };
            for (const auto& package : m_structuredOutput->Result.Packages)
            {
                Json::Value packageJson{ Json::ValueType::objectValue };
                packageJson["Name"] = package.Name;
                packageJson["Id"] = package.Id;
                packageJson["InstalledVersion"] = package.InstalledVersion;

                Json::Value versions{ Json::ValueType::arrayValue };
                for (const auto& version : package.AvailableVersions)
                {
                    versions.append(version);
                }
                packageJson["AvailableVersions"] = std::move(versions);
                packageJson["IsUpdateAvailable"] = package.IsUpdateAvailable;
                packageJson["Source"] = package.Source ? Json::Value{ *package.Source } : Json::Value{};
                packageJson["UpgradeVersion"] = package.UpgradeVersion ? Json::Value{ *package.UpgradeVersion } : Json::Value{};
                packages.append(std::move(packageJson));
            }

            result["packages"] = std::move(packages);
            result["truncated"] = m_structuredOutput->Result.Truncated;
            document["result"] = std::move(result);
        }
        else
        {
            document["mode"] = Json::Value{};
            document["result"] = Json::Value{};
        }

        Json::Value warnings{ Json::ValueType::arrayValue };
        for (const auto& warning : m_structuredOutput->Warnings)
        {
            warnings.append(MessageToJson(warning));
        }
        document["warnings"] = std::move(warnings);

        Json::Value errors{ Json::ValueType::arrayValue };
        for (const auto& error : m_structuredOutput->Errors)
        {
            errors.append(MessageToJson(error));
        }
        document["errors"] = std::move(errors);

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder.settings_["indentation"] = "";
        writerBuilder.settings_["commentStyle"] = "None";
        writerBuilder.settings_["emitUTF8"] = true;
        Json() << Json::writeString(writerBuilder, document) << std::endl;

        std::string_view outcome = "success";
        if (!m_structuredOutput->Errors.empty())
        {
            outcome = m_structuredOutput->Result.Packages.empty() ? "failure" : "partialFailure";
        }
        else if (!m_structuredOutput->Warnings.empty())
        {
            outcome = "warning";
        }

        Logging::Telemetry().LogStructuredOutput(
            m_structuredOutput->Command,
            m_structuredOutput->Mode ? ModeToString(*m_structuredOutput->Mode) : "unsupported",
            1,
            outcome);
        m_structuredOutput->Finalized = true;
    }

    void Reporter::SetStyle(VisualStyle style)
    {
        m_style = style;

        if (m_channel == Channel::Output && GetConsoleWidth().has_value())
        {
            auto sixelSupported = [&]() { return SixelsSupported(); };
            m_spinner = IIndefiniteSpinner::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), style, sixelSupported);
            m_progressBar = IProgressBar::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), style, sixelSupported);
        }

        if (style == VisualStyle::NoVT)
        {
            m_out->SetVTEnabled(false);
        }
    }

    std::istream& Reporter::RawInputStream()
    {
        return m_in;
    }

    bool Reporter::InputStreamIsInteractive() const
    {
        AICLI_LOG(CLI, Verbose, << "Reporter::m_inStreamFileType is " << m_inStreamFileType);
        return m_inStreamFileType == FILE_TYPE_CHAR;
    }

    bool Reporter::PromptForBoolResponse(Resource::LocString message, Level level, bool resultIfDisabled)
    {
        auto out = GetOutputStream(level);

        if (!out.IsEnabled())
        {
            return resultIfDisabled;
        }

        const std::vector<BoolPromptOption> options
        {
            BoolPromptOption{ Resource::String::PromptOptionYes, 'Y', true },
            BoolPromptOption{ Resource::String::PromptOptionNo, 'N', false },
        };

        out << message << std::endl;

        // Try prompting until we get a recognized option
        for (;;)
        {
            // Output all options
            for (size_t i = 0; i < options.size(); ++i)
            {
                out << PromptEmphasis << "[" + options[i].Hotkey.get() + "] " + options[i].Label.get();

                if (i + 1 == options.size())
                {
                    out << PromptEmphasis << ": ";
                }
                else
                {
                    out << "  ";
                }
            }

            // Read the response
            std::string response;
            if (!std::getline(m_in, response))
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
            }

            // Find the matching option ignoring whitespace
            Utility::Trim(response);
            for (const auto& option : options)
            {
                if (Utility::CaseInsensitiveEquals(response, option.Label) ||
                    Utility::CaseInsensitiveEquals(response, option.Hotkey))
                {
                    return option.Value;
                }
            }
        }
    }

    void Reporter::PromptForEnter(Level level)
    {
        auto out = GetOutputStream(level);
        if (!out.IsEnabled())
        {
            return;
        }

        out << std::endl << Resource::String::PressEnterToContinue << std::endl;
        m_in.get();
    }

    std::filesystem::path Reporter::PromptForPath(Resource::LocString message, Level level, std::filesystem::path resultIfDisabled)
    {
        auto out = GetOutputStream(level);

        if (!out.IsEnabled())
        {
            return resultIfDisabled;
        }

        // Try prompting until we get a valid answer
        for (;;)
        {
            out << message << ' ';

            // Read the response
            std::string response;
            if (!std::getline(m_in, response))
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
            }

            // Validate the path
            std::filesystem::path path{ response };
            if (path.is_absolute())
            {
                return path;
            }
        }

    }

    void Reporter::ShowIndefiniteProgress(bool running)
    {
        if (m_spinner)
        {
            if (running)
            {
                m_spinner->ShowSpinner();
            }
            else
            {
                m_spinner->StopSpinner();
            }
        }
    }

    void Reporter::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        ShowIndefiniteProgress(false);
        if (m_progressBar)
        {
            m_progressBar->ShowProgress(current, maximum, type);
        }
    }

    void Reporter::SetProgressMessage(std::string_view message)
    {
        if (m_spinner)
        {
            m_spinner->SetMessage(message);
        }
    }

    void Reporter::BeginProgress()
    {
        GetBasicOutputStream() << VirtualTerminal::Cursor::Visibility::DisableShow;
        ShowIndefiniteProgress(true);
    };

    void Reporter::EndProgress(bool hideProgressWhenDone)
    {
        ShowIndefiniteProgress(false);
        if (m_progressBar)
        {
            m_progressBar->EndProgress(hideProgressWhenDone);
        }
        SetProgressMessage({});
        GetBasicOutputStream() << VirtualTerminal::Cursor::Visibility::EnableShow;
    };

    Reporter::AsyncProgressScope::AsyncProgressScope(Reporter& reporter, IProgressSink* sink, bool hideProgressWhenDone) :
        m_reporter(reporter), m_callback(sink)
    {
        reporter.SetProgressCallback(&m_callback);
        sink->BeginProgress();
        m_hideProgressWhenDone = hideProgressWhenDone;
    }

    Reporter::AsyncProgressScope::~AsyncProgressScope()
    {
        m_reporter.get().SetProgressCallback(nullptr);
        m_callback.GetSink()->EndProgress(m_hideProgressWhenDone);
    }

    ProgressCallback& Reporter::AsyncProgressScope::Callback()
    {
        return m_callback;
    }

    IProgressCallback* Reporter::AsyncProgressScope::operator->()
    {
        return &m_callback;
    }

    bool Reporter::AsyncProgressScope::HideProgressWhenDone() const
    {
        return m_hideProgressWhenDone;
    }

    void Reporter::AsyncProgressScope::HideProgressWhenDone(bool value)
    {
        m_hideProgressWhenDone.store(value);
    }

    std::unique_ptr<Reporter::AsyncProgressScope> Reporter::BeginAsyncProgress(bool hideProgressWhenDone)
    {
        return std::make_unique<AsyncProgressScope>(*this, m_progressSink.load(), hideProgressWhenDone);
    }

    void Reporter::SetProgressCallback(ProgressCallback* callback)
    {
        auto lock = m_progressCallbackLock.lock_exclusive();
        // Attempting two progress operations at the same time; not supported.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_progressCallback != nullptr && callback != nullptr);
        m_progressCallback = callback;
    }

    void Reporter::CancelInProgressTask(bool force, CancelReason reason)
    {
        // TODO: Maybe ask the user if they really want to cancel?
        UNREFERENCED_PARAMETER(force);
        auto lock = m_progressCallbackLock.lock_shared();
        ProgressCallback* callback = m_progressCallback.load();
        if (callback)
        {
            if (!callback->IsCancelledBy(CancelReason::Any))
            {
                callback->SetProgressMessage(Resource::String::CancellingOperation());
                callback->Cancel(reason);
            }
        }
    }

    void Reporter::CloseOutputStream(bool forceDisable)
    {
        if (forceDisable)
        {
            m_out->Disable();
        }
        m_out->RestoreDefault();
    }

    void Reporter::SetLevelMask(Level reporterLevel, bool setEnabled) {

        if (setEnabled)
        {
            WI_SetAllFlags(m_enabledLevels, reporterLevel);
        }
        else
        {
            WI_ClearAllFlags(m_enabledLevels, reporterLevel);
        }
    }

    bool Reporter::SixelsSupported()
    {
        auto attributes = GetPrimaryDeviceAttributes();
        return (attributes ? attributes->Supports(PrimaryDeviceAttributes::Extension::Sixel) : false);
    }

    bool Reporter::SixelsEnabled()
    {
        return Settings::User().Get<Settings::Setting::EnableSixelDisplay>() && SixelsSupported();
    }
}
