// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#define WIN32_NO_STATUS
#include <bcrypt.h>
#include "Public/AppInstallerHash.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility {
namespace
{
    struct HashAlgorithmInfo
    {
        HashAlgorithm Algorithm;
        LPCWSTR CngAlgorithmId;
        const char* Name;
        size_t HashBufferSizeInBytes;
    };

    constexpr HashAlgorithmInfo s_sha256Info
    {
        HashAlgorithm::Sha256,
        BCRYPT_SHA256_ALGORITHM,
        "SHA256",
        32,
    };

    const HashAlgorithmInfo& GetHashAlgorithmInfo(HashAlgorithm algorithm)
    {
        switch (algorithm)
        {
        case HashAlgorithm::Sha256:
            return s_sha256Info;
        default:
            THROW_HR_MSG(E_INVALIDARG, "Unsupported hash algorithm");
        }
    }
}

    struct HashContext
    {
        wil::unique_bcrypt_algorithm AlgHandle;
        wil::unique_bcrypt_hash HashHandle;
        DWORD HashLength = 0;
    };

    Hash::Hash(HashAlgorithm algorithm) : m_algorithm(algorithm), m_context(new HashContext{})
    {
        const HashAlgorithmInfo& algorithmInfo = GetHashAlgorithmInfo(m_algorithm);
        BCRYPT_ALG_HANDLE algHandle{};
        BCRYPT_HASH_HANDLE hashHandle{};
        DWORD resultLength = 0;

        THROW_IF_NTSTATUS_FAILED_MSG(
            BCryptOpenAlgorithmProvider(
                &algHandle,
                algorithmInfo.CngAlgorithmId,
                nullptr,
                0),
            "failed opening %hs algorithm provider",
            algorithmInfo.Name);
        m_context->AlgHandle.reset(algHandle);

        THROW_IF_NTSTATUS_FAILED_MSG(
            BCryptGetProperty(
                m_context->AlgHandle.get(),
                BCRYPT_HASH_LENGTH,
                reinterpret_cast<PBYTE>(&m_context->HashLength),
                sizeof(m_context->HashLength),
                &resultLength,
                0),
            "failed getting %hs hash length",
            algorithmInfo.Name);

        if (resultLength != sizeof(m_context->HashLength) || m_context->HashLength != algorithmInfo.HashBufferSizeInBytes)
        {
            THROW_HR_MSG(E_UNEXPECTED, "failed getting %hs hash length", algorithmInfo.Name);
        }

        THROW_IF_NTSTATUS_FAILED_MSG(
            BCryptCreateHash(
                m_context->AlgHandle.get(),
                &hashHandle,
                nullptr,
                0,
                nullptr,
                0,
                0),
            "failed creating %hs hash object",
            algorithmInfo.Name);
        m_context->HashHandle.reset(hashHandle);
    }

    void Hash::Add(const uint8_t* buffer, size_t cbBuffer)
    {
        EnsureNotFinished();

        const HashAlgorithmInfo& algorithmInfo = GetHashAlgorithmInfo(m_algorithm);
        THROW_IF_NTSTATUS_FAILED_MSG(
            BCryptHashData(m_context->HashHandle.get(), const_cast<PUCHAR>(buffer), static_cast<ULONG>(cbBuffer), 0),
            "failed adding %hs data",
            algorithmInfo.Name);
    }

    void Hash::Get(HashBuffer& hash)
    {
        EnsureNotFinished();

        const HashAlgorithmInfo& algorithmInfo = GetHashAlgorithmInfo(m_algorithm);
        hash.resize(m_context->HashLength);

        THROW_IF_NTSTATUS_FAILED_MSG(
            BCryptFinishHash(
                m_context->HashHandle.get(),
                hash.data(),
                m_context->HashLength,
                0),
            "failed getting %hs hash",
            algorithmInfo.Name);

        m_context.reset();
    }

    std::string Hash::ConvertToString(const HashBuffer& hashBuffer, size_t hashBufferSizeInBytes)
    {
        return Utility::ConvertToHexString(hashBuffer, hashBufferSizeInBytes);
    }

    std::wstring Hash::ConvertToWideString(const HashBuffer& hashBuffer, size_t hashBufferSizeInBytes)
    {
        return ConvertToUTF16(Hash::ConvertToString(hashBuffer, hashBufferSizeInBytes));
    }

    Hash::HashBuffer Hash::ConvertToBytes(const std::string& hashStr, size_t hashBufferSizeInBytes)
    {
        return Utility::ParseFromHexString(hashStr, hashBufferSizeInBytes);
    }

    Hash::HashBuffer Hash::ConvertToBytes(const std::wstring& hashStr, size_t hashBufferSizeInBytes)
    {
        return Utility::ParseFromHexString(Utility::ConvertToUTF8(hashStr), hashBufferSizeInBytes);
    }

    Hash::HashBuffer Hash::ComputeHash(HashAlgorithm algorithm, const std::uint8_t* buffer, std::uint32_t cbBuffer)
    {
        Hash hasher{ algorithm };
        hasher.Add(buffer, cbBuffer);
        return hasher.Get();
    }

    Hash::HashBuffer Hash::ComputeHash(HashAlgorithm algorithm, const std::vector<uint8_t>& buffer)
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), buffer.size() > std::numeric_limits<uint32_t>::max());
        return ComputeHash(algorithm, buffer.data(), static_cast<uint32_t>(buffer.size()));
    }

    Hash::HashBuffer Hash::ComputeHash(HashAlgorithm algorithm, std::string_view buffer)
    {
        return ComputeHash(algorithm, reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
    }

    Hash::HashBuffer Hash::ComputeHash(HashAlgorithm algorithm, std::istream& in)
    {
        return ComputeHashDetails(algorithm, in).Hash;
    }

    Hash::HashDetails Hash::ComputeHashDetails(HashAlgorithm algorithm, std::istream& in)
    {
        auto excState = in.exceptions();
        auto revertExcState = wil::scope_exit([excState, &in]() { in.exceptions(excState); });
        in.exceptions(std::ios_base::badbit);

        const int bufferSize = 1024 * 1024;
        auto buffer = std::make_unique<uint8_t[]>(bufferSize);

        Hash hasher{ algorithm };
        uint64_t totalSize = 0;

        while (in.good())
        {
            in.read(reinterpret_cast<char*>(buffer.get()), bufferSize);
            std::streamsize bytesRead = in.gcount();
            if (bytesRead)
            {
                hasher.Add(buffer.get(), static_cast<size_t>(bytesRead));
                totalSize += static_cast<uint64_t>(bytesRead);
            }
        }

        if (in.eof())
        {
            HashDetails result;
            result.Hash = hasher.Get();
            result.SizeInBytes = totalSize;
            return result;
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_STREAM_READ_FAILURE);
        }
    }

    Hash::HashBuffer Hash::ComputeHashFromFile(HashAlgorithm algorithm, const std::filesystem::path& path)
    {
        std::ifstream inStream{ path, std::ifstream::binary };
        const Utility::Hash::HashBuffer& targetFileHash = Utility::Hash::ComputeHash(algorithm, inStream);
        inStream.close();
        return targetFileHash;
    }

    Hash::HashBuffer Hash::ComputeHashFromHandle(HashAlgorithm algorithm, HANDLE fileHandle)
    {
        constexpr DWORD bufferSize = 1024 * 1024;
        auto buffer = std::make_unique<uint8_t[]>(bufferSize);
        Hash hasher{ algorithm };
        DWORD bytesRead = 0;

        while (ReadFile(fileHandle, buffer.get(), bufferSize, &bytesRead, nullptr) && bytesRead > 0)
        {
            hasher.Add(buffer.get(), bytesRead);
        }

        return hasher.Get();
    }

    void Hash::HashContextDeleter::operator()(HashContext* context)
    {
        delete context;
    }

    bool Hash::AreEqual(const HashBuffer& first, const HashBuffer& second)
    {
        return (first.size() == second.size() && std::equal(first.begin(), first.end(), second.begin()));
    }

    void Hash::EnsureNotFinished() const
    {
        if (!m_context)
        {
            THROW_HR_MSG(E_UNEXPECTED, "The hash is already finished");
        }
    }
}
