// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PinningData.h"
#include "Microsoft/PinningIndex.h"

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Repository::Microsoft;

namespace AppInstaller::Pinning
{
    PinningData::PinningData() = default;
    PinningData::PinningData(const PinningData&) = default;
    PinningData& PinningData::operator=(const PinningData&) = default;
    PinningData::PinningData(PinningData&&) noexcept = default;
    PinningData& PinningData::operator=(PinningData&&) noexcept = default;
    PinningData::~PinningData() = default;

    PinningData::PinningData(bool readOnly)
    {
        if (readOnly)
        {
            m_database = PinningIndex::OpenIfExists(SQLiteStorageBase::OpenDisposition::Read);
        }
        else
        {
            m_database = PinningIndex::OpenOrCreateDefault(SQLiteStorageBase::OpenDisposition::ReadWrite);
        }
    }

    PinningData::operator bool() const
    {
        return IsDatabaseConnected();
    }

    bool PinningData::IsDatabaseConnected() const
    {
        return static_cast<bool>(m_database);
    }

    void PinningData::AddOrUpdatePin(const Pin& pin)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !IsDatabaseConnected());
        m_database->AddOrUpdatePin(pin);
    }

    void PinningData::RemovePin(const PinKey& pinKey)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !IsDatabaseConnected());
        m_database->RemovePin(pinKey);
    }

    std::optional<Pin> PinningData::GetPin(const PinKey& pinKey)
    {
        return IsDatabaseConnected() ? m_database->GetPin(pinKey) : std::nullopt;
    }

    std::vector<Pin> PinningData::GetAllPins()
    {
        return IsDatabaseConnected() ? m_database->GetAllPins() : std::vector<Pin>{};
    }

    bool PinningData::ResetAllPins(std::string_view sourceId)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !IsDatabaseConnected());
        return m_database->ResetAllPins(sourceId);
    }
}
