// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/InstalledFilesCorrelation.h"
#include <winget/FolderFileWatcher.h>
#include <winget/Filesystem.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Correlation
{
    namespace
    {
        constexpr std::string_view s_ShellLinkFileExtension = ".lnk"sv;
        const std::vector<std::pair<std::filesystem::path, std::string>> s_CandidateInstallLocationRoots =
        {
            { Filesystem::GetKnownFolderPath(FOLDERID_LocalAppData), "%LOCALAPPDATA%" },
            { Filesystem::GetKnownFolderPath(FOLDERID_ProgramFiles), "%PROGRAMFILES%" },
            { Filesystem::GetKnownFolderPath(FOLDERID_ProgramFilesX86), "%PROGRAMFILES(X86)%" },
        };

        // Contains shell link info
        struct ShellLinkFileInfo
        {
            std::filesystem::path Path;
            std::string Args;
            std::string DisplayName;
        };

        std::optional<ShellLinkFileInfo> ParseShellLinkFile(const std::filesystem::path& linkFile)
        {
            try
            {
                AICLI_LOG(Repo, Info, << "Parsing link file at " << linkFile);

                ShellLinkFileInfo result;

                Microsoft::WRL::ComPtr<IShellLink> shellLink;
                THROW_IF_FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink)));
                Microsoft::WRL::ComPtr<IPersistFile> persistFile;
                THROW_IF_FAILED(shellLink.As(&persistFile));
                THROW_IF_FAILED(persistFile->Load(linkFile.wstring().c_str(), STGM_READ));
                THROW_IF_FAILED(shellLink->Resolve(nullptr, SLR_NO_UI | SLR_NOUPDATE | SLR_NOSEARCH | SLR_NOTRACK | SLR_NOLINKINFO));

                {
                    // Parse Path from shell link
                    std::wstring buffer;
                    buffer.resize(MAX_PATH);
                    HRESULT hr = S_OK;
                    for (int retry = 0; retry < 5; retry++)
                    {
                        hr = shellLink->GetPath(
                            &buffer[0],
                            static_cast<int>(buffer.size()),
                            nullptr,
                            0
                        );

                        if (SUCCEEDED(hr))
                        {
                            buffer.erase(std::find(buffer.begin(), buffer.end(), L'\0'), buffer.end());
                            result.Path = buffer;
                            break;
                        }
                        else if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                        {
                            buffer.resize(buffer.size() * 2);
                        }
                        else
                        {
                            THROW_IF_FAILED(hr);
                        }
                    }
                }

                {
                    // Parse arguments from shell link
                    std::wstring buffer;
                    buffer.resize(MAX_PATH);
                    HRESULT hr = S_OK;
                    for (int retry = 0; retry < 5; retry++)
                    {
                        hr = shellLink->GetArguments(
                            &buffer[0],
                            static_cast<int>(buffer.size()));

                        if (SUCCEEDED(hr))
                        {
                            buffer.erase(std::find(buffer.begin(), buffer.end(), L'\0'), buffer.end());
                            result.Args = Utility::ConvertToUTF8(buffer);
                            break;
                        }
                        else if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                        {
                            buffer.resize(buffer.size() * 2);
                        }
                        else
                        {
                            THROW_IF_FAILED(hr);
                        }
                    }
                }

                // Use shell link file name (minus extension) as display name.
                result.DisplayName = linkFile.stem().u8string();

                AICLI_LOG(Repo, Info, << "Link file parsed. Path: " << result.Path << " Args: " << result.Args << " DisplayName: " << result.DisplayName);

                return result;
            }
            catch (...)
            {
                AICLI_LOG(Repo, Error, << "Failed to parse link file at " << linkFile);
                return {};
            }
        }

        // Returns nullopt if path is not under base.
        std::optional<std::filesystem::path> GetRelativePath(const std::filesystem::path& path, const std::filesystem::path& base)
        {
            auto canonicalPath = std::filesystem::weakly_canonical(path);
            auto canonicalBase = std::filesystem::weakly_canonical(base);

            auto relativePath = std::filesystem::relative(canonicalPath, canonicalBase);
            if (!relativePath.empty() && *relativePath.begin() != "." && *relativePath.begin() != "..")
            {
                return relativePath;
            }
            else
            {
                return {};
            }
        }

        std::optional<std::filesystem::path> CheckOneInstallLocation(const std::filesystem::path& childFile, const std::filesystem::path& baseFolder)
        {
            auto relativePath = GetRelativePath(childFile, baseFolder);
            if (relativePath)
            {
                // TODO: Here we assume the install location is the top directory of relative path.
                auto installLocation = baseFolder / *relativePath->begin();
                if (std::filesystem::exists(installLocation) && std::filesystem::is_directory(installLocation))
                {
                    return installLocation;
                }
            }

            return {};
        }

        // If install location is not provided in arp entry, try LocalAppData folder and Program Files folders.
        std::optional<std::filesystem::path> CheckInstallLocation(const std::filesystem::path& path)
        {
            for (auto const& entry : s_CandidateInstallLocationRoots)
            {
                auto installLocation = CheckOneInstallLocation(path, entry.first);
                if (installLocation)
                {
                    return installLocation;
                }
            }

            return {};
        }

        // TODO: basic heuristics to determine file type.
        AppInstaller::Manifest::InstalledFileTypeEnum GetInstalledFileType(const ShellLinkFileInfo& linkInfo)
        {
            Manifest::InstalledFileTypeEnum result = Manifest::InstalledFileTypeEnum::Other;

            if (Utility::CaseInsensitiveContainsSubstring(linkInfo.Path.u8string(), "uninstall") ||
                Utility::CaseInsensitiveContainsSubstring(linkInfo.Path.u8string(), "unins000") ||
                Utility::CaseInsensitiveContainsSubstring(linkInfo.Args, "uninstall") ||
                Utility::CaseInsensitiveContainsSubstring(linkInfo.DisplayName, "uninstall"))
            {
                result = Manifest::InstalledFileTypeEnum::Uninstall;
            }
            else if (Utility::CaseInsensitiveEquals(linkInfo.Path.extension().u8string(), ".exe"))
            {
                result = Manifest::InstalledFileTypeEnum::Launch;
            }

            return result;
        }

        std::string GetUnexpandedInstallLocation(const std::filesystem::path& installLocation)
        {
            // Try to match the candidate install location roots first.
            std::filesystem::path resultInstallLocation = installLocation;
            for (auto const& entry : s_CandidateInstallLocationRoots)
            {
                if (Filesystem::ReplaceCommonPathPrefix(resultInstallLocation, entry.first, entry.second))
                {
                    return resultInstallLocation.u8string();
                }
            }

            // Then try PathUnExpandEnvStrings OS api
            std::wstring installLocationWString = installLocation.wstring();
            std::wstring buffer;
            buffer.resize(installLocationWString.size() + 20);
            if (PathUnExpandEnvStrings(
                installLocationWString.c_str(),
                &buffer[0],
                static_cast<int>(buffer.size())))
            {
                buffer.resize(buffer.find(L'\0'));
                return Utility::ConvertToUTF8(buffer);
            }

            return resultInstallLocation.u8string();
        }
    }

    InstalledFilesCorrelation::InstalledFilesCorrelation()
    {
        m_fileWatchers.emplace_back(Filesystem::GetKnownFolderPath(FOLDERID_CommonStartMenu), std::string{ s_ShellLinkFileExtension });
        m_fileWatchers.emplace_back(Filesystem::GetKnownFolderPath(FOLDERID_StartMenu), std::string{ s_ShellLinkFileExtension });
    }

    void InstalledFilesCorrelation::StartFileWatcher()
    {
        m_files.clear();

        for (auto& watcher : m_fileWatchers)
        {
            watcher.Start();
        }
    }

    void InstalledFilesCorrelation::StopFileWatcher()
    {
        for (auto& watcher : m_fileWatchers)
        {
            watcher.Stop();
        }

        for (auto& watcher : m_fileWatchers)
        {
            FileWatcherFiles files;
            files.Folder = watcher.FolderPath();

            for (auto const& file : watcher.Files())
            {
                files.Files.emplace_back(file);
            }

            m_files.emplace_back(std::move(files));
        }
    }

    InstallationMetadata InstalledFilesCorrelation::CorrelateForNewlyInstalled(
        const Manifest::Manifest&,
        const std::string& arpInstallLocation)
    {
        InstallationMetadata result;

        std::filesystem::path installLocation;
        // Use arp install location if provided
        if (!arpInstallLocation.empty())
        {
            installLocation = Filesystem::GetExpandedPath(arpInstallLocation);
        }

        for (auto const& files : m_files)
        {
            for (auto const& file : files.Files)
            {
                // TODO: we only watch shell link files at the moment.
                auto linkInfo = ParseShellLinkFile(files.Folder / file);
                if (linkInfo)
                {
                    auto installedFileType = GetInstalledFileType(linkInfo.value());

                    // Collect installed files metadata if exist
                    if (std::filesystem::exists(linkInfo->Path) && std::filesystem::is_regular_file(linkInfo->Path))
                    {
                        if (installLocation.empty())
                        {
                            // TODO: In most cases, installed files are under same folder, so use the first file to determine install location at the moment.
                            auto location = CheckInstallLocation(linkInfo->Path);
                            if (!location)
                            {
                                continue;
                            }

                            installLocation = location.value();
                        }

                        auto relativePath = GetRelativePath(linkInfo->Path, installLocation);
                        if (relativePath)
                        {
                            AppInstaller::Manifest::InstalledFile fileEntry;
                            fileEntry.RelativeFilePath = relativePath->string();
                            std::ifstream in{ linkInfo->Path, std::ifstream::binary };
                            fileEntry.FileSha256 = Utility::SHA256::ComputeHash(in);
                            fileEntry.InvocationParameter = linkInfo->Args;
                            fileEntry.DisplayName = linkInfo->DisplayName;
                            fileEntry.FileType = installedFileType;
                            result.InstalledFiles.Files.emplace_back(std::move(fileEntry));
                        }
                    }

                    // Collect short cut paths
                    InstalledStartupLinkFile linkFile;
                    linkFile.RelativeFilePath = file.u8string();
                    linkFile.FileType = installedFileType;
                    result.StartupLinkFiles.emplace_back(linkFile);
                }
            }
        }

        if (!installLocation.empty())
        {
            result.InstalledFiles.DefaultInstallLocation = GetUnexpandedInstallLocation(installLocation);
        }

        return result;
    }
}
