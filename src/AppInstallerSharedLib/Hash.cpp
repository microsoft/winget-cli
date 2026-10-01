// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#define WIN32_NO_STATUS
#include <bcrypt.h>
#include "Public/winget/Hash.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Cryptography {
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

        // Open an algorithm handle
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptOpenAlgorithmProvider(
            &algHandle,                   // Alg Handle pointer
            algorithmInfo.CngAlgorithmId, // Cryptographic Algorithm name (null terminated unicode string)
            nullptr,                      // Provider name; if null, the default provider is loaded
            0),                           // Flags
            "failed opening %hs algorithm provider",
            algorithmInfo.Name);
        m_context->AlgHandle.reset(algHandle);

        // Obtain the length of the hash
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptGetProperty(
            m_context->AlgHandle.get(),                      // Handle to a CNG object
            BCRYPT_HASH_LENGTH,                              // Property name (null terminated unicode string)
            reinterpret_cast<PBYTE>(&m_context->HashLength), // Address of the output buffer which receives the property value
            sizeof(m_context->HashLength),                   // Size of the buffer in bytes
            &resultLength,                                   // Number of bytes that were copied into the buffer
            0),                                              // Flags
            "failed getting %hs hash length",
            algorithmInfo.Name);

        if (resultLength != sizeof(m_context->HashLength) || m_context->HashLength != algorithmInfo.HashBufferSizeInBytes)
        {
            THROW_HR_MSG(E_UNEXPECTED, "failed getting %hs hash length", algorithmInfo.Name);
        }

        // Create a hash handle
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptCreateHash(
            m_context->AlgHandle.get(), // Handle to an algorithm provider
            &hashHandle,                // A pointer to a hash handle - can be a hash or hmac object
            nullptr,                    // Pointer to the buffer that receives the hash/hmac object
            0,                          // Size of the buffer in bytes
            nullptr,                    // A pointer to a key to use for the hash or MAC
            0,                          // Size of the key in bytes
            0),                         // Flags
            "failed creating %hs hash object",
            algorithmInfo.Name);
        m_context->HashHandle.reset(hashHandle);
    }

    Hash::~Hash() = default;
    Hash::Hash(Hash&&) noexcept = default;
    Hash& Hash::operator=(Hash&&) noexcept = default;

    void Hash::Add(const uint8_t* buffer, size_t cbBuffer)
    {
        EnsureNotFinished();
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), cbBuffer > std::numeric_limits<ULONG>::max());

        // Add the data
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
        // Size the hash buffer appropriately
        hash.resize(m_context->HashLength);

        // Obtain the hash of the message(s) into the hash buffer
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptFinishHash(
            m_context->HashHandle.get(), // Handle to the hash or MAC object
            hash.data(),                 // A pointer to a buffer that receives the hash or MAC value
            m_context->HashLength,       // Size of the buffer in bytes
            0),                          // Flags
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
        return Utility::ConvertToUTF16(Hash::ConvertToString(hashBuffer, hashBufferSizeInBytes));
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
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), buffer.size() > std::numeric_limits<uint32_t>::max());
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
        // Throw exceptions on badbit
        in.exceptions(std::ios_base::badbit);

        const int bufferSize = 1024 * 1024; // 1MB
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
        const Hash::HashBuffer& targetFileHash = Hash::ComputeHash(algorithm, inStream);
        inStream.close();
        return targetFileHash;
    }

    Hash::HashBuffer Hash::ComputeHashFromHandle(HashAlgorithm algorithm, HANDLE fileHandle)
    {
        constexpr DWORD bufferSize = 1024 * 1024;
        auto buffer = std::make_unique<uint8_t[]>(bufferSize);
        Hash hasher{ algorithm };
        DWORD bytesRead = 0;

        while (true)
        {
            THROW_LAST_ERROR_IF(!ReadFile(fileHandle, buffer.get(), bufferSize, &bytesRead, nullptr));
            if (bytesRead == 0)
            {
                break;
            }

            hasher.Add(buffer.get(), bytesRead);
        }

        return hasher.Get();
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
