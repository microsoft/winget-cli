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

        void AddOrUpdatePin(const Pin& pin);

        void RemovePin(const PinKey& pinKey);

        std::optional<Pin> GetPin(const PinKey& pinKey);

        std::vector<Pin> GetAllPins();

        bool ResetAllPins(std::string_view sourceId = {});

    private:
        std::shared_ptr<AppInstaller::Repository::Microsoft::PinningIndex> m_database;
    };
}
