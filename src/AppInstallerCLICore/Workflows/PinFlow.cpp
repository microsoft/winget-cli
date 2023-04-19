// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"
#include "PinFlow.h"
#include "TableOutput.h"
#include "Microsoft/PinningIndex.h"
#include "Microsoft/SQLiteStorageBase.h"
#include "winget/RepositorySearch.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;

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

        // Gets a search request that can be used to find the installed package
        // that corresponds with a pin. The search is based on the extra ID
        // (Product Code or Package Family Name) if it is available; otherwise
        // it uses the package ID. Using the extra ID allows us to disambiguate
        // in certain cases.
        SearchRequest GetSearchRequestForPin(const Pinning::PinKey& pinKey)
        {
            SearchRequest searchRequest;

            switch (pinKey.ExtraIdType)
            {
            case Pinning::ExtraIdStringType::ProductCode:
                searchRequest.Filters.emplace_back(PackageMatchField::ProductCode, MatchType::CaseInsensitive, pinKey.ExtraId);
                break;
            case Pinning::ExtraIdStringType::PackageFamilyName:
                searchRequest.Filters.emplace_back(PackageMatchField::PackageFamilyName, MatchType::CaseInsensitive, pinKey.ExtraId);
                break;
            case Pinning::ExtraIdStringType::None:
            default:
                searchRequest.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, pinKey.PackageId);
                break;
            }

            return searchRequest;
        }
    }

    void OpenPinningIndex::operator()(Execution::Context& context) const
    {
        auto openDisposition = m_readOnly ? SQLiteStorageBase::OpenDisposition::Read : SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto pinningIndex = PinningIndex::OpenOrCreateDefault(openDisposition);
        if (!pinningIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open pinning index.");
            context.Reporter.Error() << Resource::String::PinCannotOpenIndex << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CANNOT_OPEN_PINNING_INDEX);
        }

        context.Add<Execution::Data::PinningIndex>(std::move(pinningIndex));
    }

    void GetAllPins(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "Getting all existing pins");
        context.Add<Execution::Data::Pins>(context.Get<Execution::Data::PinningIndex>()->GetAllPins());
    }

    void SearchPin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        std::vector<Pinning::Pin> pins;
        std::set<std::pair<std::string, std::string>> sourcesAndIds;

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();

        auto packageVersionKeys = package->GetAvailableVersionKeys();

        auto installedVersion = package->GetInstalledVersion();
        auto installedProductCodes = installedVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
        auto installedPackageFamilyNames = installedVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);

        for (const auto& versionKey : packageVersionKeys)
        {
            auto availableVersion = package->GetAvailableVersion(versionKey);
            const auto availablePackageId = availableVersion->GetProperty(PackageVersionProperty::Id).get();
            const auto availableSourceId = availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get();
            const auto pinKeys = Pinning::GetPinKeysForAvailablePackage(availablePackageId, availableSourceId, installedProductCodes, installedPackageFamilyNames);

            for (const auto& pinKey : pinKeys)
            {
                if (sourcesAndIds.emplace(pinKey.SourceId, pinKey.ExtraId).second)
                {
                    auto pin = pinningIndex->GetPin(pinKey);
                    if (pin)
                    {
                        pins.emplace_back(std::move(pin.value()));
                    }
                }
            }
        }

        context.Add<Execution::Data::Pins>(std::move(pins));
    }

    void AddPin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedVersion = context.Get<Execution::Data::InstalledPackageVersion>();

        auto installedVersionString = installedVersion->GetProperty(PackageVersionProperty::Version);
        auto installedProductCodes = installedVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
        auto installedPackageFamilyNames = installedVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);

        std::vector<Pinning::Pin> pinsToAddOrUpdate;
        std::set<std::pair<std::string, std::string>> sourcesAndIds;

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();

        auto packageVersionKeys = package->GetAvailableVersionKeys();
        for (const auto& versionKey : packageVersionKeys)
        {
            auto availableVersion = package->GetAvailableVersion(versionKey);
            const auto availablePackageId = availableVersion->GetProperty(PackageVersionProperty::Id).get();
            const auto availableSourceId = availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get();
            const auto pinKeys = Pinning::GetPinKeysForAvailablePackage(availablePackageId, availableSourceId, installedProductCodes, installedPackageFamilyNames);

            for (const auto& pinKey : pinKeys)
            {
                if (!sourcesAndIds.emplace(pinKey.SourceId, pinKey.ExtraId).second)
                {
                    // We already considered the pin for this source and product code/pfn
                    continue;
                }

                auto pin = CreatePin(context, pinKey);
                AICLI_LOG(CLI, Info, << "Evaluating Pin " << pin.ToString());

                auto existingPin = pinningIndex->GetPin(pinKey);

                if (existingPin)
                {
                    auto packageName = availableVersion->GetProperty(PackageVersionProperty::Name);

                    // Pin already exists.
                    // If it is the same, we do nothing. If it is different, check for the --force arg
                    if (pin == existingPin)
                    {
                        AICLI_LOG(CLI, Info, << "Pin already exists");
                        context.Reporter.Info() << Resource::String::PinAlreadyExists(packageName) << std::endl;
                        continue;
                    }

                    AICLI_LOG(CLI, Info, << "Another pin already exists for the package for source " << pinKey.SourceId);
                    if (context.Args.Contains(Execution::Args::Type::Force))
                    {
                        AICLI_LOG(CLI, Info, << "Overwriting pin due to --force argument");
                        context.Reporter.Warn() << Resource::String::PinExistsOverwriting(packageName) << std::endl;
                        pinsToAddOrUpdate.push_back(std::move(pin));
                    }
                    else
                    {
                        context.Reporter.Error() << Resource::String::PinExistsUseForceArg(packageName) << std::endl;
                        AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS);
                    }
                }
                else
                {
                    pinsToAddOrUpdate.push_back(std::move(pin));
                }
            }
        }

        if (!pinsToAddOrUpdate.empty())
        {
            for (const auto& pin : pinsToAddOrUpdate)

            {
                pinningIndex->AddOrUpdatePin(pin);
            }

            context.Reporter.Info() << Resource::String::PinAdded << std::endl;
        }
    }

    void RemovePin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        std::vector<Pinning::Pin> pins;
        std::set<std::pair<std::string, std::string>> sourcesAndIds;

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
        bool pinExists = false;

        // Note that if a source was specified in the command line,
        // that will be the only one we get version keys from.
        // So, we remove pins from all sources unless one was provided.
        auto packageVersionKeys = package->GetAvailableVersionKeys();

        auto installedVersion = package->GetInstalledVersion();
        auto installedProductCodes = installedVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
        auto installedPackageFamilyNames = installedVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);

        for (const auto& versionKey : packageVersionKeys)
        {
            auto availableVersion = package->GetAvailableVersion(versionKey);
            const auto availablePackageId = availableVersion->GetProperty(PackageVersionProperty::Id).get();
            const auto availableSourceId = availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get();
            const auto pinKeys = Pinning::GetPinKeysForAvailablePackage(availablePackageId, availableSourceId, installedProductCodes, installedPackageFamilyNames);

            for (const auto& pinKey : pinKeys)
            {
                if (sourcesAndIds.emplace(pinKey.SourceId, pinKey.ExtraId).second)
                {
                    if (pinningIndex->GetPin(pinKey))
                    {
                        AICLI_LOG(CLI, Info, << "Removing Pin " << pinKey.ToString());
                        pinningIndex->RemovePin(pinKey);
                        pinExists = true;
                    }
                }
            }
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
                auto installedVersion = match.Package->GetInstalledVersion();
                auto availableVersion = match.Package->GetAvailableVersion({ pinKey.SourceId, "", "" });
                if (availableVersion)
                {
                    table.OutputLine({
                        searchResult.Matches[0].Package->GetInstalledVersion()->GetProperty(PackageVersionProperty::Name),
                        pinKey.PackageId,
                        searchResult.Matches[0].Package->GetInstalledVersion()->GetProperty(PackageVersionProperty::Version),
                        availableVersion->GetProperty(PackageVersionProperty::SourceName),
                        std::string{ ToString(pin.GetType()) },
                        pin.GetGatedVersion().ToString(),
                    });
                }
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

        if (context.Get<Execution::Data::PinningIndex>()->ResetAllPins(sourceId))
        {
            context.Reporter.Info() << Resource::String::PinResetSuccessful << std::endl;
        }
        else
        {
            context.Reporter.Info() << Resource::String::PinNoPinsExist << std::endl;
        }
    }

    void CrossReferencePinsWithSource(Execution::Context& context)
    {
        const auto& pins = context.Get<Execution::Data::Pins>();
        const auto& source = context.Get<Execution::Data::Source>();

        std::vector<Pinning::Pin> matchingPins;
        for (const auto& pin : pins)
        {
            const auto& pinKey = pin.GetKey();
            auto searchRequest = GetSearchRequestForPin(pin.GetKey());
            auto searchResult = source.Search(searchRequest);

            // Ensure the match comes from the right source
            for (const auto& match : searchResult.Matches)
            {
                auto availableVersion = match.Package->GetAvailableVersion({ pinKey.SourceId, "", "" });
                if (availableVersion)
                {
                    matchingPins.push_back(pin);
                }
            }
        }

        context.Add<Execution::Data::Pins>(std::move(matchingPins));
    }

}