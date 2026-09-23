// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Utility {

    enum class HashAlgorithm
    {
        Sha256,
    };

    struct HashContext;

    // Class used to compute hashes over various sets of data.
    // Create one and Add data to it if the data is not all available,
    // or simply call ComputeHash if the data is all in memory.
    class Hash
    {
    public:
        using HashBuffer = std::vector<uint8_t>;

        struct HashDetails
        {
            HashBuffer Hash;
            uint64_t SizeInBytes = 0;
        };

        Hash(HashAlgorithm algorithm);

        // Adds the next chunk of data to the hash.
        void Add(const uint8_t* buffer, size_t cbBuffer);

        inline void Add(const std::vector<std::uint8_t>& buffer)
        {
            Add(buffer.data(), buffer.size());
        }

        // Gets the hash of the data. This is a destructive action; the accumulated hash
        // value will be returned and the object can no longer be used.
        void Get(HashBuffer& hash);

        inline HashBuffer Get()
        {
            HashBuffer result{};
            Get(result);
            return result;
        }

        // Computes the hash of the given buffer immediately.
        static HashBuffer ComputeHash(HashAlgorithm algorithm, const uint8_t* buffer, std::uint32_t cbBuffer);

        // Computes the hash of the given buffer immediately.
        static HashBuffer ComputeHash(HashAlgorithm algorithm, const std::vector<uint8_t>& buffer);

        // Computes the hash of the given string immediately.
        static HashBuffer ComputeHash(HashAlgorithm algorithm, std::string_view buffer);

        // Computes the hash from a given stream.
        static HashBuffer ComputeHash(HashAlgorithm algorithm, std::istream& in);

        // Computes the hash from a given stream.
        static HashDetails ComputeHashDetails(HashAlgorithm algorithm, std::istream& in);

        // Computes the hash from a given file path.
        static HashBuffer ComputeHashFromFile(HashAlgorithm algorithm, const std::filesystem::path& path);

        // Computes the hash from an open file HANDLE by reading sequentially from the current position.
        // The caller retains ownership of the handle.
        static HashBuffer ComputeHashFromHandle(HashAlgorithm algorithm, HANDLE fileHandle);

        static std::string ConvertToString(const HashBuffer& hashBuffer, size_t hashBufferSizeInBytes);

        static std::wstring ConvertToWideString(const HashBuffer& hashBuffer, size_t hashBufferSizeInBytes);

        static HashBuffer ConvertToBytes(const std::string& hashStr, size_t hashBufferSizeInBytes);
        static HashBuffer ConvertToBytes(const std::wstring& hashStr, size_t hashBufferSizeInBytes);

        // Returns a value indicating whether the two hashes are equal.
        static bool AreEqual(const HashBuffer& first, const HashBuffer& second);

    private:
        void EnsureNotFinished() const;

        HashAlgorithm m_algorithm;

        struct HashContextDeleter
        {
            void operator()(HashContext* context);
        };

        std::unique_ptr<HashContext, HashContextDeleter> m_context;
    };
}
