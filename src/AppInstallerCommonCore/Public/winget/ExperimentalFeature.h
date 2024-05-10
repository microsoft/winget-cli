// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <vector>
#include <string>
#include <type_traits>
#include "AppInstallerStrings.h"

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    struct UserSettings;

    struct ExperimentalFeature
    {
        // To add an experimental feature
        // 1 - add a flag in this enum, before Max
        // 2 - add a setting in Setting enum in UserSettings.h
        // 3 - follow how to add setting instructions
        // 4 - provide implementation in ExperimentalFeature.cpp
        enum class Feature : unsigned
        {
            None = 0x0,
            // Before making DirectMSI non-experimental, it should be part of manifest validation.
            DirectMSI = 0x1,
            Resume = 0x2,
            Configuration03 = 0x4,
            SideBySide = 0x8,
            ConfigureSelfElevation = 0x10,
            StoreDownload = 0x20,
            ConfigureExport = 0x40,
            IndexV2 = 0x80,
            Max, // This MUST always be after all experimental features

            // Features listed after Max will not be shown with the features command
            // This can be used to hide highly experimental features (or these example ones)
            ExperimentalCmd = 0x10000,
            ExperimentalArg = 0x20000,
        };

        using Feature_t = std::underlying_type_t<ExperimentalFeature::Feature>;

        ExperimentalFeature(std::string_view name, std::string_view jsonName, std::string_view link, Feature feature) :
            m_name(name), m_jsonName(jsonName), m_link(link), m_feature(feature) {}

        ~ExperimentalFeature() = default;

        ExperimentalFeature(const ExperimentalFeature&) = default;
        ExperimentalFeature& operator=(const ExperimentalFeature&) = default;

        ExperimentalFeature(ExperimentalFeature&&) = default;
        ExperimentalFeature& operator=(ExperimentalFeature&&) = default;

        static bool IsEnabled(Feature feature);

#ifndef AICLI_DISABLE_TEST_HOOKS
        static bool IsEnabled(Feature feature, const UserSettings& userSettings);
#endif

        static ExperimentalFeature GetFeature(ExperimentalFeature::Feature feature);
        static std::vector<ExperimentalFeature> GetAllFeatures();

        std::string_view Name() const { return m_name; }
        Utility::LocIndView JsonName() const { return m_jsonName; }
        std::string_view Link() const { return m_link; }
        Feature GetFeature() const { return m_feature; }

    private:
        std::string_view m_name;
        Utility::LocIndView m_jsonName;
        std::string_view m_link;
        Feature m_feature;
    };

    inline ExperimentalFeature::Feature operator|(ExperimentalFeature::Feature lhs, ExperimentalFeature::Feature rhs)
    {
        return static_cast<ExperimentalFeature::Feature> (
            static_cast<ExperimentalFeature::Feature_t>(lhs) |
            static_cast<ExperimentalFeature::Feature_t>(rhs));
    }

    inline ExperimentalFeature::Feature& operator|=(ExperimentalFeature::Feature& lhs, ExperimentalFeature::Feature rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline ExperimentalFeature::Feature operator&(ExperimentalFeature::Feature lhs, ExperimentalFeature::Feature rhs)
    {
        return static_cast<ExperimentalFeature::Feature>(
            static_cast<ExperimentalFeature::Feature_t>(lhs) &
            static_cast<ExperimentalFeature::Feature_t>(rhs));
    }

    inline ExperimentalFeature::Feature& operator&=(ExperimentalFeature::Feature& lhs, ExperimentalFeature::Feature rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }
}
