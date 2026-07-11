// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Pin.h>
#include <winget/RepositorySearch.h>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace AppInstaller::Repository::Microsoft
{
    struct PinningIndex;
}

namespace AppInstaller::Pinning
{
    // Possible ways to consider pins when getting a package's available versions
    enum class PinBehavior
    {
        // Ignore pins, returns all available versions.
        IgnorePins,
        // Include available versions for packages with a Pinning pin.
        // Blocking pins and Gating pins still respected.
        IncludePinned,
        // Respect all the types of pins.
        ConsiderPins,
    };

    // The public representation of the pinning database.
    struct PinningData
    {
        // Creates an empty pinning data.
        PinningData();

        // Enum to make the pinning data disposition clear in the caller.
        enum class Disposition
        {
            // The data can only be read.
            ReadOnly,
            // The data can be read and written.
            ReadWrite,
        };

        // Creates a usable pinning data with the given read/write capability.
        PinningData(Disposition disposition);

        PinningData(const PinningData&);
        PinningData& operator=(const PinningData&);
        PinningData(PinningData&&) noexcept;
        PinningData& operator=(PinningData&&) noexcept;
        ~PinningData();

        // Determines if the pinning database is opened
        operator bool() const;
        bool IsDatabaseConnected() const;

        // Pass through functions to the index itself
        void AddOrUpdatePin(const Pin& pin);
        void RemovePin(const PinKey& pinKey);
        std::optional<Pin> GetPin(const PinKey& pinKey);
        std::vector<Pin> GetAllPins();
        bool ResetAllPins(std::string_view sourceId = {});

        // A type used for evaluating the pinning state for a given package.
        struct PinStateEvaluator
        {
            PinStateEvaluator(
                PinBehavior behavior,
                std::shared_ptr<AppInstaller::Repository::Microsoft::PinningIndex> database,
                const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& installedVersion);

            PinStateEvaluator(const PinStateEvaluator&);
            PinStateEvaluator& operator=(const PinStateEvaluator&);
            PinStateEvaluator(PinStateEvaluator&&) noexcept;
            PinStateEvaluator& operator=(PinStateEvaluator&&) noexcept;

            ~PinStateEvaluator();

            // Gets the latest available package version that fits within the pinning restrictions.
            // This should be the package object that contains available versions associated with the installed version for which this evaluator was created.
            std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetLatestAvailableVersionForPins(const std::shared_ptr<AppInstaller::Repository::IPackageVersionCollection>& package);

            // Determines if the given version is an update to the installed version that this object was created with.
            // This should be a version associated with the installed version for which this evaluator was created.
            bool IsUpdate(const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& availableVersion);

            // Determines the pin type to apply to the given version.
            PinType EvaluatePinType(const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& packageVersion);

        private:
            PinBehavior m_behavior;
            std::shared_ptr<AppInstaller::Repository::Microsoft::PinningIndex> m_database;
            std::optional<Pin> m_installedPin;
            std::optional<Utility::VersionAndChannel> m_installedVersion;
            // Cache pins for available version to reduce database lookups.
            std::map<PinKey, std::optional<Pin>> m_availablePins;
        };

        // Creates an object for use in evaluating pinning data for a given package
        PinStateEvaluator CreatePinStateEvaluator(
            PinBehavior behavior,
            const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& installedVersion);

    private:
        std::shared_ptr<AppInstaller::Repository::Microsoft::PinningIndex> m_database;
    };
}
