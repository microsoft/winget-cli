// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PackageVersionDataManifest.h"
#include "Public/winget/Yaml.h"

using namespace std::string_view_literals;

namespace AppInstaller::Manifest
{
    static constexpr std::string_view s_FieldName_SchemaVersion = "schemaVersion"sv;
    static constexpr std::string_view s_FieldName_VersionData = "versionData"sv;
    static constexpr std::string_view s_FieldName_Version = "version"sv;
    static constexpr std::string_view s_FieldName_ArpMinVersion = "arpMinVersion"sv;
    static constexpr std::string_view s_FieldName_ArpMaxVersion = "arpMaxVersion"sv;
    static constexpr std::string_view s_FieldName_RelativePath = "relativePath"sv;
    static constexpr std::string_view s_FieldName_Sha256Hash = "sha256Hash"sv;

    static constexpr std::string_view s_SchemaVersion_1_0 = "1.0"sv;

    std::string_view PackageVersionDataManifest::VersionManifestFileName()
    {
        return "versionData.yml"sv;
    }

    PackageVersionDataManifest::VersionData::VersionData(
        const Utility::VersionAndChannel& versionAndChannel,
        std::optional<std::string> arpMinVersion,
        std::optional<std::string> arpMaxVersion,
        std::optional<std::string> relativePath,
        std::optional<std::string> manifestHash) :
        Version(versionAndChannel.GetVersion()),
        ArpMinVersion(arpMinVersion),
        ArpMaxVersion(arpMaxVersion),
        ManifestRelativePath(relativePath.value()),
        ManifestHash(manifestHash.value())
    {}

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
