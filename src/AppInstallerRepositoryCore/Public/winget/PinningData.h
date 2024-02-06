// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Pin.h>

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

        // Creates a usable pinning data with the given read/write capability.
        PinningData(bool readOnly);

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



    private:
        std::shared_ptr<AppInstaller::Repository::Microsoft::PinningIndex> m_database;
    };
}
