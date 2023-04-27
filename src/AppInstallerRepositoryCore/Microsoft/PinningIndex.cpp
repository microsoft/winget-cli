// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PinningIndex.h"
#include "SQLiteStorageBase.h"
#include "Schema/Pinning_1_0/PinningIndexInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    PinningIndex PinningIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Pinning Index with version [" << version << "] at '" << filePath << "'");
        PinningIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "pinningindex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    std::optional<std::filesystem::path> s_PinningIndexOverride{};
    void TestHook_SetPinningIndex_Override(std::optional<std::filesystem::path>&& indexPath)
    {
        s_PinningIndexOverride = std::move(indexPath);
    }
#endif

    std::shared_ptr<PinningIndex> PinningIndex::OpenOrCreateDefault(OpenDisposition openDisposition)
    {
        const auto DefaultIndexPath = Runtime::GetPathTo(Runtime::PathName::LocalState) / "pinning.db";
#ifndef AICLI_DISABLE_TEST_HOOKS
        const auto indexPath = s_PinningIndexOverride.has_value() ? s_PinningIndexOverride.value() : DefaultIndexPath;
#else
        const auto indexPath = DefaultIndexPath;
#endif

        AICLI_LOG(Repo, Info, << "Opening pinning index");


        try
        {
            if (std::filesystem::exists(indexPath))
            {
                if (std::filesystem::is_regular_file(indexPath))
                {
                    try
                    {
                        AICLI_LOG(Repo, Info, << "Opening existing pinning index");
                        return std::make_shared<PinningIndex>(PinningIndex::Open(indexPath.u8string(), openDisposition));
                    }
                    CATCH_LOG();
                }

                AICLI_LOG(Repo, Info, << "Attempting to delete bad index file");
                std::filesystem::remove_all(indexPath);
            }

            return std::make_shared<PinningIndex>(PinningIndex::CreateNew(indexPath.u8string()));
        }
        CATCH_LOG();

        return {};
    }

    PinningIndex::IdType PinningIndex::AddPin(const Pinning::Pin& pin)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding Pin " << pin.ToString());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "pinningindex_addpin");

        IdType result = m_interface->AddPin(m_dbconn, pin);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    bool PinningIndex::UpdatePin(const Pinning::Pin& pin)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Updating Pin " << pin.ToString());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "pinningindex_updatepin");

        bool result = m_interface->UpdatePin(m_dbconn, pin).first;

        if (result)
        {
            SetLastWriteTime();
            savepoint.Commit();
        }

        return result;
    }

    void PinningIndex::AddOrUpdatePin(const Pinning::Pin& pin)
    {
        auto existingPin = GetPin(pin.GetKey());
        if (existingPin.has_value())
        {
            UpdatePin(pin);
        }
        else
        {
            AddPin(pin);
        }
    }

    void PinningIndex::RemovePin(const Pinning::PinKey& pinKey)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Removing Pin " << pinKey.ToString());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "pinningIndex_removePin");

        m_interface->RemovePin(m_dbconn, pinKey);

        SetLastWriteTime();

        savepoint.Commit();
    }

    std::optional<Pinning::Pin> PinningIndex::GetPin(const Pinning::PinKey& pinKey)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetPin(m_dbconn, pinKey);
    }

    std::vector<Pinning::Pin> PinningIndex::GetAllPins()
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->GetAllPins(m_dbconn);
    }

    bool PinningIndex::ResetAllPins(std::string_view sourceId)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        return m_interface->ResetAllPins(m_dbconn, sourceId);
    }

    std::unique_ptr<Schema::IPinningIndex> PinningIndex::CreateIPinningIndex() const
    {
        if (m_version == Schema::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Pinning_V1_0::PinningIndexInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    PinningIndex::PinningIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Pinning Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateIPinningIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    PinningIndex::PinningIndex(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateIPinningIndex();
        m_version = m_interface->GetVersion();
    }
}