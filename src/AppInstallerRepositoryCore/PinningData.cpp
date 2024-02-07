// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PinningData.h"
#include "Microsoft/PinningIndex.h"

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;

namespace AppInstaller::Pinning
{
    PinningData::PinningData() = default;
    PinningData::PinningData(const PinningData&) = default;
    PinningData& PinningData::operator=(const PinningData&) = default;
    PinningData::PinningData(PinningData&&) noexcept = default;
    PinningData& PinningData::operator=(PinningData&&) noexcept = default;
    PinningData::~PinningData() = default;

    PinningData::PinningData(Disposition disposition)
    {
        if (disposition == Disposition::ReadOnly)
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

    PinningData::PinStateEvaluator::PinStateEvaluator(
        PinBehavior behavior,
        std::shared_ptr<PinningIndex> database,
        const std::shared_ptr<IPackageVersion>& installedVersion) :
        m_behavior(behavior), m_database(std::move(database))
    {
        if (m_behavior == PinBehavior::IgnorePins)
        {
            // No need to keep the database reference if we are just going to ignore it
            m_database.reset();
        }
        else if (installedVersion)
        {
            PinKey key = PinKey::GetPinKeyForInstalled(installedVersion->GetProperty(PackageVersionProperty::Id));
            m_installedPin = m_database->GetPin(key);
        }

        if (installedVersion)
        {
            m_installedVersion = Utility::VersionAndChannel{
                Utility::Version{ installedVersion->GetProperty(PackageVersionProperty::Version) },
                Utility::Channel{ installedVersion->GetProperty(PackageVersionProperty::Channel) }
            };
        }
    }

    PinningData::PinStateEvaluator::PinStateEvaluator(const PinStateEvaluator&) = default;
    PinningData::PinStateEvaluator& PinningData::PinStateEvaluator::operator=(const PinStateEvaluator&) = default;
    PinningData::PinStateEvaluator::PinStateEvaluator(PinStateEvaluator&&) noexcept = default;
    PinningData::PinStateEvaluator& PinningData::PinStateEvaluator::operator=(PinStateEvaluator&&) noexcept = default;

    PinningData::PinStateEvaluator::~PinStateEvaluator() = default;

    // Gets the latest available package that fits within the pinning restrictions.
    std::shared_ptr<AppInstaller::Repository::IPackageVersion> PinningData::PinStateEvaluator::GetLatestAvailableVersionForPins(const std::shared_ptr<AppInstaller::Repository::IPackage>& package);

    bool PinningData::PinStateEvaluator::IsUpdate(const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& availableVersion)
    {
        if (m_installedVersion && availableVersion)
        {
            Utility::VersionAndChannel availableVersionAndChannel{
                Utility::Version{ availableVersion->GetProperty(PackageVersionProperty::Version) },
                Utility::Channel{ availableVersion->GetProperty(PackageVersionProperty::Channel) }
            };

            return m_installedVersion->IsUpdatedBy(availableVersionAndChannel);
        }

        return false;
    }

    // Determines if the given version is a possible update given the current pinning restrictions.
    PinType PinningData::PinStateEvaluator::EvaluatePinType(const AppInstaller::Repository::PackageVersionKey& key);

    // Creates an object for use in evaluating pinning data for a given package
    PinningData::PinStateEvaluator PinningData::CreatePinStateEvaluator(
        PinBehavior behavior,
        const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& installedVersion)
    {
        return { behavior, m_database, installedVersion };
    }
}
