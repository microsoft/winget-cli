// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Utility {

    enum class HashAlgorithm
    {
        Sha256,
    };

    template <HashAlgorithm Algorithm>
    struct HashAlgorithmTraits;

    template <>
    struct HashAlgorithmTraits<HashAlgorithm::Sha256>
    {
        constexpr static size_t HashBufferSizeInBytes = 32;
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

    // Binds the generic hash implementation to a specific algorithm.
    template <HashAlgorithm Algorithm>
    class AlgorithmHash
    {
    public:
        using HashBuffer = Hash::HashBuffer;
        using HashDetails = Hash::HashDetails;
        constexpr static size_t HashBufferSizeInBytes = HashAlgorithmTraits<Algorithm>::HashBufferSizeInBytes;
        constexpr static size_t HashStringSizeInChars = HashBufferSizeInBytes * 2;

        AlgorithmHash() : m_hash(Algorithm)
        {
        }

        void Add(const uint8_t* buffer, size_t cbBuffer)
        {
            m_hash.Add(buffer, cbBuffer);
        }

        void Add(const std::vector<std::uint8_t>& buffer)
        {
            m_hash.Add(buffer);
        }

        void Get(HashBuffer& hash)
        {
            m_hash.Get(hash);
        }

        HashBuffer Get()
        {
            return m_hash.Get();
        }

        static HashBuffer ComputeHash(const uint8_t* buffer, std::uint32_t cbBuffer)
        {
            return Hash::ComputeHash(Algorithm, buffer, cbBuffer);
        }

        static HashBuffer ComputeHash(const std::vector<uint8_t>& buffer)
        {
            return Hash::ComputeHash(Algorithm, buffer);
        }

        static HashBuffer ComputeHash(std::string_view buffer)
        {
            return Hash::ComputeHash(Algorithm, buffer);
        }

        static HashBuffer ComputeHash(std::istream& in)
        {
            return Hash::ComputeHash(Algorithm, in);
        }

        static HashDetails ComputeHashDetails(std::istream& in)
        {
            return Hash::ComputeHashDetails(Algorithm, in);
        }

        static HashBuffer ComputeHashFromFile(const std::filesystem::path& path)
        {
            return Hash::ComputeHashFromFile(Algorithm, path);
        }

        static HashBuffer ComputeHashFromHandle(HANDLE fileHandle)
        {
            return Hash::ComputeHashFromHandle(Algorithm, fileHandle);
        }

        static std::string ConvertToString(const HashBuffer& hashBuffer)
        {
            return Hash::ConvertToString(hashBuffer, HashBufferSizeInBytes);
        }

        static std::wstring ConvertToWideString(const HashBuffer& hashBuffer)
        {
            return Hash::ConvertToWideString(hashBuffer, HashBufferSizeInBytes);
        }

        static HashBuffer ConvertToBytes(const std::string& hashStr)
        {
            return Hash::ConvertToBytes(hashStr, HashBufferSizeInBytes);
        }

        static HashBuffer ConvertToBytes(const std::wstring& hashStr)
        {
            return Hash::ConvertToBytes(hashStr, HashBufferSizeInBytes);
        }

        static bool AreEqual(const HashBuffer& first, const HashBuffer& second)
        {
            return Hash::AreEqual(first, second);
        }

    private:
        Hash m_hash;
    };
}
