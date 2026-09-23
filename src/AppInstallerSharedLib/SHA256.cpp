// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#include "Public/AppInstallerSHA256.h"

namespace AppInstaller::Utility {

    SHA256::SHA256() : m_hash(HashAlgorithm::Sha256)
    {
    }

    void SHA256::Add(const uint8_t* buffer, size_t cbBuffer)
    {
        m_hash.Add(buffer, cbBuffer);
    }

    void SHA256::Get(HashBuffer& hash)
    {
        m_hash.Get(hash);
    }

    std::string SHA256::ConvertToString(const HashBuffer& hashBuffer)
    {
        return Hash::ConvertToString(hashBuffer, HashBufferSizeInBytes);
    }

    std::wstring SHA256::ConvertToWideString(const HashBuffer& hashBuffer)
    {
        return Hash::ConvertToWideString(hashBuffer, HashBufferSizeInBytes);
    }

    SHA256::HashBuffer SHA256::ConvertToBytes(const std::string& hashStr)
    {
        return Hash::ConvertToBytes(hashStr, HashBufferSizeInBytes);
    }

    SHA256::HashBuffer SHA256::ConvertToBytes(const std::wstring& hashStr)
    {
        return Hash::ConvertToBytes(hashStr, HashBufferSizeInBytes);
    }

    SHA256::HashBuffer SHA256::ComputeHash(const std::uint8_t* buffer, std::uint32_t cbBuffer)
    {
        return Hash::ComputeHash(HashAlgorithm::Sha256, buffer, cbBuffer);
    }

    SHA256::HashBuffer SHA256::ComputeHash(const std::vector<uint8_t>& buffer)
    {
        return Hash::ComputeHash(HashAlgorithm::Sha256, buffer);
    }

    SHA256::HashBuffer SHA256::ComputeHash(std::string_view buffer)
    {
        return Hash::ComputeHash(HashAlgorithm::Sha256, buffer);
    }

    SHA256::HashBuffer SHA256::ComputeHash(std::istream& in)
    {
        return Hash::ComputeHash(HashAlgorithm::Sha256, in);
    }

    SHA256::HashDetails SHA256::ComputeHashDetails(std::istream& in)
    {
        return Hash::ComputeHashDetails(HashAlgorithm::Sha256, in);
    }

    SHA256::HashBuffer SHA256::ComputeHashFromFile(const std::filesystem::path& path)
    {
        return Hash::ComputeHashFromFile(HashAlgorithm::Sha256, path);
    }

    SHA256::HashBuffer SHA256::ComputeHashFromHandle(HANDLE fileHandle)
    {
        return Hash::ComputeHashFromHandle(HashAlgorithm::Sha256, fileHandle);
    }

    bool SHA256::AreEqual(const HashBuffer& first, const HashBuffer& second)
    {
        return Hash::AreEqual(first, second);
    }
}
