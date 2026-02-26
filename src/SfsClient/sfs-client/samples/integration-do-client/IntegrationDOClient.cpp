// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <do_download.h>
#include <sfsclient/SFSClient.h>

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <time.h>
#include <vector>

#ifdef _WIN32
#include <combaseapi.h>
#endif

namespace
{
void DisplayUsage()
{
    std::cout << "Usage: ExampleIntegrationDOClient --product <identifier> [options]" << std::endl
              << std::endl
              << "Required:" << std::endl
              << "  --product <identifier>\t\tName or GUID of the product to be retrieved" << std::endl
              << std::endl
              << "Options:" << std::endl
              << "  -h, --help\t\t\tDisplay this help message" << std::endl
              << "  --accountId <id>\t\tAccount ID of the SFS service, used to identify the caller" << std::endl
              << "  --outDir <id>\t\tLocation where download will be placed. Defaults to a tmp dir" << std::endl
              << std::endl
              << std::endl
              << "Example:" << std::endl
              << "  ExampleIntegrationDOClient --product \"msedge-stable-win-x64\" --accountId msedge" << std::endl;
}

void DisplayHelp()
{
    std::cout
        << "ExampleIntegrationDOClient" << std::endl
        << "Copyright (c) Microsoft Corporation. All rights reserved." << std::endl
        << std::endl
        << "Use to interact with the SFS service through the SFS Client library and download using the Delivery Optimization SDK."
        << std::endl
        << std::endl;
    DisplayUsage();
}

const std::string c_boldRedStart = "\033[1;31m";
const std::string c_cyanStart = "\033[0;36m";
const std::string c_darkGreyStart = "\033[0;90m";
const std::string c_colorEnd = "\033[0m";

void PrintError(std::string_view message)
{
    std::cout << c_boldRedStart << message << c_colorEnd << std::endl;
}

void PrintLog(std::string_view message)
{
    std::cout << c_cyanStart << message << c_colorEnd << std::endl;
}

struct Settings
{
    bool displayHelp{true};
    std::string product;
    std::string accountId;
    std::string outDir;
};

int ParseArguments(const std::vector<std::string_view>& args, Settings& settings)
{
    settings = {};
    settings.displayHelp = args.size() == 1;

    const size_t argsSize = args.size();
    auto validateArg =
        [&argsSize](const size_t index, const std::string& switchName, const std::string_view& argValue) -> bool {
        if (argsSize <= index + 1)
        {
            PrintError("Missing argument of --" + switchName + ".");
            return false;
        }
        if (!argValue.empty())
        {
            PrintError("--" + switchName + " can only be specified once.");
            return false;
        }
        return true;
    };

    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i].compare("-h") == 0 || args[i].compare("--help") == 0)
        {
            settings.displayHelp = true;
        }
        else if (args[i].compare("--product") == 0)
        {
            if (!validateArg(i, "product", settings.product))
            {
                return 1;
            }
            settings.product = args[++i];
        }
        else if (args[i].compare("--accountId") == 0)
        {
            if (!validateArg(i, "accountId", settings.accountId))
            {
                return 1;
            }
            settings.accountId = args[++i];
        }
        else if (args[i].compare("--outDir") == 0)
        {
            if (!validateArg(i, "outDir", settings.outDir))
            {
                return 1;
            }
            settings.outDir = args[++i];
        }
        else
        {
            PrintError("Unknown option " + std::string(args[i]) + ".\n");
            return 1;
        }
    }

    if (settings.product.empty())
    {
        return 1;
    }

    return 0;
}

void LogResult(const SFS::Result& result)
{
    std::cout << "  Result code: " << ToString(result.GetCode());
    if (!result.GetMsg().empty())
    {
        std::cout << ". Message: " << result.GetMsg();
    }
    std::cout << std::endl;
}

std::string TimestampToString(std::chrono::time_point<std::chrono::system_clock> time)
{
    using namespace std::chrono;

    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(time.time_since_epoch()) % 1000;

    auto timer = system_clock::to_time_t(time);

    std::stringstream timeStream;
    struct tm gmTime;
#ifdef _WIN32
    gmtime_s(&gmTime, &timer); // gmtime_s is the safe version of gmtime, not available on Linux
#else
    gmTime = (*std::gmtime(&timer));
#endif
    timeStream << std::put_time(&gmTime, "%F %X"); // yyyy-mm-dd HH:MM:SS
    timeStream << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return timeStream.str();
}

void LoggingCallback(const SFS::LogData& logData)
{
    std::cout << c_darkGreyStart << "Log: " << TimestampToString(logData.time) << " [" << ToString(logData.severity)
              << "]"
              << " " << std::filesystem::path(logData.file).filename().string() << ":" << logData.line << " "
              << logData.message << c_colorEnd << std::endl;
}

std::filesystem::path GetOutDir(const std::string& outDir)
{
    if (outDir.empty())
    {
        auto tmpDir = std::filesystem::temp_directory_path();
        auto outDir = tmpDir / "SFSDownload";
        std::filesystem::create_directories(outDir);

        // Set write permissions to the download dir since the download happens from a different process, but only in
        // non-Windows
#ifndef _WIN32
        std::filesystem::permissions(outDir,
                                     std::filesystem::perms::owner_all | std::filesystem::perms::group_all |
                                         std::filesystem::perms::others_all,
                                     std::filesystem::perm_options::add);
#endif
        return outDir.string();
    }
    return outDir;
}

std::vector<SFS::Content> GetLatestDownloadInfo(const Settings& settings)
{
    // Initialize SFSClient
    PrintLog("Initializing SFSClient with accountId: " + settings.accountId);

    SFS::ClientConfig config;
    config.accountId = settings.accountId;
    config.logCallbackFn = LoggingCallback;

    std::unique_ptr<SFS::SFSClient> sfsClient;
    auto result = SFS::SFSClient::Make(config, sfsClient);
    if (!result)
    {
        PrintError("Failed to initialize SFSClient.");
        LogResult(result);
        return {};
    }

    // Perform operations using SFSClient
    PrintLog("Getting latest download info for product: " + settings.product);
    std::vector<SFS::Content> contents;
    SFS::RequestParams params;
    params.productRequests = {{settings.product, {}}};
    result = sfsClient->GetLatestDownloadInfo(params, contents);
    if (!result)
    {
        PrintError("Failed to get latest download info.");
        LogResult(result);
        return {};
    }

    return contents;
}

#ifdef _WIN32
class CoInitializeWrapper
{
  public:
    CoInitializeWrapper()
    {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }

    ~CoInitializeWrapper()
    {
        CoUninitialize();
    }
};
#endif

std::string ErrorCodeToHexString(std::error_code error)
{
    std::stringstream stream;
    stream << "0x" << std::hex << error.value();
    return stream.str();
}

int Download(const SFS::Content& content, const std::string& baseOutDir)
{
    PrintLog("Found content: " + content.GetContentId().GetNameSpace() + "/" + content.GetContentId().GetName() + "/" +
             content.GetContentId().GetVersion());

    const auto outDir = GetOutDir(baseOutDir);
    PrintLog("Downloading files to: " + outDir.string());

    for (const auto& file : content.GetFiles())
    {
        PrintLog("Downloading file " + file.GetFileId() + " from " + file.GetUrl());
        auto outFilePath = outDir / file.GetFileId();

        if (std::filesystem::exists(outFilePath))
        {
            // In a real implementation, one would check the file sha1 or sha256 hash to see if the file is the same as
            // the current one
            PrintLog("File already exists, cannot download again. Skipping.");
            continue;
        }

        // Download the file using Delivery Optimization SDK
        std::unique_ptr<microsoft::deliveryoptimization::download> download;
        auto error = microsoft::deliveryoptimization::download::make(file.GetUrl(), outFilePath.string(), download);
        if (error)
        {
            PrintError("Failed to create download object with error " + ErrorCodeToHexString(error));
            return 1;
        }

        error = download->start_and_wait_until_completion();
        if (error)
        {
            PrintError("Failed to download file with error " + ErrorCodeToHexString(error));
            return 1;
        }

        PrintLog("File successfully downloaded");
    }

    return 0;
}
} // namespace

int main(int argc, char* argv[])
{
    Settings settings;
    if (ParseArguments(std::vector<std::string_view>(argv, argv + argc), settings) != 0)
    {
        DisplayUsage();
        return 1;
    }

    if (settings.displayHelp)
    {
        DisplayHelp();
        return 0;
    }

    auto contents = GetLatestDownloadInfo(settings);
    if (contents.size() != 1)
    {
        return 1;
    }

    // In Windows the DO SDK uses COM, so we must call CoInitializeEx before using it
#ifdef _WIN32
    CoInitializeWrapper coInitWrapper;
#endif

    if (Download(contents[0], settings.outDir) != 0)
    {
        return 1;
    }

    return 0;
}
