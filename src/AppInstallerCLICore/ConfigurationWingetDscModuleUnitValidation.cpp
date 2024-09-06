// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationWingetDscModuleUnitValidation.h"
#include "ExecutionContext.h"
#include <winrt/Microsoft.Management.Configuration.h>

using namespace winrt::Microsoft::Management::Configuration;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Configuration
{
    namespace
    {
        constexpr static std::string_view UnitType_WinGetSource = "WinGetSource"sv;
        constexpr static std::string_view UnitType_WinGetPackage = "WinGetPackage"sv;

        constexpr static std::string_view WellKnownSourceName_WinGet = "winget"sv;
        constexpr static std::string_view WellKnownSourceName_MSStore = "msstore"sv;

        constexpr static std::string_view ValueSetKey_TreatAsArray = "treatAsArray"sv;

        constexpr static std::string_view WinGetSourceValueSetKey_Name = "name"sv;
        constexpr static std::string_view WinGetSourceValueSetKey_Type = "type"sv;
        constexpr static std::string_view WinGetSourceValueSetKey_Arg = "argument"sv;
        constexpr static std::string_view WinGetSourceValueSetKey_Ensure = "ensure"sv;
        constexpr static std::string_view WinGetSourceValueSetKey_Ensure_Present = "present"sv;

        constexpr static std::string_view WinGetPackageValueSetKey_Id = "id"sv;
        constexpr static std::string_view WinGetPackageValueSetKey_Version = "version"sv;
        constexpr static std::string_view WinGetPackageValueSetKey_Source = "source"sv;
        constexpr static std::string_view WinGetPackageValueSetKey_UseLatest = "useLatest"sv;

        struct WinGetSource
        {
            std::string Name;
            std::string Type;
            std::string Arg;
            bool Present = true;

            bool Empty()
            {
                return Name.empty() && Arg.empty() && Type.empty();
            }
        };

        std::string GetPropertyValueAsString(const winrt::Windows::Foundation::IInspectable& value)
        {
            IPropertyValue propertyValue = value.try_as<IPropertyValue>();
            if (propertyValue && propertyValue.Type() == PropertyType::String)
            {
                return Utility::ConvertToUTF8(propertyValue.GetString());
            }

            return {};
        }

        bool GetPropertyValueAsBoolean(const winrt::Windows::Foundation::IInspectable& value, bool defaultIfFailed = false)
        {
            IPropertyValue propertyValue = value.try_as<IPropertyValue>();
            if (propertyValue && propertyValue.Type() == PropertyType::Boolean)
            {
                return propertyValue.GetBoolean();
            }

            return defaultIfFailed;
        }

        WinGetSource ParseWinGetSourceFromSettings(const ValueSet& settings)
        {
            WinGetSource result;

            // Iterate through the value set as Powershell variables are case-insensitive.
            for (auto const& settingsPair : settings)
            {
                auto settingsKey = Utility::ConvertToUTF8(settingsPair.Key());

                if (Utility::CaseInsensitiveEquals(WinGetSourceValueSetKey_Name, settingsKey))
                {
                    result.Name = GetPropertyValueAsString(settingsPair.Value());
                }
                else if (Utility::CaseInsensitiveEquals(WinGetSourceValueSetKey_Type, settingsKey))
                {
                    result.Type = GetPropertyValueAsString(settingsPair.Value());
                }
                else if (Utility::CaseInsensitiveEquals(WinGetSourceValueSetKey_Arg, settingsKey))
                {
                    result.Arg = GetPropertyValueAsString(settingsPair.Value());
                }
                else if (Utility::CaseInsensitiveEquals(WinGetSourceValueSetKey_Ensure, settingsKey))
                {
                    result.Present = Utility::CaseInsensitiveEquals(WinGetSourceValueSetKey_Ensure_Present, GetPropertyValueAsString(settingsPair.Value()));
                }
            }

            return result;
        }

        bool IsWellKnownSourceName(std::string_view sourceName)
        {
            return Utility::CaseInsensitiveEquals(WellKnownSourceName_WinGet, sourceName) ||
                Utility::CaseInsensitiveEquals(WellKnownSourceName_MSStore, sourceName);
        }

        bool ValidateWellKnownSource(const WinGetSource& source)
        {
            static std::vector<Repository::SourceDetails> wellKnownSourceDetails =
            {
                Repository::Source{ Repository::WellKnownSource::WinGet }.GetDetails(),
                Repository::Source{ Repository::WellKnownSource::MicrosoftStore }.GetDetails(),
            };

            for (auto const& wellKnownSource : wellKnownSourceDetails)
            {
                if (Utility::CaseInsensitiveEquals(wellKnownSource.Name, source.Name) &&
                    Utility::CaseInsensitiveEquals(wellKnownSource.Arg, source.Arg) &&
                    Utility::CaseInsensitiveEquals(wellKnownSource.Type, source.Type))
                {
                    return true;
                }
            }

            return false;
        }

        struct WinGetPackage
        {
            std::string Id;
            std::string Version;
            std::string Source;
            bool UseLatest = false;

            bool Empty()
            {
                return Id.empty() && Version.empty() && Source.empty();
            }
        };

        WinGetPackage ParseWinGetPackageFromSettings(const ValueSet& settings)
        {
            // Iterate through the value set as Powershell variables are case-insensitive.
            WinGetPackage result;
            for (auto const& settingsPair : settings)
            {
                auto settingsKey = Utility::ConvertToUTF8(settingsPair.Key());
                if (Utility::CaseInsensitiveEquals(WinGetPackageValueSetKey_Id, settingsKey))
                {
                    result.Id = GetPropertyValueAsString(settingsPair.Value());
                }
                else if (Utility::CaseInsensitiveEquals(WinGetPackageValueSetKey_Version, settingsKey))
                {
                    result.Version = GetPropertyValueAsString(settingsPair.Value());
                }
                else if (Utility::CaseInsensitiveEquals(WinGetPackageValueSetKey_Source, settingsKey))
                {
                    result.Source = GetPropertyValueAsString(settingsPair.Value());
                }
                else if (Utility::CaseInsensitiveEquals(WinGetPackageValueSetKey_UseLatest, settingsKey))
                {
                    result.UseLatest = GetPropertyValueAsBoolean(settingsPair.Value());
                }
            }

            return result;
        }
    }

    bool WingetDscModuleUnitValidator::ValidateConfigurationSetUnit(Execution::Context& context, const ConfigurationUnit& unit)
    {
       bool foundIssues = false;
       auto details = unit.Details();
       auto unitType = Utility::ConvertToUTF8(details.UnitType());
       auto unitIntent = unit.Intent();

       if (Utility::CaseInsensitiveEquals(UnitType_WinGetSource, unitType))
       {
           auto source = ParseWinGetSourceFromSettings(unit.Settings());

            // Validate basic semantics.
            if (source.Name.empty())
            {
                AICLI_LOG(Config, Error, << "WinGetSource unit missing required arg: Name");
                context.Reporter.Error() << Resource::String::WinGetResourceUnitMissingRequiredArg(Utility::LocIndView{ UnitType_WinGetSource }, "Name"_liv) << std::endl;
                foundIssues = true;
            }
            if (source.Arg.empty() && source.Present)
            {
                AICLI_LOG(Config, Error, << "WinGetSource unit missing required arg: Argument");
                context.Reporter.Error() << Resource::String::WinGetResourceUnitMissingRequiredArg(Utility::LocIndView{ UnitType_WinGetSource }, "Argument"_liv) << std::endl;
                foundIssues = true;
            }

            // Validate well known source or process 3rd party source.
            if (IsWellKnownSourceName(source.Name))
            {
                if (!ValidateWellKnownSource(source))
                {
                    AICLI_LOG(Config, Warning, << "WinGetSource conflicts with a well known source. Source: " << source.Name);
                    context.Reporter.Warn() << Resource::String::WinGetResourceUnitKnownSourceConfliction(Utility::LocIndView{ source.Name }) << std::endl;
                    foundIssues = true;
                }
            }
            else
            {
                if (unitIntent == ConfigurationUnitIntent::Assert)
                {
                    AICLI_LOG(Config, Warning, << "Asserting on 3rd party source: " << source.Name);
                    context.Reporter.Warn() << Resource::String::WinGetResourceUnitThirdPartySourceAssertion(Utility::LocIndView{ source.Name }) << std::endl;
                    foundIssues = true;
                }
                else if (unitIntent == ConfigurationUnitIntent::Apply)
                {
                    // Add to dependency source map so it can be validated with later WinGetPackage source
                    m_dependenciesSourceAndUnitIdMap.emplace(Utility::FoldCase(std::string_view{ source.Name }), Utility::FoldCase(Utility::NormalizedString{ unit.Identifier() }));
                }
            }
       }
       else if (Utility::CaseInsensitiveEquals(UnitType_WinGetPackage, unitType))
       {
           auto package = ParseWinGetPackageFromSettings(unit.Settings());
           if (package.Empty())
           {
               AICLI_LOG(Config, Warning, << "Failed to parse WinGetPackage or empty content.");
               context.Reporter.Warn() << Resource::String::WinGetResourceUnitEmptyContent(Utility::LocIndView{ UnitType_WinGetPackage }) << std::endl;
               foundIssues = true;
           }
           // Validate basic semantics.
           if (package.Id.empty())
           {
               AICLI_LOG(Config, Error, << "WinGetPackage unit missing required arg: Id");
               context.Reporter.Error() << Resource::String::WinGetResourceUnitMissingRequiredArg(Utility::LocIndView{ UnitType_WinGetPackage }, "Id"_liv) << std::endl;
               foundIssues = true;
           }
           if (package.Source.empty())
           {
               AICLI_LOG(Config, Warning, << "WinGetPackage unit missing recommended arg: Source");
               context.Reporter.Warn() << Resource::String::WinGetResourceUnitMissingRecommendedArg(Utility::LocIndView{ UnitType_WinGetPackage }, "Source"_liv) << std::endl;
               foundIssues = true;
           }
           if (package.UseLatest && !package.Version.empty())
           {
               AICLI_LOG(Config, Warning, << "WinGetPackage unit both UseLatest and Version declared. Package: " << package.Id);
               context.Reporter.Warn() << Resource::String::WinGetResourceUnitBothPackageVersionAndUseLatest(Utility::LocIndView{ package.Id }) << std::endl;
               foundIssues = true;
           }
           // Validate dependency source is configured.
           if (!package.Source.empty() && !IsWellKnownSourceName(package.Source))
           {
               if (unitIntent == ConfigurationUnitIntent::Assert)
               {
                   AICLI_LOG(Config, Warning, << "Asserting on a package that depends on a 3rd party source. Package: " << package.Id << " Source: " << package.Source);
                   context.Reporter.Warn() << Resource::String::WinGetResourceUnitThirdPartySourceAssertionForPackage(Utility::LocIndView{ package.Id }, Utility::LocIndView{ package.Source }) << std::endl;
                   foundIssues = true;
               }
               else
               {
                   auto dependencySourceItr = m_dependenciesSourceAndUnitIdMap.find(Utility::FoldCase(std::string_view{ package.Source }));
                   if (dependencySourceItr == m_dependenciesSourceAndUnitIdMap.end())
                   {
                       AICLI_LOG(Config, Warning, << "WinGetPackage depends on a 3rd party source not previously configured. Package: " << package.Id << " Source: " << package.Source);
                       context.Reporter.Warn() << Resource::String::WinGetResourceUnitDependencySourceNotConfigured(Utility::LocIndView{ package.Id }, Utility::LocIndView{ package.Source }) << std::endl;
                       foundIssues = true;
                   }
                   else
                   {
                       bool foundInUnitDependencies = false;
                       for (auto const& entry : unit.Dependencies())
                       {
                           // The map contains normalized string, so just use direct comparison;
                           if (dependencySourceItr->second == Utility::FoldCase(Utility::NormalizedString{ entry }))
                           {
                               foundInUnitDependencies = true;
                               break;
                           }
                       }
                       if (!foundInUnitDependencies)
                       {
                           AICLI_LOG(Config, Warning, << "WinGetPackage depends on a 3rd party source. It is recommended to add the WinGetSources unit configuring the source to the unit's dependsOn list. Package: " << package.Id << " Source: " << package.Source);
                           context.Reporter.Warn() << Resource::String::WinGetResourceUnitDependencySourceNotDeclaredAsDependency(Utility::LocIndView{ package.Id }, Utility::LocIndView{ package.Source }) << std::endl;
                           foundIssues = true;
                       }
                   }
               }
           }
           // Validate package is found and version available.
           try
           {
               Repository::Source source{ package.Source };
               if (!source)
               {
                   AICLI_LOG(Config, Warning, << "Failed to open WinGet source. Package: " << package.Id << " Source: " << package.Source);
                   context.Reporter.Warn() << Resource::String::WinGetResourceUnitFailedToValidatePackageSourceOpenFailed(Utility::LocIndView{ package.Id }, Utility::LocIndView{ package.Source }) << std::endl;
                   foundIssues = true;
               }
               else
               {
                   source.SetCaller("winget-cli-configuration-unit-module-validation");
                   ProgressCallback empty;
                   source.Open(empty);
                   Repository::SearchRequest searchRequest;
                   searchRequest.Filters.emplace_back(Repository::PackageMatchFilter{ Repository::PackageMatchField::Id, Repository::MatchType::CaseInsensitive, package.Id });
                   auto searchResult = source.Search(searchRequest);
                   if (searchResult.Matches.size() == 0)
                   {
                       AICLI_LOG(Config, Warning, << "WinGetPackage not found: " << package.Id);
                       context.Reporter.Warn() << Resource::String::WinGetResourceUnitFailedToValidatePackageNotFound(Utility::LocIndView{ package.Id }) << std::endl;
                       foundIssues = true;
                   }
                   else if (searchResult.Matches.size() > 1)
                   {
                       AICLI_LOG(Config, Warning, << "More than one WinGetPackage found: " << package.Id);
                       context.Reporter.Warn() << Resource::String::WinGetResourceUnitFailedToValidatePackageMultipleFound(Utility::LocIndView{ package.Id }) << std::endl;
                       foundIssues = true;
                   }
                   else
                   {
                       if (!package.Version.empty())
                       {
                           std::shared_ptr<Repository::IPackage> availablePackage = searchResult.Matches.at(0).Package->GetAvailable().at(0);
                           auto versionKeys = availablePackage->GetVersionKeys();
                           bool foundVersion = false;
                           for (auto const& versionKey : versionKeys)
                           {
                               if (versionKey.Version == Utility::NormalizedString(package.Version))
                               {
                                   foundVersion = true;
                                   break;
                               }
                           }
                           if (!foundVersion)
                           {
                               AICLI_LOG(Config, Warning, << "WinGetPackage version not found. Package: " << package.Id << " Version: " << package.Version);
                               context.Reporter.Warn() << Resource::String::WinGetResourceUnitFailedToValidatePackageVersionNotFound(Utility::LocIndView{ package.Id }, Utility::LocIndView{ package.Version }) << std::endl;
                               foundIssues = true;
                           }
                           if (versionKeys.size() == 1)
                           {
                               AICLI_LOG(Config, Warning, << "WinGetPackage version specified with only one version available: " << package.Id);
                               context.Reporter.Warn() << Resource::String::WinGetResourceUnitPackageVersionSpecifiedWithOnlyOnePackageVersion(Utility::LocIndView{ package.Id }, Utility::LocIndView{ package.Version }) << std::endl;
                               foundIssues = true;
                           }
                       }
                   }
               }
           }
           catch (...)
           {
               AICLI_LOG(Config, Warning, << "Failed to validate WinGetPackage: " << package.Id);
               context.Reporter.Warn() << Resource::String::WinGetResourceUnitFailedToValidatePackage(Utility::LocIndView{ package.Id }) << std::endl;
               foundIssues = true;
           }
       }

       return !foundIssues;
    }
}
