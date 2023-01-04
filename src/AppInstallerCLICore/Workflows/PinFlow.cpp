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
        Pinning::Pin CreatePin(Execution::Context& context, std::string_view packageId, std::string_view sourceId, const std::string& installedVersion)
        {
            if (context.Args.Contains(Execution::Args::Type::GatedVersion))
            {
                return Pinning::Pin::CreateGatingPin({ packageId, sourceId }, context.Args.GetArg(Execution::Args::Type::GatedVersion));
            }
            else if (context.Args.Contains(Execution::Args::Type::BlockingPin))
            {
                return Pinning::Pin::CreateBlockingPin({ packageId, sourceId }, { installedVersion });
            }
            else
            {
                return Pinning::Pin::CreatePinningPin({ packageId, sourceId }, { installedVersion });
            }
        }
    }

    void OpenPinningIndex(Execution::Context& context)
    {
        auto pinningIndex = PinningIndex::OpenOrCreateDefault(SQLiteStorageBase::OpenDisposition::ReadWrite);
        context.Add<Execution::Data::PinningIndex>(std::make_shared<PinningIndex>(std::move(pinningIndex)));
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
        std::set<std::string> sources;

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();

        auto packageVersionKeys = package->GetAvailableVersionKeys(PinBehavior::IgnorePins);
        for (auto versionKey : packageVersionKeys)
        {
            auto availableVersion = package->GetAvailableVersion(versionKey, PinBehavior::IgnorePins);
            Pinning::PinKey pinKey{
                availableVersion->GetProperty(PackageVersionProperty::Id).get(),
                availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() };

            if (sources.insert(pinKey.SourceId).second)
            {
                auto pin = pinningIndex->GetPin(pinKey);
                if (pin)
                {
                    pins.emplace_back(std::move(pin.value()));
                }
            }
        }

        context.Add<Execution::Data::Pins>(std::move(pins));
    }

    void AddPin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        if (!installedVersion)
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }

        auto installedVersionString = installedVersion->GetProperty(PackageVersionProperty::Version);

        std::vector<Pinning::Pin> pinsToAddOrUpdate;
        std::set<std::string> sources;

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();

        auto packageVersionKeys = package->GetAvailableVersionKeys(PinBehavior::IgnorePins);
        for (auto versionKey : packageVersionKeys)
        {
            auto availableVersion = package->GetAvailableVersion(versionKey, PinBehavior::IgnorePins);
            Pinning::PinKey pinKey{
                availableVersion->GetProperty(PackageVersionProperty::Id).get(),
                availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() };

            if (!sources.insert(pinKey.SourceId).second)
            {
                // We already considered the pin for this source
                continue;
            }

            auto pin = CreatePin(context, pinKey.PackageId, pinKey.SourceId, installedVersionString);
            AICLI_LOG(CLI, Info, << "Evaluating pin with type " << ToString(pin.GetType()) << " for package [" << pin.GetPackageId() << "] from source [" << pin.GetSourceId() << "]");

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

        if (!pinsToAddOrUpdate.empty())
        {
            for (auto pin : pinsToAddOrUpdate)
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
        std::set<std::string> sources;

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
        bool pinExists = false;

        // Note that if a source was specified in the command line,
        // that will be the only one we get version keys from.
        // So, we remove pins from all sources unless one was provided.
        auto packageVersionKeys = package->GetAvailableVersionKeys(PinBehavior::IgnorePins);
        for (auto versionKey : packageVersionKeys)
        {
            auto availableVersion = package->GetAvailableVersion(versionKey, PinBehavior::IgnorePins);
            Pinning::PinKey pinKey{
                availableVersion->GetProperty(PackageVersionProperty::Id).get(),
                availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() };

            if (sources.insert(pinKey.SourceId).second)
            {
                if (pinningIndex->GetPin(pinKey))
                {
                    AICLI_LOG(CLI, Info, << "Removing pin for package [" << pinKey.PackageId << "] from source [" << pinKey.SourceId << "]");
                    pinningIndex->RemovePin(pinKey);
                    pinExists = true;
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

        // Get a mapping of source IDs to names so that we can show something nicer
        std::map<std::string, std::string> sourceNames;
        for (const auto& source : Repository::Source::GetCurrentSources())
        {
            sourceNames[source.Identifier] = source.Name;
        }

        Execution::TableOutput<4> table(context.Reporter,
            {
                Resource::String::SearchId,
                Resource::String::SearchSource,
                Resource::String::PinType,
                Resource::String::SearchVersion,
            });

        for (const auto& pin : pins)
        {
            table.OutputLine({
                pin.GetPackageId(),
                sourceNames[pin.GetSourceId()],
                std::string{ ToString(pin.GetType()) },
                pin.GetVersion().ToString(),
                });
        }

        table.Complete();
    }

    void ResetAllPins(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "Resetting all pins");
        if (context.Args.Contains(Execution::Args::Type::Force))
        {
            context.Reporter.Info() << Resource::String::PinResettingAll << std::endl;

            if (context.Get<Execution::Data::Pins>().empty())
            {
                context.Reporter.Info() << Resource::String::PinNoPinsExist << std::endl;
            }
            else
            {
                auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
                pinningIndex->ResetAllPins();
            }
        }
        else
        {
            AICLI_LOG(CLI, Info, << "--force argument is not present");
            context.Reporter.Info() << Resource::String::PinResetUseForceArg << std::endl;
            context << ReportPins;
        }
    }

    void CrossReferencePinsWithSource(Execution::Context& context)
    {
        const auto& pins = context.Get<Execution::Data::Pins>();
        const auto& source = context.Get<Execution::Data::Source>();

        std::vector<Pinning::Pin> matchingPins;
        for (const auto pin : pins)
        {
            SearchRequest searchRequest;
            searchRequest.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, pin.GetPackageId());
            auto searchResult = source.Search(searchRequest);

            // Ensure the match comes from the right source
            for (const auto& match : searchResult.Matches)
            {
                auto availableVersion = match.Package->GetAvailableVersion({ pin.GetSourceId(), "", "" }, PinBehavior::IgnorePins);
                if (availableVersion)
                {
                    matchingPins.push_back(pin);
                }
            }
        }

        context.Add<Execution::Data::Pins>(std::move(matchingPins));
    }

}