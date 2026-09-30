// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Commands/HashCommand.h"
#include <winget/Hash.h>
#include <limits>
#include <type_traits>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;

static_assert(std::is_move_constructible_v<SHA256>);
static_assert(std::is_move_assignable_v<SHA256>);
static_assert(!std::is_copy_constructible_v<SHA256>);
static_assert(!std::is_copy_assignable_v<SHA256>);

#if defined(_WIN64)
TEST_CASE("Hash_RejectsOversizedInput", "[Sha256Hash][Cryptography]")
{
    constexpr size_t oversizedAddLength = static_cast<size_t>(std::numeric_limits<ULONG>::max()) + 1;
    uint8_t byte = 0;
    SHA256 hasher;

    REQUIRE_THROWS_HR(hasher.Add(&byte, oversizedAddLength), HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    std::string_view oversizedString{ reinterpret_cast<const char*>(&byte), static_cast<size_t>(std::numeric_limits<uint32_t>::max()) + 1 };
    REQUIRE_THROWS_HR(SHA256::ComputeHash(oversizedString), HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
}
#endif

TEST_CASE("SHA256_HandleReadFailure", "[Sha256Hash][Cryptography]")
{
    REQUIRE_THROWS_HR(SHA256::ComputeHashFromHandle(INVALID_HANDLE_VALUE), HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE));
}

TEST_CASE("SHA256_KnownVectors", "[Sha256Hash][Cryptography]")
{
    REQUIRE(SHA256::ConvertToString(SHA256::ComputeHash("")) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    REQUIRE(SHA256::ConvertToString(SHA256::ComputeHash("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256_Streaming", "[Sha256Hash][Cryptography]")
{
    SHA256 hasher;
    std::string_view first{ "a" };
    std::string_view second{ "b" };
    std::string_view third{ "c" };

    hasher.Add(reinterpret_cast<const uint8_t*>(first.data()), first.size());
    hasher.Add(reinterpret_cast<const uint8_t*>(second.data()), second.size());
    hasher.Add(reinterpret_cast<const uint8_t*>(third.data()), third.size());

    REQUIRE(SHA256::ConvertToString(hasher.Get()) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    REQUIRE_THROWS_HR(hasher.Get(), E_UNEXPECTED);
}

TEST_CASE("SHA256_StreamAndFile", "[Sha256Hash][Cryptography]")
{
    std::istringstream stream{ "abc" };
    SHA256::HashDetails details = SHA256::ComputeHashDetails(stream);

    REQUIRE(details.SizeInBytes == 3);
    REQUIRE(SHA256::ConvertToString(details.Hash) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    TempFile tempFile{ "sha256_test", ".txt" };
    {
        std::ofstream out{ tempFile.GetPath(), std::ios::binary };
        out << "abc";
    }

    REQUIRE(SHA256::ConvertToString(SHA256::ComputeHashFromFile(tempFile.GetPath())) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256_Conversions", "[Sha256Hash][Cryptography]")
{
    std::string hashString = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    SHA256::HashBuffer hashBytes = SHA256::ConvertToBytes(hashString);

    REQUIRE(hashBytes.size() == SHA256::HashBufferSizeInBytes);
    REQUIRE(SHA256::ConvertToString(hashBytes) == hashString);
    REQUIRE(SHA256::ConvertToWideString(hashBytes) == L"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("HashCommandWithTestMsix", "[Sha256Hash][Cryptography]")
{
    std::ostringstream hashOutput;
    Execution::Context context{ hashOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::HashFile, TestDataFile("TestSignedApp.msix").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Msix);
    HashCommand hashCommand({});

    hashCommand.Execute(context);

    REQUIRE(hashOutput.str().find("Sha256: 6a2d3683fa19bf00e58e07d1313d20a5f5735ebbd6a999d33381d28740ee07ea") != std::string::npos);
    REQUIRE(hashOutput.str().find("SignatureSha256: 138781c3e6f635240353f3d14d1d57bdcb89413e49be63b375e6a5d7b93b0d07") != std::string::npos);
}
