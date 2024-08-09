// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PinningData.h"
#include "Microsoft/PinningIndex.h"
#include "Public/winget/RepositorySource.h"

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;

namespace AppInstaller::Pinning
{
    namespace
    {
        // Evaluates the pinning state of a version for a single pin.
        PinType EvaluatePinnedStateForVersion(
            const Utility::Version& version,
            const std::optional<Pin>& pin,
            PinBehavior behavior)
        {
            if (pin)
            {
                if (pin->GetType() == PinType::Blocking
                    || (pin->GetType() == PinType::Pinning && behavior != PinBehavior::IncludePinned)
                    || (pin->GetType() == PinType::Gating && !pin->GetGatedVersion().IsValidVersion(version)))
                {
                    return pin->GetType();
                }
            }

            return PinType::Unknown;
        }

        // Gets the pinned state for an available version that may have a pin,
        // and optionally an additional pin that could come from the installed version.
        // If both pins are present, we return the one that is the most strict.
        Pinning::PinType GetPinnedStateForVersion(
            const Utility::Version& version,
            const std::optional<Pinning::Pin>& availablePin,
            const std::optional<Pinning::Pin>& installedPin,
            PinBehavior behavior)
        {
            if (behavior == PinBehavior::IgnorePins)
            {
                return Pinning::PinType::Unknown;
            }

            return Stricter(
                EvaluatePinnedStateForVersion(version, availablePin, behavior),
                EvaluatePinnedStateForVersion(version, installedPin, behavior));
        }
    }

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
        if (m_behavior == PinBehavior::IgnorePins || !installedVersion)
        {
            // Because the database isn't guaranteed to be present, align ignoring pins with there being no pins to ignore.
            // Also do not consider pins when there is no installed version. This is to remain consistent with the previous
            // implementation. If this is to be changed, more install paths will need to be do pinning checks to ensure
            // that one could, for instance, block the install of a package.
            m_database.reset();
        }
        else if (m_database)
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

    std::shared_ptr<IPackageVersion> PinningData::PinStateEvaluator::GetLatestAvailableVersionForPins(const std::shared_ptr<IPackageVersionCollection>& package)
    {
        if (!m_database)
        {
            return package->GetLatestVersion();
        }

        auto availableVersionKeys = package->GetVersionKeys();

        // Skip until we find a version that isn't pinned
        for (const auto& availableVersion : availableVersionKeys)
        {
            std::shared_ptr<IPackageVersion> packageVersion = package->GetVersion(availableVersion);
            if (EvaluatePinType(packageVersion) == Pinning::PinType::Unknown)
            {
                return packageVersion;
            }
        }

        return {};
    }

    bool PinningData::PinStateEvaluator::IsUpdate(const std::shared_ptr<IPackageVersion>& availableVersion)
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

    PinType PinningData::PinStateEvaluator::EvaluatePinType(const std::shared_ptr<AppInstaller::Repository::IPackageVersion>& packageVersion)
    {
        if (!m_database || !packageVersion)
        {
            return PinType::Unknown;
        }

        std::optional<Pin> incomingPin;

        PinKey pinKey{ packageVersion->GetProperty(PackageVersionProperty::Id).get(), packageVersion->GetSource().GetIdentifier()};
        auto itr = m_availablePins.find(pinKey);
        if (itr == m_availablePins.end())
        {
            incomingPin = m_database->GetPin(pinKey);
            m_availablePins[pinKey] = incomingPin;
        }
        else
        {
            incomingPin = itr->second;
        }

        return GetPinnedStateForVersion(packageVersion->GetProperty(PackageVersionProperty::Version).get(), incomingPin, m_installedPin, m_behavior);
    }

    // Creates an object for use in evaluating pinning data for a given package
    PinningData::PinStateEvaluator PinningData::CreatePinStateEvaluator(
        PinBehavior behavior,
        const std::shared_ptr<IPackageVersion>& installedVersion)
    {
        return { behavior, m_database, installedVersion };
    }
}
