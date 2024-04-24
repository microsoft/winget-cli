// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <sfsclient/SFSClient.h>

#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <time.h>
#include <vector>

#include <nlohmann/json.hpp>

using namespace SFS;
using json = nlohmann::json;

namespace
{
void DisplayUsage()
{
    std::cout
        << "Usage: SFSClientTool --product <identifier> --accountId <id> [options]" << std::endl
        << std::endl
        << "Required:" << std::endl
        << "  --product <identifier>\tName or GUID of the product to be retrieved" << std::endl
        << "  --accountId <id>\t\tAccount ID of the SFS service, used to identify the caller" << std::endl
        << std::endl
        << "Options:" << std::endl
        << "  -h, --help\t\t\tDisplay this help message" << std::endl
        << "  -v, --version\t\t\tDisplay the library version" << std::endl
        << "  --isApp\t\tIndicates the specific product is an App" << std::endl
        << "  --instanceId <id>\t\tA custom SFS instance ID" << std::endl
        << "  --namespace <ns>\t\tA custom SFS namespace" << std::endl
        << "  --customUrl <url>\t\tA custom URL for the SFS service. Library must have been built with SFS_ENABLE_OVERRIDES"
        << std::endl
        << std::endl
        << "Example:" << std::endl
        << "  SFSClientTool --product msedge-stable-win-x64 --accountId msedge" << std::endl;
}

void DisplayHelp()
{
    std::cout << "SFSClient Tool" << std::endl
              << "Copyright (c) Microsoft Corporation. All rights reserved." << std::endl
              << std::endl
              << "Use to interact with the SFS service through the SFS Client library." << std::endl
              << std::endl;
    DisplayUsage();
}

void DisplayVersion()
{
    std::cout << "SFSClient Tool " << SFSClient::GetVersion() << std::endl;
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

bool AreEqualI(std::string_view a, std::string_view b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

struct Settings
{
    bool displayHelp{true};
    bool displayVersion{false};
    bool isApp{false};
    std::string product;
    std::string accountId;
    std::string instanceId;
    std::string nameSpace;
    std::string customUrl;
};

void ParseArguments(const std::vector<std::string_view>& args, Settings& settings)
{
    settings = {};
    settings.displayHelp = args.size() == 1;

    const size_t argsSize = args.size();
    auto validateArg =
        [&argsSize](const size_t index, const std::string& switchName, const std::string_view& argValue) -> void {
        if (argsSize <= index + 1)
        {
            throw std::runtime_error("Missing argument of --" + switchName);
        }
        if (!argValue.empty())
        {
            throw std::runtime_error("--" + switchName + " can only be specified once");
        }
    };

    auto matchArg =
        [](std::string_view arg, const std::string& shortSwitchName, const std::string& longSwitchName) -> bool {
        return (!shortSwitchName.empty() && AreEqualI(arg, shortSwitchName)) || AreEqualI(arg, longSwitchName);
    };

    auto matchLongArg = [](std::string_view arg, const std::string& longSwitchName) -> bool {
        return AreEqualI(arg, longSwitchName);
    };

    for (size_t i = 1; i < args.size(); ++i)
    {
        if (matchArg(args[i], "-h", "--help"))
        {
            settings.displayHelp = true;
        }
        else if (matchArg(args[i], "-v", "--version"))
        {
            settings.displayVersion = true;
        }
        else if (matchLongArg(args[i], "--isApp"))
        {
            settings.isApp = true;
        }
        else if (matchLongArg(args[i], "--product"))
        {
            validateArg(i, "product", settings.product);
            settings.product = args[++i];
        }
        else if (matchLongArg(args[i], "--accountId"))
        {
            validateArg(i, "accountId", settings.accountId);
            settings.accountId = args[++i];
        }
        else if (matchLongArg(args[i], "--instanceId"))
        {
            validateArg(i, "instanceId", settings.instanceId);
            settings.instanceId = args[++i];
        }
        else if (matchLongArg(args[i], "--namespace"))
        {
            validateArg(i, "namespace", settings.nameSpace);
            settings.nameSpace = args[++i];
        }
        else if (matchLongArg(args[i], "--customUrl"))
        {
            validateArg(i, "customUrl", settings.customUrl);
            settings.customUrl = args[++i];
        }
        else
        {
            throw std::runtime_error("Unknown option " + std::string(args[i]));
        }
    }

    if (!settings.displayHelp && (settings.product.empty() || settings.accountId.empty()))
    {
        throw std::runtime_error("--product and --accountId are required and cannot be empty");
    }
}

constexpr std::string_view ToString(HashType type)
{
    switch (type)
    {
    case HashType::Sha1:
        return "Sha1";
    case HashType::Sha256:
        return "Sha256";
    }
    return "";
}

constexpr std::string_view ToString(Architecture type)
{
    switch (type)
    {
    case Architecture::None:
        return "None";
    case Architecture::Amd64:
        return "amd64";
    case Architecture::Arm:
        return "arm";
    case Architecture::Arm64:
        return "arm64";
    case Architecture::x86:
        return "x86";
    }
    return "";
}

void DisplayResults(const std::vector<Content>& contents)
{
    if (contents.empty())
    {
        PrintError("No results found");
        return;
    }

    PrintLog("Content found:");

    json out = json::array();
    for (const auto& content : contents)
    {
        json j = json::object();
        j["ContentId"]["Namespace"] = content.GetContentId().GetNameSpace();
        j["ContentId"]["Name"] = content.GetContentId().GetName();
        j["ContentId"]["Version"] = content.GetContentId().GetVersion();

        j["Files"] = json::array();
        for (const auto& file : content.GetFiles())
        {
            json fileJson = json::object();
            fileJson["FileId"] = file.GetFileId();
            fileJson["Url"] = file.GetUrl();
            fileJson["SizeInBytes"] = file.GetSizeInBytes();
            json hashes = json::object();
            for (const auto& hash : file.GetHashes())
            {
                hashes[ToString(hash.first)] = hash.second;
            }
            fileJson["Hashes"] = hashes;
            j["Files"].push_back(fileJson);
        }
        out.push_back(j);
    }

    PrintLog(out.dump(2 /*indent*/));
}

json AppFileToJson(const AppFile& file)
{
    json fileJson = json::object();
    fileJson["FileId"] = file.GetFileId();
    fileJson["Url"] = file.GetUrl();
    fileJson["SizeInBytes"] = file.GetSizeInBytes();
    fileJson["FileMoniker"] = file.GetFileMoniker();
    json hashes = json::object();
    for (const auto& hash : file.GetHashes())
    {
        hashes[ToString(hash.first)] = hash.second;
    }
    fileJson["Hashes"] = hashes;

    fileJson["ApplicabilityDetails"] = json::object();
    fileJson["ApplicabilityDetails"]["Architectures"] = json::array();
    for (const auto& arch : file.GetApplicabilityDetails().GetArchitectures())
    {
        fileJson["ApplicabilityDetails"]["Architectures"].push_back(ToString(arch));
    }
    fileJson["ApplicabilityDetails"]["PlatformApplicabilityForPackage"] = json::array();
    for (const auto& app : file.GetApplicabilityDetails().GetPlatformApplicabilityForPackage())
    {
        fileJson["ApplicabilityDetails"]["PlatformApplicabilityForPackage"].push_back(app);
    }

    return fileJson;
}

void DisplayResults(const std::vector<AppContent>& contents)
{
    if (contents.empty())
    {
        std::cout << "No results found." << std::endl;
        return;
    }

    PrintLog("Content found:");

    json out = json::array();
    for (const auto& content : contents)
    {
        json j = json::object();
        j["ContentId"]["Namespace"] = content.GetContentId().GetNameSpace();
        j["ContentId"]["Name"] = content.GetContentId().GetName();
        j["ContentId"]["Version"] = content.GetContentId().GetVersion();
        j["UpdateId"] = content.GetUpdateId();

        j["Files"] = json::array();
        for (const auto& file : content.GetFiles())
        {
            j["Files"].push_back(AppFileToJson(file));
        }

        j["Prerequisites"] = json::array();
        for (const auto& prereq : content.GetPrerequisites())
        {
            json prereqJson = json::object();
            prereqJson["ContentId"]["Namespace"] = prereq.GetContentId().GetNameSpace();
            prereqJson["ContentId"]["Name"] = prereq.GetContentId().GetName();
            prereqJson["ContentId"]["Version"] = prereq.GetContentId().GetVersion();

            prereqJson["Files"] = json::array();
            for (const auto& file : prereq.GetFiles())
            {
                prereqJson["Files"].push_back(AppFileToJson(file));
            }

            j["Prerequisites"].push_back(prereqJson);
        }
        out.push_back(j);
    }

    PrintLog(out.dump(2 /*indent*/));
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
              << "]" << " " << std::filesystem::path(logData.file).filename().string() << ":" << logData.line << " "
              << logData.message << c_colorEnd << std::endl;
}

bool SetEnv(const std::string& varName, const std::string& value)
{
    if (varName.empty() || value.empty())
    {
        return false;
    }
#ifdef _WIN32
    return _putenv_s(varName.c_str(), value.c_str()) == 0;
#else
    return setenv(varName.c_str(), value.c_str(), 1 /*overwrite*/) == 0;
#endif
}

Result GetLatestDownloadInfo(const SFSClient& sfsClient, const Settings& settings)
{
    PrintLog("Getting latest download info for product: " + settings.product);
    RequestParams params;
    params.productRequests = {{settings.product, {}}};
    if (settings.isApp)
    {
        std::vector<AppContent> appContents;
        auto result = sfsClient.GetLatestAppDownloadInfo(params, appContents);
        if (!result)
        {
            PrintError("Failed to get latest download info for app.");
            LogResult(result);
            return result.GetCode();
        }

        // Display results
        DisplayResults(appContents);
    }
    else
    {
        std::vector<Content> contents;
        auto result = sfsClient.GetLatestDownloadInfo(params, contents);

        if (!result)
        {
            PrintError("Failed to get latest download info.");
            LogResult(result);
            return result.GetCode();
        }

        // Display results
        DisplayResults(contents);
    }

    return Result::Success;
}
} // namespace

int main(int argc, char* argv[])
{
    Settings settings;
    try
    {
        ParseArguments(std::vector<std::string_view>(argv, argv + argc), settings);
    }
    catch (const std::runtime_error& e)
    {
        PrintError(std::string(e.what()) + "\n");
        DisplayUsage();
        return 1;
    }

    if (settings.displayVersion)
    {
        DisplayVersion();
        return 0;
    }

    if (settings.displayHelp)
    {
        DisplayHelp();
        return 0;
    }

    if (!settings.customUrl.empty())
    {
        SetEnv("SFS_TEST_OVERRIDE_BASE_URL", settings.customUrl);
        PrintLog("Using custom URL: " + settings.customUrl);
        PrintLog("Note that the library must have been built with SFS_ENABLE_OVERRIDES to use a custom URL.");
    }

    // Initialize SFSClient
    PrintLog("Initializing SFSClient with accountId: " + settings.accountId +
             (settings.instanceId.empty() ? "" : ", instanceId: " + settings.instanceId) +
             (settings.nameSpace.empty() ? "" : ", namespace: " + settings.nameSpace));

    ClientConfig config;
    config.accountId = settings.accountId;
    if (!settings.instanceId.empty())
    {
        config.instanceId = settings.instanceId;
    }
    if (!settings.nameSpace.empty())
    {
        config.nameSpace = settings.nameSpace;
    }
    config.logCallbackFn = LoggingCallback;

    std::unique_ptr<SFSClient> sfsClient;
    auto result = SFSClient::Make(config, sfsClient);
    if (!result)
    {
        PrintError("Failed to initialize SFSClient.");
        LogResult(result);
        return result.GetCode();
    }

    // Perform operations using SFSClient
    result = GetLatestDownloadInfo(*sfsClient, settings);
    if (!result)
    {
        LogResult(result);
        return result.GetCode();
    }

    return 0;
}
