// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PackageVersionDataManifest.h"
#include "Public/winget/Yaml.h"

using namespace std::string_view_literals;

namespace AppInstaller::Manifest
{
    static constexpr std::string_view s_FieldName_SchemaVersion = "sV"sv;
    static constexpr std::string_view s_FieldName_VersionData = "vD"sv;
    static constexpr std::string_view s_FieldName_Version = "v"sv;
    static constexpr std::string_view s_FieldName_ArpMinVersion = "aMiV"sv;
    static constexpr std::string_view s_FieldName_ArpMaxVersion = "aMaV"sv;
    static constexpr std::string_view s_FieldName_RelativePath = "rP"sv;
    static constexpr std::string_view s_FieldName_Sha256Hash = "s256H"sv;

    static constexpr std::string_view s_SchemaVersion_1_0 = "1.0"sv;

    static constexpr DWORD CompressionAlgorithm = COMPRESS_ALGORITHM_MSZIP;
    static constexpr bool CompressionSetLevel1 = false;

    std::string_view PackageVersionDataManifest::VersionManifestFileName()
    {
        return "versionData.yml"sv;
    }

    std::string_view PackageVersionDataManifest::VersionManifestCompressedFileName()
    {
        return "versionData.mszyml"sv;
    }

    Compression::Compressor PackageVersionDataManifest::CreateCompressor()
    {
        Compression::Compressor result(CompressionAlgorithm);
        if constexpr (CompressionSetLevel1)
        {
            result.SetInformation(COMPRESS_INFORMATION_CLASS_LEVEL, 1);
        }
        return result;
    }

    Compression::Decompressor PackageVersionDataManifest::CreateDecompressor()
    {
        return Compression::Decompressor(CompressionAlgorithm);
    }

    PackageVersionDataManifest::VersionData::VersionData(
        const Utility::VersionAndChannel& versionAndChannel,
        std::optional<std::string> arpMinVersion,
        std::optional<std::string> arpMaxVersion,
        std::optional<std::string> relativePath,
        std::optional<std::string> manifestHash) :
        Version(versionAndChannel.GetVersion()),
        ArpMinVersion(std::move(arpMinVersion)),
        ArpMaxVersion(std::move(arpMaxVersion)),
        ManifestRelativePath(std::move(relativePath).value()),
        ManifestHash(std::move(manifestHash).value())
    {
        if (ArpMinVersion && ArpMinVersion->empty())
        {
            ArpMinVersion.reset();
        }

        if (ArpMaxVersion && ArpMaxVersion->empty())
        {
            ArpMaxVersion.reset();
        }
    }

    void PackageVersionDataManifest::AddVersion(VersionData&& versionData)
    {
        m_versions.emplace_back(std::move(versionData));
    }

    std::string PackageVersionDataManifest::Serialize()
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << s_FieldName_SchemaVersion << YAML::Value << s_SchemaVersion_1_0;

        out << YAML::Key << s_FieldName_VersionData;
        out << YAML::BeginSeq;

        for (const auto& version : m_versions)
        {
            out << YAML::BeginMap;
            out << YAML::Key << s_FieldName_Version << YAML::Value << version.Version.ToString();
            if (version.ArpMinVersion)
            {
                out << YAML::Key << s_FieldName_ArpMinVersion << YAML::Value << version.ArpMinVersion.value();
            }
            if (version.ArpMaxVersion)
            {
                out << YAML::Key << s_FieldName_ArpMaxVersion << YAML::Value << version.ArpMaxVersion.value();
            }
            out << YAML::Key << s_FieldName_RelativePath << YAML::Value << version.ManifestRelativePath;
            out << YAML::Key << s_FieldName_Sha256Hash << YAML::Value << version.ManifestHash;
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        return out.str();
    }
}
