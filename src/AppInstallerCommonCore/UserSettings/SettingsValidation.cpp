#pragma once
#include "pch.h"
#include "SettingValidation.h"

namespace AppInstaller::Settings
{
    std::vector<std::string> ValidateAndProcessFields(
        const Json::Value& root,
        const std::vector<SettingFieldInfo> fieldInfos)
    {
        if (root.isNull())
        {
            return {};
        }

        std::vector<std::string> warnings;

        for (const std::string& key : root.getMemberNames()) {

            auto fieldIter = std::find_if(fieldInfos.begin(), fieldInfos.end(), [&](auto const& s) { return s.Name == key; });

            if (fieldIter != fieldInfos.end())
            {
                fieldIter->ProcessFunc(key, root);
            }
            else
            {
                // TODO: Localize
                std::string warning = "Unrecognized field " + key;
                warnings.emplace_back(warning);
            }
        }

        return warnings;
    }
}