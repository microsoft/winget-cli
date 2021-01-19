// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/NameNormalization.h"
#include "Public/AppInstallerStrings.h"


namespace AppInstaller::Utility
{
    namespace
    {
        struct NameNormalizationResult
        {
            std::string Name;
            Architecture Architecture;
            std::string Locale;
        };

        struct PublisherNormalizationResult
        {
            std::string Publisher;
        };

        // To maintain consistency, changes that result in different output must be done in a new version.
        // This can potentially be ignored (if thought through) when the changes will only increase the
        // number of matches being made, with no impact to existing matches. For instance, removing an
        // arbitrary new processor architecture from names would hopefully only affect existing packages
        // that were not matching properly. Fixing a bug that was causing bad strings to be produced would
        // be impactful, and thus should likely result in a new iteration.
        struct NormalizationInitial
        {
        private:
            static std::string PrepareForValidation(std::string_view value)
            {
                std::string result = FoldCase(value);
                Trim(result);
                return result.substr(0, result.find("@@", 3));
            }

            // If the string is wrapped with some character groups, remove them.
            // Returns true if string was wrapped; false if not.
            static bool Unwrap(std::string& value)
            {
                if (value.length() >= 2)
                {
                    bool unwrap = false;

                    switch (value[0])
                    {
                    case '"':
                        unwrap = value.back() == '"';
                        break;

                    case '(':
                        unwrap = value.back() == ')';
                        break;
                    }

                    if (unwrap)
                    {
                        value = value.substr(1, value.length() - 2);
                        return true;
                    }
                }

                return false;
            }

            // Returns true if the entire string is a match for the regex.
            static bool IsMatch(const std::regex& re, std::string_view value)
            {
                return std::regex_match(value.begin(), value.end(), re);
            }

            // Removes all matches from the input string.
            static bool Remove(const std::regex& re, std::string& input)
            {
                std::string output = std::regex_replace(input, re, std::string{});
                bool result = (output != input);
                input = std::move(output);
                return result;
            }

            // Removes the architecture and returns the value, if any
            Architecture RemoveArchitecture(std::string& value) const
            {
                Architecture result = Architecture::Unknown;

                if (Remove(ArchitectureX32, value) || Remove(Architecture32Bit, value))
                {
                    result = Architecture::X86;
                }
                else if (Remove(ArchitectureX64, value) || Remove(Architecture64Bit, value))
                {
                    result = Architecture::X64;
                }

                return result;
            }

            // Removes all matches for the given regular expressions
            static bool RemoveAll(const std::vector<std::regex*>& regexes, std::string& value)
            {
                bool result = false;

                for (const auto& re : regexes)
                {

                    result = Remove(*re, value) || result;
                }

                return result;
            }

            // Removes all locales and returns the common value, if any
            std::string RemoveLocale(std::string& value) const
            {
                bool localeFound = false;
                std::string result;

                std::string newValue;
                auto newValueInserter = std::back_inserter(newValue);
                auto startItr = value.cbegin();

                std::sregex_iterator begin{ value.begin(), value.end(), Locale };
                std::sregex_iterator end;

                for (auto i = begin; i != end; ++i)
                {
                    const std::smatch& match = *i;

                    // Copy range before match to newValue
                    std::copy(startItr, match[0].first, newValueInserter);
                    startItr = match[0].first;

                    std::string matchStr = match.str();

                    // Ensure that the value is in the locale list
                    auto bound = std::lower_bound(Locales.begin(), Locales.end(), matchStr);

                    if (bound == Locales.end() || *bound != matchStr)
                    {
                        // Match was not a locale in our list, so copy it out
                        std::copy(startItr, match[0].second, newValueInserter);
                        startItr = match[0].second;
                        continue;
                    }
                    else
                    {
                        // Move for next copy operation
                        startItr = match[0].second;
                    }

                    if (!localeFound)
                    {
                        // First/only match, just extract the value
                        result = std::move(matchStr);
                        localeFound = true;
                    }
                    else if (!result.empty())
                    {
                        // For some reason, there are multiple locales listed.
                        // See if they have anything in common.
                        if (result != matchStr)
                        {
                            // Not completely the same (expected), see if they are at least the same language
                            result.erase(result.find('-'));
                            matchStr.erase(matchStr.find('-'));

                            if (result != matchStr)
                            {
                                // Not the same language, abandon having a locale and just clean them
                                result.clear();
                            }
                        }
                    }
                }

                // Copy the last bit of the old string over
                std::copy(startItr, value.cend(), newValueInserter);

                value = std::move(newValue);

                return result;
            }

            // Splits the string based on the regex matches, excluding empty/whitespace strings
            // and any values found in the exclusions.
            static std::vector<std::string> Split(const std::regex& re, const std::string& value, const std::vector<std::string_view>& exclusions, bool stopOnExclusion = false)
            {
                std::vector<std::string> result;

                auto startItr = value.cbegin();

                std::sregex_iterator begin{ value.begin(), value.end(), re };
                std::sregex_iterator end;

                bool stopEarly = false;

                auto MoveIfAllowed = [&](std::string&& input)
                {
                    if (IsEmptyOrWhitespace(input))
                    {
                        return;
                    }

                    auto bound = std::lower_bound(exclusions.begin(), exclusions.end(), input);

                    if (bound != exclusions.end() && *bound == input)
                    {
                        stopEarly = stopOnExclusion;
                        return;
                    }

                    result.emplace_back(std::move(input));
                };

                for (auto i = begin; i != end; ++i)
                {
                    const std::smatch& match = *i;

                    // Deal with range before match
                    MoveIfAllowed(std::string{ startItr, match[0].first });

                    if (stopEarly)
                    {
                        break;
                    }

                    // Deal with match
                    MoveIfAllowed(match.str());

                    if (stopEarly)
                    {
                        break;
                    }

                    startItr = match[0].second;
                }

                return result;
            }

            // Joins all of the given strings into a single value
            static std::string Join(const std::vector<std::string>& values)
            {
                std::string result;

                for (const auto& v : values)
                {
                    result += v;
                }

                return result;
            }

            static constexpr std::regex::flag_type reOptions = std::regex::flag_type::icase | std::regex::flag_type::optimize;

            // Archictecture
            std::regex ArchitectureX32{ R"((X32|X86)(?=\P{Nd}|$)(?:\sEDITION)?)", reOptions };
            std::regex ArchitectureX64{ R"((X64|X86([\p{Pd}\p{Pc}]64))(?=\P{Nd}|$)(?:\sEDITION)?)", reOptions };
            std::regex Architecture32Bit{ R"((?<=^|\P{Nd})(32[\p{Pd}\p{Pc}\p{Z}]?BIT)S?(?:\sEDITION)?)", reOptions };
            std::regex Architecture64Bit{ R"((?<=^|\P{Nd})(64[\p{Pd}\p{Pc}\p{Z}]?BIT)S?(?:\sEDITION)?)", reOptions };

            // Locale
            std::regex Locale{ R"((?<![A-Z])((?:\p{Lu}{2,3}(-(CANS|CYRL|LATN|MONG))?-\p{Lu}{2})(?![A-Z])(?:-VALENCIA)?))", reOptions };

            // Specifically for SAP Business Objects programs
            std::regex SAPPackage{ R"(^(?:[\p{Lu}\p{Nd}]+[\._])+[\p{Lu}\p{Nd}]+(?:-(?:\p{Nd}+\.)+\p{Nd}+)(?:-(?:\p{Lu}{2}(?:_\p{Lu}{2})?|CORE))(?:-(?:\p{Lu}{2}|\p{Nd}{2}))$)", reOptions };

            std::regex VersionDelimited{ R"((?:(?<!\p{L})(?:V|VER|VERSI(?:O|Ó)N|VERSÃO|VERSIE|WERSJA|BUILD|RELEASE|RC|SP)\P{L}?)?\p{Nd}+(?:[\p{Po}\p{Pd}\p{Pc}]\p{Nd}?(?:RC|B|A|R|SP|K)?\p{Nd}+)+(?:[\p{Po}\p{Pd}\p{Pc}]?\p{Lu}(?:\P{Lu}|$))?)", reOptions };
            std::regex Version{ R"((FOR\s)?(?<!\p{L})(?:P|V|R|VER|VERSI(?:O|Ó)N|VERSÃO|VERSIE|WERSJA|BUILD|RELEASE|RC|SP)(?:\P{L}|\P{L}\p{L})?[\p{Nd}\.]+(?:RC|B|A|R|V|SP)?\p{Nd}?)", reOptions };
            std::regex VersionLetter{ R"((?<!\p{L})(?:(?:V|VER|VERSI(?:O|Ó)N|VERSÃO|VERSIE|WERSJA|BUILD|RELEASE|RC|SP)\P{L})?\p{Lu}\p{Nd}+(?:[\p{Po}\p{Pd}\p{Pc}]\p{Nd}+)+)", reOptions };
            std::regex NonNestedBracket{ R"(\([^\(\)]*\)|\[[^\[\]]*\])", reOptions }; // remove things in parentheses, if there aren't parentheses nested inside
            std::regex BracketEnclosed{ R"((?:\p{Ps}.*\p{Pe}|".*"))", reOptions }; // Impossible to properly handle nested parens with regex
            std::regex LeadingNonLetters{ R"(^\P{L}+)", reOptions }; // remove non-letters at the beginning
            std::regex TrailingNonLetters{ R"(\P{L}+$)", reOptions }; // remove non-letters at the end
            std::regex PrefixParens{ R"(^\(.*?\))", reOptions }; // remove things in parentheses at the front of program names
            std::regex EmptyParens{ R"((\(\s*\)|\[\s*\]|"\s*"))", reOptions }; // remove appearances of (), [], and "", with any number of spaces within
            std::regex EN{ R"(\sEN\s*$)", reOptions }; // remove appearances of EN (represents English language) at the ends of program names
            std::regex TrailingSymbols{ R"([^\p{L}\p{Nd}]+$)", reOptions }; // remove all non-letter/numbers
            std::regex FilePath{ R"(((INSTALLED\sAT|IN)\s)?[CDEF]:\\(.+?\\)*[^\s]*\\?)", reOptions }; // remove file paths
            std::regex FilePathGHS{ R"(\(CHANGE #\d{1,2} TO [CDEF]:\\(.+?\\)*[^\s]*\\?\))", reOptions }; // remove file paths in certain Green Hills Software program names
            std::regex FilePathParens{ R"(\([CDEF]:\\(.+?\\)*[^\s]*\\?\))", reOptions }; // remove file paths within parentheses
            std::regex FilePathQuotes{ R"("[CDEF]:\\(.+?\\)*[^\s]*\\?")", reOptions }; // remove file paths within quotes
            std::regex Roblox{ R"((?<=^ROBLOX\s(PLAYER|STUDIO))(\sFOR\s.*))", reOptions }; // for Roblox programs
            std::regex Bomgar{ R"((?<=^BOMGAR\s(JUMP CLIENT|(ACCESS|REPRESENTATIVE) CONSOLE|BUTTON)|^EMBEDDED CALLBACK)(\s.*))", reOptions }; // for Bomgar programs
            std::regex AcronymSeparators{ R"((?:(?<=^\p{L})|(?<=\P{L}\p{L}))(?:\.|/)(?=\p{L}(?:\P{L}|$)))", reOptions };
            std::regex NonLetters{ R"((?<=^|\s)[^\p{L}]+(?=\s|$))", reOptions }; // remove all non-letters not attached to 
            std::regex ProgramNameSplit{ R"([^\p{L}\p{Nd}\+\&])", reOptions }; // used to separate 'words' in program names
            std::regex PublisherNameSplit{ R"([^\p{L}\p{Nd}])", reOptions }; // used to separate 'words' in publisher names

            const std::vector<std::regex*> ProgramNameRegexes
            {
                &Roblox,
                &Bomgar,
                &PrefixParens,
                &EmptyParens,
                &FilePathGHS,
                &FilePathParens,
                &FilePathQuotes,
                &FilePath,
                &VersionLetter,
                &VersionDelimited,
                &Version,
                &EN,
                &EmptyParens,
                &NonNestedBracket,
                &BracketEnclosed,
                &LeadingNonLetters,
                &TrailingSymbols
            };

            const std::vector<std::regex*> PublisherNameRegexes
            {
                &VersionDelimited,
                &Version,
                &NonNestedBracket,
                &BracketEnclosed,
                &NonLetters,
                &TrailingNonLetters,
                &AcronymSeparators
            };

            // Must be in sorted order so that search can be efficient
            const std::vector<std::string_view> Locales
            {
                "AF-ZA", "AM-ET", "AR-AE", "AR-BH", "AR-DZ", "AR-EG", "AR-IQ", "AR-JO", "AR-KW", "AR-LB", "AR-LY",
                "AR-MA", "ARN-CL", "AR-OM", "AR-QA", "AR-SA", "AR-SY", "AR-TN", "AR-YE", "AS-IN", "BA-RU", "BE-BY",
                "BG-BG", "BN-BD", "BN-IN", "BO-CN", "BR-FR", "CA-ES", "CA-ES-VALENCIA",
                "CO-FR", "CS-CZ", "CY-GB", "DA-DK", "DE-AT",
                "DE-CH", "DE-DE", "DE-LI", "DE-LU", "DSB-DE", "DV-MV", "EL-GR", "EN-AU", "EN-BZ", "EN-CA", "EN-GB",
                "EN-IE", "EN-IN", "EN-JM", "EN-MY", "EN-NZ", "EN-PH", "EN-SG", "EN-TT", "EN-US", "EN-ZA", "EN-ZW",
                "ES-AR", "ES-BO", "ES-CL", "ES-CO", "ES-CR", "ES-DO", "ES-EC", "ES-ES", "ES-GT", "ES-HN", "ES-MX",
                "ES-NI", "ES-PA", "ES-PE", "ES-PR", "ES-PY", "ES-SV", "ES-US", "ES-UY", "ES-VE", "ET-EE", "EU-ES",
                "FA-IR", "FI-FI", "FIL-PH", "FO-FO", "FR-BE", "FR-CA", "FR-CH", "FR-FR", "FR-LU", "FR-MC", "FY-NL",
                "GA-IE", "GD-DB", "GL-ES", "GSW-FR", "GU-IN", "HE-IL", "HI-IN", "HR-BA", "HR-HR", "HSB-DE", "HU-HU",
                "HY-AM", "ID-ID", "IG-NG", "II-CN", "IS-IS", "IT-CH", "IT-IT", "JA-JP", "KA-GE", "KK-KZ", "KL-GL",
                "KM-KH", "KN-IN", "KOK-IN", "KO-KR", "KY-KG", "LB-LU", "LO-LA", "LT-LT", "LV-LV", "MI-NZ", "MK-MK",
                "ML-IN", "MN-MN", "MOH-CA", "MR-IN", "MS-BN", "MS-MY", "MT-MT", "NB-NO", "NE-NP", "NL-BE", "NL-NL",
                "NN-NO", "NSO-ZA", "OC-FR", "OR-IN", "PA-IN", "PL-PL", "PRS-AF", "PS-AF", "PT-BR", "PT-PT", "QUT-GT",
                "QUZ-BO", "QUZ-EC", "QUZ-PE", "RM-CH", "RO-RO", "RU-RU", "RW-RW", "SAH-RU", "SA-IN", "SE-FI", "SE-NO",
                "SE-SE", "SI-LK", "SK-SK", "SL-SI", "SMA-NO", "SMA-SE", "SMJ-NO", "SMJ-SE", "SMN-FI", "SMS-FI", "SQ-AL",
                "SV-FI", "SV-SE", "SW-KE", "SYR-SY", "TA-IN", "TE-IN", "TH-TH", "TK-TM", "TN-ZA", "TR-TR", "TT-RU",
                "UG-CN", "UK-UA", "UR-PK", "VI-VN", "WO-SN", "XH-ZA", "YO-NG", "ZH-CN", "ZH-HK", "ZH-MO", "ZH-SG",
                "ZH-TW", "ZU-ZA", "AZ-CYRL-AZ", "AZ-LATN-AZ", "BS-CYRL-BA", "BS-LATN-BA", "HA-LATN-NG", "IU-CANS-CA",
                "IU-LATN-CA", "MN-MONG-CN", "SR-CYRL-BA", "SR-CYRL-CS", "SR-CYRL-ME", "SR-CYRL-RS", "SR-LATN-BA",
                "SR-LATN-CS", "SR-LATN-ME", "SR-LATN-RS", "TG-CYRL-TJ", "TZM-LATN-DZ", "UZ-CYRL-UZ", "UZ-LATN-UZ",
            };

            // Must be in sorted order so that search can be efficient
            const std::vector<std::string_view> LegalEntitySuffixes
            {
                // Acronyms
                "AB", "AD", "AG", "APS", "AS", "ASA", "BV", "CO", "CV", "DOO", "GES", "GESMBH", "GMBH", "INC", "KG",
                "KS", "PS", "LLC", "LP", "LTD", "LTDA", "MBH", "NV", "PLC", "SL", "PTY", "PVT", "SA", "SARL",
                "SC", "SCA", "SL", "SP", "SPA", "SRL", "SRO",

                // Words
                "COMPANY", "CORP", "CORPORATION", "HOLDING", "HOLDINGS", "INCORPORATED", "LIMITED", "SUBSIDIARY"
            };

        public:
            NormalizationInitial() = default;

            NameNormalizationResult NormalizeName(std::string_view name)
            {
                NameNormalizationResult result;
                result.Name = PrepareForValidation(name);
                while (Unwrap(result.Name)); // remove wrappers

                // handle (large majority of) SAP Business Object programs
                if (IsMatch(SAPPackage, result.Name))
                {
                    return result;
                }

                result.Architecture = RemoveArchitecture(result.Name);

                // First pass to remove the majority
                RemoveAll(ProgramNameRegexes, result.Name);

                result.Locale = RemoveLocale(result.Name);

                // Repeatedly remove matches for the regexes to create the minimum name
                while (RemoveAll(ProgramNameRegexes, result.Name));

                auto tokens = Split(ProgramNameSplit, result.Name, LegalEntitySuffixes);
                result.Name = Join(tokens);

                return result;
            }

            PublisherNormalizationResult NormalizePublisher(std::string_view publisher)
            {
                PublisherNormalizationResult result;

                result.Publisher = PrepareForValidation(publisher);

                while (RemoveAll(PublisherNameRegexes, result.Publisher));

                auto tokens = Split(PublisherNameSplit, result.Publisher, LegalEntitySuffixes, true);
                result.Publisher = Join(tokens);

                return result;
            }
        };
    }

    NormalizedName NormalizedName::Create(std::string_view name, std::string_view publisher, NormalizationVersion version)
    {
        NameNormalizationResult nameResult;
        PublisherNormalizationResult publisherResult;

        switch (version)
        {
        case AppInstaller::Utility::NormalizationVersion::Initial:
        {
            NormalizationInitial norm;
            nameResult = norm.NormalizeName(name);
            publisherResult = norm.NormalizePublisher(publisher);
        }
            break;
        }

        NormalizedName result;
        result.m_name = std::move(nameResult.Name);
        result.m_arch = nameResult.Architecture;
        result.m_locale = std::move(nameResult.Locale);
        result.m_publisher = std::move(publisherResult.Publisher);

        return result;
    }
}
