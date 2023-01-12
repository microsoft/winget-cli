// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#define WIN32_NO_STATUS
#include <bcrypt.h>
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerStrings.h"

using namespace AppInstaller::Runtime;

namespace AppInstaller::Utility {

    struct SHA256Context
    {
        wil::unique_bcrypt_algorithm algHandle;
        wil::unique_bcrypt_hash hashHandle;
        DWORD hashLength = 0;
    };

    SHA256::SHA256() : context(new SHA256Context{})
    {
        BCRYPT_ALG_HANDLE algHandleT{};
        BCRYPT_HASH_HANDLE hashHandleT;
        DWORD resultLength = 0;

        // Open an algorithm handle
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptOpenAlgorithmProvider(
            &algHandleT,                // Alg Handle pointer
            BCRYPT_SHA256_ALGORITHM,    // Cryptographic Algorithm name (null terminated unicode string)
            nullptr,                    // Provider name; if null, the default provider is loaded
            0),                         // Flags
            "failed opening SHA256 algorithm provider");
        context->algHandle.reset(algHandleT);

        // Obtain the length of the hash
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptGetProperty(
            context->algHandle.get(),       // Handle to a CNG object
            BCRYPT_HASH_LENGTH,             // Property name (null terminated unicode string)
            (PBYTE) & (context->hashLength),  // Address of the output buffer which receives the property value
            sizeof(context->hashLength),    // Size of the buffer in bytes
            &resultLength,                  // Number of bytes that were copied into the buffer
            0),                             // Flags
            "failed getting SHA256 hash length");
        
        if (resultLength != sizeof(context->hashLength))
        {
            THROW_HR_MSG(E_UNEXPECTED, "failed getting SHA256 hash length");
        }

        // Create a hash handle
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptCreateHash(
            context->algHandle.get(),   // Handle to an algorithm provider
            &hashHandleT,               // A pointer to a hash handle - can be a hash or hmac object
            nullptr,                    // Pointer to the buffer that receives the hash/hmac object
            0,                          // Size of the buffer in bytes
            nullptr,                    // A pointer to a key to use for the hash or MAC
            0,                          // Size of the key in bytes
            0),                         // Flags
            "failed creating SHA256 hash object");
        context->hashHandle.reset(hashHandleT);
    }

    void SHA256::Add(const uint8_t* buffer, size_t cbBuffer)
    {
        EnsureNotFinished();

        // Add the data
        THROW_IF_NTSTATUS_FAILED_MSG(
            BCryptHashData(context->hashHandle.get(), const_cast<PUCHAR>(buffer), static_cast<ULONG>(cbBuffer), 0),
            "failed adding SHA256 data");
    }

    void SHA256::Get(HashBuffer& hash)
    {
        EnsureNotFinished();

        // Size the hash buffer appropriately
        hash.resize(context->hashLength);

        // Obtain the hash of the message(s) into the hash buffer
        THROW_IF_NTSTATUS_FAILED_MSG(BCryptFinishHash(
            context->hashHandle.get(),  // Handle to the hash or MAC object
            hash.data(),                // A pointer to a buffer that receives the hash or MAC value
            context->hashLength,        // Size of the buffer in bytes
            0),                         // Flags
            "failed getting SHA256 hash");

        context.reset();
    }

    std::string SHA256::ConvertToString(const HashBuffer& hashBuffer)
    {
        return Utility::ConvertToHexString(hashBuffer, HashBufferSizeInBytes);
    }

    std::wstring SHA256::ConvertToWideString(const HashBuffer& hashBuffer)
    {
        return ConvertToUTF16(SHA256::ConvertToString(hashBuffer));
    }

    SHA256::HashBuffer SHA256::ConvertToBytes(const std::string& hashStr)
    {
        return Utility::ParseFromHexString(hashStr, HashBufferSizeInBytes);
    }

    SHA256::HashBuffer SHA256::ComputeHash(const std::uint8_t* buffer, std::uint32_t cbBuffer)
    {
        SHA256 hasher;
        hasher.Add(buffer, cbBuffer);
        return hasher.Get();
    }

    SHA256::HashBuffer SHA256::ComputeHash(std::string_view buffer)
    {
        return ComputeHash(reinterpret_cast<const std::uint8_t*>(buffer.data()), static_cast<std::uint32_t>(buffer.size()));
    }

    SHA256::HashBuffer SHA256::ComputeHash(std::istream& in)
    {
        // Throw exceptions on badbit
        auto excState = in.exceptions();
        auto revertExcState = wil::scope_exit([excState, &in]() { in.exceptions(excState); });
        in.exceptions(std::ios_base::badbit);

        const int bufferSize = 1024 * 1024; // 1MB
        auto buffer = std::make_unique<uint8_t[]>(bufferSize);

        SHA256 hasher;

        while (in.good())
        {
            in.read((char*)(buffer.get()), bufferSize);
            if (in.gcount())
            {
                hasher.Add(buffer.get(), static_cast<size_t>(in.gcount()));
            }
        }

        if (in.eof())
        {
            return hasher.Get();
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_STREAM_READ_FAILURE);
        }
    }


    SHA256::HashBuffer SHA256::ComputeHashFromFile(const std::filesystem::path& path)
    {
        std::ifstream inStream{ path, std::ifstream::binary };
        const Utility::SHA256::HashBuffer& targetFileHash = Utility::SHA256::ComputeHash(inStream);
        inStream.close();
        return targetFileHash;
    }

    void SHA256::SHA256ContextDeleter::operator()(SHA256Context* context)
    {
        delete context;
    }

    bool SHA256::AreEqual(const HashBuffer& first, const HashBuffer& second)
    {
        return (first.size() == second.size() && std::equal(first.begin(), first.end(), second.begin()));
    }

    void SHA256::EnsureNotFinished() const
    {
        if (!context)
        {
            THROW_HR_MSG(E_UNEXPECTED, "The hash is already finished");
        }
    }
}