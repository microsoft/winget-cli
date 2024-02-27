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
        constexpr static std::string_view UnitType_WinGetSources = "WinGetSources"sv;
        constexpr static std::string_view UnitType_WinGetPackage = "WinGetPackage"sv;

        constexpr static std::string_view WellKnownSourceName_WinGet = "winget"sv;
        constexpr static std::string_view WellKnownSourceName_MSStore = "msstore"sv;

        constexpr static std::string_view ValueSetKey_TreatAsArray = "treatAsArray"sv;

        constexpr static std::string_view WinGetSourcesValueSetKey_Sources = "sources"sv;
        constexpr static std::string_view WinGetSourcesValueSetKey_SourceName = "name"sv;
        constexpr static std::string_view WinGetSourcesValueSetKey_SourceType = "type"sv;
        constexpr static std::string_view WinGetSourcesValueSetKey_SourceArg = "arg"sv;

        constexpr static std::string_view WinGetPackageValueSetKey_Id = "id"sv;
        constexpr static std::string_view WinGetPackageValueSetKey_Version = "version"sv;
        constexpr static std::string_view WinGetPackageValueSetKey_Source = "source"sv;
        constexpr static std::string_view WinGetPackageValueSetKey_UseLatest = "useLatest"sv;

        struct WinGetSource
        {
            std::string Name;
            std::string Type;
            std::string Arg;

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

        std::vector<WinGetSource> ParseWinGetSourcesFromSettings(const ValueSet& settings)
        {
            // Iterate through the value set as Powershell variables are case insensitive.
            std::vector<WinGetSource> result;
            for (auto const& settingsPair : settings)
            {
                auto settingsKey = Utility::ConvertToUTF8(settingsPair.Key());
                if (Utility::CaseInsensitiveEquals(WinGetSourcesValueSetKey_Sources, settingsKey))
                {
                    auto sources = settingsPair.Value().try_as<ValueSet>();
                    if (!sources)
                    {
                        return {};
                    }
                    bool isArray = false;
                    for (auto const& sourcesPair : sources)
                    {
                        if (Utility::CaseInsensitiveEquals(ValueSetKey_TreatAsArray, Utility::ConvertToUTF8(sourcesPair.Key())))
                        {
                            isArray = true;
                        }
                        else
                        {
                            auto source = sourcesPair.Value().try_as<ValueSet>();
                            if (source)
                            {
                                WinGetSource wingetSource;
                                for (auto const& sourcePair : source)
                                {
                                    auto sourceKey = Utility::ConvertToUTF8(sourcePair.Key());
                                    if (Utility::CaseInsensitiveEquals(WinGetSourcesValueSetKey_SourceName, sourceKey))
                                    {
                                        wingetSource.Name = GetPropertyValueAsString(sourcePair.Value());
                                    }
                                    else if (Utility::CaseInsensitiveEquals(WinGetSourcesValueSetKey_SourceType, sourceKey))
                                    {
                                        wingetSource.Type = GetPropertyValueAsString(sourcePair.Value());
                                    }
                                    else if (Utility::CaseInsensitiveEquals(WinGetSourcesValueSetKey_SourceArg, sourceKey))
                                    {
                                        wingetSource.Arg = GetPropertyValueAsString(sourcePair.Value());
                                    }
                                }

                                if (!wingetSource.Empty())
                                {
                                    result.emplace_back(std::move(wingetSource));
                                }
                            }
                        }
                    }

                    if (!isArray)
                    {
                        return {};
                    }

                    break;
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
            // Iterate through the value set as Powershell variables are case insensitive.
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

       if (Utility::CaseInsensitiveEquals(UnitType_WinGetSources, unitType))
       {
           auto sources = ParseWinGetSourcesFromSettings(unit.Settings());
           if (sources.size() == 0)
           {
               AICLI_LOG(Config, Warning, << "Failed to parse WinGetSources or empty content.");
               context.Reporter.Warn() << Resource::String::WinGetResourceUnitEmptyContent(Utility::LocIndView{ UnitType_WinGetSources }) << std::endl;
               foundIssues = true;
           }
           for (auto const& source : sources)
           {
               // Validate basic semantics.
               if (source.Name.empty())
               {
                   AICLI_LOG(Config, Error, << "WinGetSource unit missing required arg: Name");
                   context.Reporter.Error() << Resource::String::WinGetResourceUnitMissingRequiredArg(Utility::LocIndView{ UnitType_WinGetSources }, "Name"_liv) << std::endl;
                   foundIssues = true;
               }
               if (source.Arg.empty())
               {
                   AICLI_LOG(Config, Error, << "WinGetSource unit missing required arg: Arg");
                   context.Reporter.Error() << Resource::String::WinGetResourceUnitMissingRequiredArg(Utility::LocIndView{ UnitType_WinGetSources }, "Arg"_liv) << std::endl;
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
               AICLI_LOG(Config, Error, << "WinGetPackage unit both UseLatest and Version declared. Package: " << package.Id);
               context.Reporter.Error() << Resource::String::WinGetResourceUnitBothPackageVersionAndUseLatest(Utility::LocIndView{ package.Id }) << std::endl;
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