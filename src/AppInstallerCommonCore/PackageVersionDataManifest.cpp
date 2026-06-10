// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PackageVersionDataManifest.h"
#include "Public/winget/Yaml.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>

using namespace std::string_view_literals;

namespace AppInstaller::Manifest
{
    // These shortened names save some bytes since humans are neither authoring them nor reading them (except to debug).
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

    namespace anon
    {
        std::string GetRequiredChildString(const YAML::Node& node, std::string_view childName)
        {
            const YAML::Node& childNode = node.GetChildNode(childName);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !childNode.IsScalar());
            return childNode.as<std::string>();
        }

        std::optional<std::string> GetOptionalChildString(const YAML::Node& node, std::string_view childName)
        {
            const YAML::Node& childNode = node.GetChildNode(childName);
            return childNode.IsScalar() ? std::make_optional(childNode.as<std::string>()) : std::nullopt;
        }

        void Deserialize_1_0(const YAML::Node& document, PackageVersionDataManifest& manifest)
        {
            const YAML::Node& versionDataItems = document.GetChildNode(s_FieldName_VersionData);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !versionDataItems.IsSequence());

            for (const YAML::Node& item : versionDataItems.Sequence())
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !item.IsMap());

                PackageVersionDataManifest::VersionData versionData;

                versionData.Version.Assign(GetRequiredChildString(item, s_FieldName_Version));
                versionData.ArpMinVersion = GetOptionalChildString(item, s_FieldName_ArpMinVersion);
                versionData.ArpMaxVersion = GetOptionalChildString(item, s_FieldName_ArpMaxVersion);
                versionData.ManifestRelativePath = GetRequiredChildString(item, s_FieldName_RelativePath);
                versionData.ManifestHash = GetRequiredChildString(item, s_FieldName_Sha256Hash);

                manifest.AddVersion(std::move(versionData));
            }
        }
    }

    std::string_view PackageVersionDataManifest::VersionManifestFileName()
    {
        return "versionData.yml"sv;
    }

    std::string_view PackageVersionDataManifest::VersionManifestCompressedFileName()
    {
        return "versionData.mszyml"sv;
    }

    std::filesystem::path PackageVersionDataManifest::GetRelativeDirectoryPath(std::string_view packageIdentifier, std::string_view manifestHash)
    {
        std::filesystem::path result = "packages";
        result /= Utility::ConvertToUTF16(packageIdentifier);
        result /= manifestHash.substr(0, 8);
        return result;
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
        ManifestRelativePath(std::move(relativePath).value_or("")),
        ManifestHash(std::move(manifestHash).value_or(""))
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

    const std::vector<PackageVersionDataManifest::VersionData>& PackageVersionDataManifest::Versions() const
    {
        return m_versions;
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

    void PackageVersionDataManifest::Deserialize(std::string_view input)
    {
        AICLI_LOG_LARGE_STRING(Core, Verbose, << "PackageVersionDataManifest deserializing:", input);

        YAML::Node document = YAML::Load(input);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !document.IsMap());

        const YAML::Node& schemaVersionNode = document.GetChildNode(s_FieldName_SchemaVersion);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !schemaVersionNode.IsScalar());

        Utility::Version schemaVersion{ schemaVersionNode.as<std::string>() };

        if (schemaVersion.PartAt(0).Integer == 1)
        {
            anon::Deserialize_1_0(document, *this);
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION);
        }
    }

    void PackageVersionDataManifest::Deserialize(const std::vector<uint8_t>& input)
    {
        Deserialize(std::string_view{ reinterpret_cast<const char*>(input.data()), input.size() });
    }
}
