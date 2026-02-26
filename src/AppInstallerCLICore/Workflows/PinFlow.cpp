// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"
#include "PinFlow.h"
#include "TableOutput.h"
#include <winget/PinningData.h>
#include <winget/RepositorySearch.h>
#include <winget/PackageVersionSelection.h>

using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Creates a Pin appropriate for the context based on the arguments provided
        Pinning::Pin CreatePin(Execution::Context& context, const Pinning::PinKey& pinKey)
        {
            if (context.Args.Contains(Execution::Args::Type::GatedVersion))
            {
                return Pinning::Pin::CreateGatingPin(pinKey, context.Args.GetArg(Execution::Args::Type::GatedVersion));
            }
            else if (context.Args.Contains(Execution::Args::Type::BlockingPin))
            {
                return Pinning::Pin::CreateBlockingPin(pinKey);
            }
            else
            {
                return Pinning::Pin::CreatePinningPin(pinKey);
            }
        }

        void GetPinKeysForInstalled(const std::shared_ptr<IPackageVersion>& installedVersion, std::set<Pinning::PinKey>& pinKeys)
        {
            auto installedType = Manifest::ConvertToInstallerTypeEnum(installedVersion->GetMetadata()[PackageVersionMetadata::InstalledType]);
            std::vector<Utility::LocIndString> propertyStrings;

            if (Manifest::DoesInstallerTypeUsePackageFamilyName(installedType))
            {
                propertyStrings = installedVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
            }
            else if (Manifest::DoesInstallerTypeUseProductCode(installedType))
            {
                propertyStrings = installedVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            }

            for (const auto& value : propertyStrings)
            {
                pinKeys.emplace(Pinning::PinKey::GetPinKeyForInstalled(value));
            }
        }

        std::set<Pinning::PinKey> GetPinKeysForPackage(Execution::Context& context)
        {
            auto package = context.Get<Execution::Data::Package>();

            std::set<Pinning::PinKey> pinKeys;

            if (context.Args.Contains(Execution::Args::Type::PinInstalled))
            {
                auto installedVersion = GetInstalledVersion(package);
                if (installedVersion)
                {
                    GetPinKeysForInstalled(installedVersion, pinKeys);
                }
            }
            else
            {
                auto availablePackages = package->GetAvailable();
                for (const auto& availablePackage : availablePackages)
                {
                    pinKeys.emplace(
                        availablePackage->GetProperty(PackageProperty::Id).get(),
                        availablePackage->GetSource().GetIdentifier());
                }
            }

            return pinKeys;
        }

        // Gets a search request that can be used to find the installed package that corresponds with a pin.
        SearchRequest GetSearchRequestForPin(const Pinning::PinKey& pinKey)
        {
            SearchRequest searchRequest;
            if (pinKey.IsForInstalled())
            {
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::PackageFamilyName, MatchType::Exact, pinKey.PackageId));
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, pinKey.PackageId));
            }
            else
            {
                searchRequest.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, pinKey.PackageId);
            }

            return searchRequest;
        }
    }

    void OpenPinningIndex::operator()(Execution::Context& context) const
    {
        auto pinningData = Pinning::PinningData{ m_readOnly ? Pinning::PinningData::Disposition::ReadOnly : Pinning::PinningData::Disposition::ReadWrite };
        if (!m_readOnly && !pinningData)
        {
            AICLI_LOG(CLI, Error, << "Unable to open pinning index.");
            context.Reporter.Error() << Resource::String::PinCannotOpenIndex << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CANNOT_OPEN_PINNING_INDEX);
        }

        context.Add<Execution::Data::PinningData>(std::move(pinningData));
    }

    void GetAllPins(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "Getting all existing pins");
        context.Add<Execution::Data::Pins>(context.Get<Execution::Data::PinningData>().GetAllPins());
    }

    void SearchPin(Execution::Context& context)
    {
        auto pinKeys = GetPinKeysForPackage(context);

        auto package = context.Get<Execution::Data::Package>();
        auto pinningData = context.Get<Execution::Data::PinningData>();

        std::vector<Pinning::Pin> pins;
        for (const auto& pinKey : pinKeys)
        {
            auto pin = pinningData.GetPin(pinKey);
            if (pin)
            {
                pins.emplace_back(std::move(pin.value()));
            }
        }

        context.Add<Execution::Data::Pins>(std::move(pins));
    }

    void AddPin(Execution::Context& context)
    {
        auto pinKeys = GetPinKeysForPackage(context);

        auto package = context.Get<Execution::Data::Package>();
        auto pinningData = context.Get<Execution::Data::PinningData>();
        auto installedVersion = context.Get<Execution::Data::InstalledPackageVersion>();

        std::vector<Pinning::Pin> pinsToAddOrUpdate;
        for (const auto& pinKey : pinKeys)
        {
            auto pin = CreatePin(context, pinKey);
            AICLI_LOG(CLI, Info, << "Evaluating Pin " << pin.ToString());

            auto existingPin = pinningData.GetPin(pinKey);
            if (existingPin)
            {
                Utility::LocIndString packageNameToReport;
                if (pinKey.IsForInstalled() && installedVersion)
                {
                    packageNameToReport = installedVersion->GetProperty(PackageVersionProperty::Name);
                }
                else
                {
                    auto availableVersion = GetAvailablePackageFromSource(package, pinKey.SourceId)->GetLatestVersion();
                    if (availableVersion)
                    {
                        packageNameToReport = availableVersion->GetProperty(PackageVersionProperty::Name);
                    }
                }

                // Pin already exists.
                // If it is the same, we do nothing. If it is different, check for the --force arg
                if (pin == existingPin)
                {
                    AICLI_LOG(CLI, Info, << "Pin already exists");
                    context.Reporter.Info() << Resource::String::PinAlreadyExists(packageNameToReport) << std::endl;
                    continue;
                }

                AICLI_LOG(CLI, Info, << "Another pin already exists for the package for source " << pinKey.SourceId);
                if (context.Args.Contains(Execution::Args::Type::Force))
                {
                    AICLI_LOG(CLI, Info, << "Overwriting pin due to --force argument");
                    context.Reporter.Warn() << Resource::String::PinExistsOverwriting(packageNameToReport) << std::endl;
                    pinsToAddOrUpdate.push_back(std::move(pin));
                }
                else
                {
                    context.Reporter.Error() << Resource::String::PinExistsUseForceArg(packageNameToReport) << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS);
                }
            }
            else
            {
                pinsToAddOrUpdate.push_back(std::move(pin));
            }
        }

        if (!pinsToAddOrUpdate.empty())
        {
            for (const auto& pin : pinsToAddOrUpdate)

            {
                pinningData.AddOrUpdatePin(pin);
            }

            context.Reporter.Info() << Resource::String::PinAdded << std::endl;
        }
    }

    void RemovePin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        auto pins = context.Get<Execution::Data::Pins>();

        auto pinningData = context.Get<Execution::Data::PinningData>();
        bool pinExists = false;

        // Note that if a source was specified in the command line,
        // that will be the only one we get version keys from.
        // So, we remove pins from all sources unless one was provided.
        for (const auto& pin : pins)
        {
            AICLI_LOG(CLI, Info, << "Removing Pin " << pin.GetKey().ToString());
            pinningData.RemovePin(pin.GetKey());
            pinExists = true;
        }

        if (!pinExists)
        {
            AICLI_LOG(CLI, Warning, << "Pin does not exist");
            context.Reporter.Warn() << Resource::String::PinDoesNotExist(package->GetProperty(PackageProperty::Name)) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
        }

        context.Reporter.Info() << Resource::String::PinRemovedSuccessfully << std::endl;
    }

    void ReportPins(Execution::Context& context)
    {
        const auto& pins = context.Get<Execution::Data::Pins>();
        if (pins.empty())
        {
            context.Reporter.Info() << Resource::String::PinNoPinsExist << std::endl;
            return;
        }

        Execution::TableOutput<6> table(context.Reporter,
            {
                Resource::String::SearchName,
                Resource::String::SearchId,
                Resource::String::SearchVersion,
                Resource::String::SearchSource,
                Resource::String::PinType,
                Resource::String::PinVersion,
            });

        const auto& source = context.Get<Execution::Data::Source>();
        for (const auto& pin : pins)
        {
            const auto& pinKey = pin.GetKey();
            auto searchRequest = GetSearchRequestForPin(pin.GetKey());
            auto searchResult = source.Search(searchRequest);
            for (const auto& match : searchResult.Matches)
            {
                Utility::LocIndString packageName;
                Utility::LocIndString sourceName;
                Utility::LocIndString version;

                if (pinKey.IsForInstalled())
                {
                    sourceName = Resource::LocString{ Resource::String::PinInstalledSource };
                }
                else
                {
                    // This ensures we get the info from the right source if it exists on multiple
                    auto availablePackage = GetAvailablePackageFromSource(match.Package, pinKey.SourceId);
                    if (availablePackage)
                    {
                        auto availableVersion = availablePackage->GetLatestVersion();
                        if (availableVersion)
                        {
                            packageName = availableVersion->GetProperty(PackageVersionProperty::Name);
                            sourceName = availableVersion->GetProperty(PackageVersionProperty::SourceName);
                        }
                    }
                }

                auto installedVersion = GetInstalledVersion(match.Package);
                if (installedVersion)
                {
                    packageName = installedVersion->GetProperty(PackageVersionProperty::Name);
                    version = installedVersion->GetProperty(PackageVersionProperty::Version);
                }

                table.OutputLine({
                    packageName,
                    pinKey.PackageId,
                    version,
                    sourceName,
                    std::string{ ToString(pin.GetType()) },
                    pin.GetGatedVersion().ToString(),
                });
            }
        }

        table.Complete();
    }

    void ResetAllPins(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "Resetting all pins");
        context.Reporter.Info() << Resource::String::PinResettingAll << std::endl;

        std::string sourceId;
        if (context.Args.Contains(Execution::Args::Type::Source))
        {
            auto sourceName = context.Args.GetArg(Execution::Args::Type::Source);
            auto sources = Source::GetCurrentSources();
            for (const auto& source : sources)
            {
                if (Utility::CaseInsensitiveEquals(source.Name, sourceName))
                {
                    sourceId = source.Identifier;
                    break;
                }
            }
        }

        if (context.Get<Execution::Data::PinningData>().ResetAllPins(sourceId))
        {
            context.Reporter.Info() << Resource::String::PinResetSuccessful << std::endl;
        }
        else
        {
            context.Reporter.Info() << Resource::String::PinNoPinsExist << std::endl;
        }
    }
}