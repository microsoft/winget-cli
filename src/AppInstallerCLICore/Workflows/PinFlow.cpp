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
        Pinning::Pin CreatePin(Execution::Context& context, std::string_view packageId, std::string_view sourceId)
        {
            if (context.Args.Contains(Execution::Args::Type::GatedVersion))
            {
                return Pinning::Pin::CreateGatingPin({ packageId, sourceId }, context.Args.GetArg(Execution::Args::Type::GatedVersion));
            }
            else if (context.Args.Contains(Execution::Args::Type::BlockingPin))
            {
                return Pinning::Pin::CreateBlockingPin({ packageId, sourceId });
            }
            else
            {
                return Pinning::Pin::CreatePinningPin({ packageId, sourceId });
            }
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

        // TODO: We should support querying the multiple sources for a package, instead of just one
        auto availableVersion = package->GetLatestAvailableVersion();

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
        auto pin = pinningIndex->GetPin({
            availableVersion->GetProperty(PackageVersionProperty::Id).get(),
            availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() });
        if (pin)
        {
            pins.emplace_back(std::move(pin.value()));
        }

        context.Add<Execution::Data::Pins>(std::move(pins));
    }

    void AddPin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedVersion = context.Get<Execution::Data::InstalledPackageVersion>();

        std::vector<Pinning::Pin> pins;

        // TODO: We should support querying the multiple sources for a package, instead of just one
        auto availableVersion = package->GetLatestAvailableVersion();

        Pinning::PinKey pinKey{
            availableVersion->GetProperty(PackageVersionProperty::Id).get(),
            availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() };
        auto pin = CreatePin(context, pinKey.PackageId, pinKey.SourceId);
        AICLI_LOG(CLI, Info, << "Adding pin with type " << ToString(pin.GetType()) << " for package [" << pin.GetPackageId() << "] from source [" << pin.GetSourceId() << "]");


        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
        auto existingPin = pinningIndex->GetPin(pinKey);

        if (existingPin)
        {
            // Pin already exists.
            // If it is the same, we do nothing. If it is different, check for the --force arg
            if (pin == existingPin)
            {
                AICLI_LOG(CLI, Info, << "Pin already exists");
                context.Reporter.Info() << Resource::String::PinAlreadyExists << std::endl;
                return;
            }

            AICLI_LOG(CLI, Info, << "Another pin already exists for the package");
            if (context.Args.Contains(Execution::Args::Type::Force))
            {
                AICLI_LOG(CLI, Info, << "Overwriting pin due to --force argument");
                context.Reporter.Warn() << Resource::String::PinExistsOverwriting << std::endl;
                pinningIndex->UpdatePin(pin);
            }
            else
            {
                context.Reporter.Error() << Resource::String::PinExistsUseForceArg << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS);
            }
        }
        else
        {
            pinningIndex->AddPin(pin);
            AICLI_LOG(CLI, Info, << "Finished adding pin");
            context.Reporter.Info() << Resource::String::PinAdded << std::endl;
        }
    }

    void RemovePin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        std::vector<Pinning::Pin> pins;

        // TODO: We should support querying the multiple sources for a package, instead of just one
        auto availableVersion = package->GetLatestAvailableVersion();

        auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
        Pinning::PinKey pinKey{
            availableVersion->GetProperty(PackageVersionProperty::Id).get(),
            availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() };
        AICLI_LOG(CLI, Info, << "Removing pin for package [" << pinKey.PackageId << "] from source [" << pinKey.SourceId << "]");
        if (!pinningIndex->GetPin(pinKey))
        {
            AICLI_LOG(CLI, Warning, << "Pin does not exist");
            context.Reporter.Warn() << Resource::String::PinDoesNotExist(pinKey.PackageId) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
        }

        pinningIndex->RemovePin(pinKey);
    }

    void ReportPins(Execution::Context& context)
    {
        const auto& pins = context.Get<Execution::Data::Pins>();
        if (pins.empty())
        {
            context.Reporter.Info() << Resource::String::PinNoPinsExist << std::endl;
            return;
        }

        // TODO: Use package and source names
        Execution::TableOutput<4> table(context.Reporter,
            {
                Resource::String::SearchId,
                Resource::String::SearchSource,
                Resource::String::SearchVersion,
                Resource::String::PinType,
            });

        for (const auto& pin : pins)
        {
            // TODO: Avoid these conversions to string
            table.OutputLine({
                pin.GetPackageId(),
                std::string{ pin.GetSourceId() },
                pin.GetGatedVersion().ToString(),
                std::string{ ToString(pin.GetType()) },
                });
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
        std::copy_if(pins.begin(), pins.end(), std::back_inserter(matchingPins), [&](Pinning::Pin pin) {
            // TODO: Filter to source
            SearchRequest searchRequest;
            searchRequest.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, pin.GetPackageId());
            auto searchResult = source.Search(searchRequest);

            return !searchResult.Matches.empty();
            });

        context.Add<Execution::Data::Pins>(std::move(matchingPins));
    }

}