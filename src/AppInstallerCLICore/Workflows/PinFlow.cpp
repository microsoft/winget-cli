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
            if (context.Args.Contains(Execution::Args::Type::BlockingPin))
            {
                return Pinning::Pin::CreateBlockingPin({ packageId, sourceId });
            }
            else if (context.Args.Contains(Execution::Args::Type::GatedVersion))
            {
                return Pinning::Pin::CreateGatingPin({ packageId, sourceId }, context.Args.GetArg(Execution::Args::Type::GatedVersion));
            }
            else
            {
                return Pinning::Pin::CreatePinningPin({ packageId, sourceId });
            }
        }
    }

    void OpenPinningIndex(Execution::Context& context)
    {
        auto indexPath = Runtime::GetPathTo(Runtime::PathName::LocalState) / "pinning.db";

        AICLI_LOG(CLI, Info, << "Openning pinning index");
        auto pinningIndex = std::filesystem::exists(indexPath) ?
            PinningIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite) :
            PinningIndex::CreateNew(indexPath.u8string());

        context.Add<Execution::Data::PinningIndex>(std::make_shared<PinningIndex>(std::move(pinningIndex)));
    }

    void GetAllPins(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "Getting all existing pins");
        context.Add<Execution::Data::Pins>(context.Get<Execution::Data::PinningIndex>()->GetAllPins());
    }

    void SearchPin(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "SEARCHING PIN");
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

    // Adds a pin for the current package.
    // A separate pin will be added for each source.
    // Required Args: None
    // Inputs: PinningIndex, Package
    // Outputs: None
    void AddPin(Execution::Context& context)
    {
        auto package = context.Get<Execution::Data::Package>();
        std::vector<Pinning::Pin> pins;

        // TODO: We should support querying the multiple sources for a package, instead of just one
        auto availableVersion = package->GetLatestAvailableVersion();

        Pinning::PinKey pinKey{
            availableVersion->GetProperty(PackageVersionProperty::Id).get(),
            availableVersion->GetProperty(PackageVersionProperty::SourceIdentifier).get() };
        auto pin = CreatePin(context, pinKey.PackageId, pinKey.SourceId);
        AICLI_LOG(CLI, Info, << "Adding pin with type " << ToString(pin.GetType()) << " for package[" << pin.GetPackageId() << "] from source [" << pin.GetSourceId() << "]");

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

    // Removes all the pins associated with a package.
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
        if (!pinningIndex->GetPin(pinKey))
        {
            // TODO: Report pin not found
        }

        pinningIndex->RemovePin(pinKey);
    }

    // Report the pins in a table.
    // Required Args: None
    // Inputs: Pins
    // Outputs: None
    void ReportPins(Execution::Context& context)
    {
        // TODO: Use package and source names
        Execution::TableOutput<4> table(context.Reporter,
            {
                Resource::String::SearchId,
                Resource::String::SearchSource,
                Resource::String::PinType,
                Resource::String::GatedVersion
            });

        for (const auto& pin : context.Get<Execution::Data::Pins>())
        {
            // TODO: Avoid these conversions to string
            table.OutputLine({
                pin.GetPackageId(),
                std::string{ pin.GetSourceId() },
                std::string{ ToString(pin.GetType()) },
                pin.GetType() == Pinning::PinType::Gating ? pin.GetGatedVersion().ToString() : "",
                });
        }

        table.Complete();
    }

    // Resets all the existing pins.
    // Required Args: None
    // Inputs: PinningIndex
    // Outputs: None
    void ResetAllPins(Execution::Context& context)
    {
        AICLI_LOG(CLI, Info, << "Resetting all pins");
        if (context.Args.Contains(Execution::Args::Type::Force))
        {
            context.Reporter.Info() << Resource::String::PinResettingAll << std::endl;
            auto pinningIndex = context.Get<Execution::Data::PinningIndex>();
            pinningIndex->ResetAllPins();
        }
        else
        {
            AICLI_LOG(CLI, Info, << "--force argument is not present");
            context.Reporter.Info() << Resource::String::PinResetUseForceArg << std::endl;

            // TODO: Report pins here
        }
    }

    // Updates the list of pins to include only those matching the current open source
    // Required Args: None
    // Inputs: Pins, Source
    // Outputs: None
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