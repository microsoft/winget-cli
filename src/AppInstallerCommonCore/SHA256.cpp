//
//  Copyright (C) 2017 Microsoft.  All rights reserved.
//  See LICENSE file in the project root for full license information.
//

#include <pch.h>
#define WIN32_NO_STATUS
#include <bcrypt.h>
#include <Public/Exceptions.h>
#include "Public/AppInstallerSHA256.h"

struct unique_hash_handle_deleter {
    void operator()(BCRYPT_HASH_HANDLE h) const {
        BCryptDestroyHash(h);
    };
};

struct unique_alg_handle_deleter {
    void operator()(BCRYPT_ALG_HANDLE h) const {
        BCryptCloseAlgorithmProvider(h, 0);
    };
};

typedef std::unique_ptr<void, unique_alg_handle_deleter> unique_alg_handle;
typedef std::unique_ptr<void, unique_hash_handle_deleter> unique_hash_handle;

namespace AppInstaller::Utility {

    struct SHA256Context
    {
        unique_alg_handle algHandle;
        unique_hash_handle hashHandle;
        DWORD hashLength = 0;
    };

    SHA256::SHA256() : context(new SHA256Context{})
    {
        BCRYPT_HASH_HANDLE hashHandleT;
        DWORD hashLength = 0;
        DWORD resultLength = 0;

        // Open an algorithm handle
        BCRYPT_ALG_HANDLE algHandleT{};
        ThrowStatusIfFailed(BCryptOpenAlgorithmProvider(
            &algHandleT,                // Alg Handle pointer
            BCRYPT_SHA256_ALGORITHM,    // Cryptographic Algorithm name (null terminated unicode string)
            nullptr,                    // Provider name; if null, the default provider is loaded
            0),                         // Flags
            "failed opening SHA256 algorithm provider");
        context->algHandle.reset(algHandleT);

        // Obtain the length of the hash
        ThrowStatusIfFailed(BCryptGetProperty(
            context->algHandle.get(),       // Handle to a CNG object
            BCRYPT_HASH_LENGTH,             // Property name (null terminated unicode string)
            (PBYTE) & (context->hashLength),  // Address of the output buffer which receives the property value
            sizeof(context->hashLength),    // Size of the buffer in bytes
            &resultLength,                  // Number of bytes that were copied into the buffer
            0),                             // Flags
            "failed getting SHA256 hash length");
        
        if (resultLength != sizeof(context->hashLength))
        {
            throw std::runtime_error("failed getting SHA256 hash length");
        }

        // Create a hash handle
        ThrowStatusIfFailed(BCryptCreateHash(
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
        ThrowStatusIfFailed(BCryptHashData(context->hashHandle.get(), const_cast<PUCHAR>(buffer), static_cast<ULONG>(cbBuffer), 0), "failed adding SHA256 data");
    }

    void SHA256::Get(HashBuffer& hash)
    {
        EnsureNotFinished();

        // Size the hash buffer appropriately
        hash.resize(context->hashLength);

        // Obtain the hash of the message(s) into the hash buffer
        ThrowStatusIfFailed(BCryptFinishHash(
            context->hashHandle.get(),  // Handle to the hash or MAC object
            hash.data(),                // A pointer to a buffer that receives the hash or MAC value
            context->hashLength,        // Size of the buffer in bytes
            0),                         // Flags
            "failed getting SHA256 hash");

        context.reset();
    }

    std::string SHA256::GetAsString()
    {
        HashBuffer resultBuffer{};
        Get(resultBuffer);

        char resultStrBuffer[65];

        for (int i = 0; i < resultBuffer.size(); i++)
        {
            sprintf(resultStrBuffer + i * 2, "%02x", resultBuffer[i]);
        }

        resultStrBuffer[64] = '\0';

        return std::string(resultStrBuffer);
    }

    bool SHA256::ComputeHash(std::uint8_t* buffer, std::uint32_t cbBuffer, HashBuffer& hash)
    {
        SHA256 hasher;
        hasher.Add(buffer, cbBuffer);
        hasher.Get(hash);

        return true;
    }

    void SHA256::SHA256ContextDeleter::operator()(SHA256Context* context)
    {
        delete context;
    }
}