// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexed/SourceData.h"

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    using namespace std::string_view_literals;

    namespace
    {
        constexpr std::string_view s_SourceData_FullIdentity = "fullIdentity"sv;
        constexpr std::string_view s_SourceData_DeltaIdentity = "deltaIdentity"sv;
        constexpr std::string_view s_SourceData_BaselineIdentity = "baselineIdentity"sv;

        // Reads an optional string member, requiring that it be a string when present.
        std::string ReadStringMember(const Json::Value& root, std::string_view name)
        {
            const Json::Value& member = root[std::string{ name }];

            if (member.isNull())
            {
                return {};
            }

            if (!member.isString())
            {
                AICLI_LOG(Repo, Error, << "Source data member '" << name << "' was not a string");
                THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
            }

            return member.asString();
        }
    }

    SourceData::SourceData(std::string_view data)
    {
        size_t firstNonSpace = data.find_first_not_of(" \t\r\n"sv);

        if (firstNonSpace == std::string_view::npos)
        {
            // An empty value names nothing; the identities are discovered when the source is added.
            return;
        }

        if (data[firstNonSpace] != '{')
        {
            // The original syntax: the whole value is the one identity that the source has.
            m_fullIdentity = data;
            return;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
        std::string error;

        if (!reader->parse(data.data(), data.data() + data.size(), &root, &error))
        {
            AICLI_LOG(Repo, Error, << "Source data was not valid JSON (" << error << "): " << data);
            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
        }

        if (!root.isObject())
        {
            AICLI_LOG(Repo, Error, << "Source data was not a JSON object: " << data);
            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
        }

        // Unrecognized members are ignored, so that a newer client can add to this value without
        // an older one refusing to read the source at all.
        m_fullIdentity = ReadStringMember(root, s_SourceData_FullIdentity);
        m_deltaIdentity = ReadStringMember(root, s_SourceData_DeltaIdentity);
        m_baselineIdentity = ReadStringMember(root, s_SourceData_BaselineIdentity);

        // A value that names no package at all identifies nothing, and every path from here needs
        // something to key the source on.
        if (m_fullIdentity.empty() && m_deltaIdentity.empty())
        {
            AICLI_LOG(Repo, Error, << "Source data did not contain an identity for the source: " << data);
            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
        }
    }

    std::string SourceData::Serialize() const
    {
        if (m_deltaIdentity.empty() && m_baselineIdentity.empty())
        {
            return m_fullIdentity;
        }

        Json::Value root{ Json::objectValue };

        // Only what the source actually has is written, so that the value says what it means
        // rather than carrying empty members that a reader would have to interpret.
        if (!m_fullIdentity.empty())
        {
            root[std::string{ s_SourceData_FullIdentity }] = m_fullIdentity;
        }

        if (!m_deltaIdentity.empty())
        {
            root[std::string{ s_SourceData_DeltaIdentity }] = m_deltaIdentity;
        }

        if (!m_baselineIdentity.empty())
        {
            root[std::string{ s_SourceData_BaselineIdentity }] = m_baselineIdentity;
        }

        Json::StreamWriterBuilder writer;
        // The value is stored as a single line in the sources settings file.
        writer["indentation"] = "";

        return Json::writeString(writer, root);
    }
}
