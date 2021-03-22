// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/NameNormalization.h"
#include "Public/AppInstallerStrings.h"
#include "Public/winget/Regex.h"


namespace AppInstaller::Utility
{
    namespace
    {
        struct InterimNameNormalizationResult
        {
            std::wstring Name;
            Architecture Architecture;
            std::wstring Locale;
        };

        struct InterimPublisherNormalizationResult
        {
            std::wstring Publisher;
        };

        // To maintain consistency, changes that result in different output must be done in a new version.
        // This can potentially be ignored (if thought through) when the changes will only increase the
        // number of matches being made, with no impact to existing matches. For instance, removing an
        // arbitrary new processor architecture from names would hopefully only affect existing packages
        // that were not matching properly. Fixing a bug that was causing bad strings to be produced would
        // be impactful, and thus should likely result in a new iteration.
        class NormalizationInitial : public details::INameNormalizer
        {
            static std::wstring PrepareForValidation(std::string_view value)
            {
                std::wstring result = Utility::Normalize(ConvertToUTF16(value));
                Trim(result);
                size_t atPos = result.find(L"@@", 3);
                if (atPos != std::wstring::npos)
                {
                    result = result.substr(0, atPos);
                }
                return result;
            }

            // If the string is wrapped with some character groups, remove them.
            // Returns true if string was wrapped; false if not.
            static bool Unwrap(std::wstring& value)
            {
                if (value.length() >= 2)
                {
                    bool unwrap = false;

                    switch (value[0])
                    {
                    case L'"':
                        unwrap = value.back() == L'"';
                        break;

                    case L'(':
                        unwrap = value.back() == L')';
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

            // Removes all matches from the input string.
            static bool Remove(const Regex::Expression& re, std::wstring& input)
            {
                std::wstring output = re.Replace(input, {});
                bool result = (output != input);
                input = std::move(output);
                return result;
            }

            // Removes the architecture and returns the value, if any
            Architecture RemoveArchitecture(std::wstring& value) const
            {
                Architecture result = Architecture::Unknown;

                // Must detect this first because "32/64 bit" is a superstring of "64 bit"
                if (Remove(Architecture32Or64Bit, value))
                {
                    // If the program is 32 and 64 bit in the same installer, leave as unknown.
                }
                // Must detect 64 bit before 32 bit because of "x86-64" being a superstring of "x86"
                else if (Remove(ArchitectureX64, value) || Remove(Architecture64Bit, value))
                {
                    result = Architecture::X64;
                }
                else if (Remove(ArchitectureX32, value) || Remove(Architecture32Bit, value))
                {
                    result = Architecture::X86;
                }

                return result;
            }

            // Removes all matches for the given regular expressions
            static bool RemoveAll(const std::vector<Regex::Expression*>& regexes, std::wstring& value)
            {
                bool result = false;

                for (const auto& re : regexes)
                {
                    result = Remove(*re, value) || result;
                }

                return result;
            }

            // Removes all locales and returns the common value, if any
            std::wstring RemoveLocale(std::wstring& value) const
            {
                bool localeFound = false;
                std::wstring result;

                std::wstring newValue;
                auto newValueInserter = std::back_inserter(newValue);

                Locale.ForEach(value,
                    [&](bool isMatch, std::wstring_view text)
                    {
                        bool copy = !isMatch;

                        if (isMatch)
                        {
                            std::wstring foldedText = ConvertToUTF16(FoldCase(text));

                            // Ensure that the value is in the locale list
                            auto bound = std::lower_bound(Locales.begin(), Locales.end(), foldedText);

                            if (bound == Locales.end() || *bound != foldedText)
                            {
                                // Match was not a locale in our list, so copy it out
                                copy = true;
                            }
                            else if (!localeFound)
                            {
                                // First/only match, just extract the value
                                result = foldedText;
                                localeFound = true;
                            }
                            else if (!result.empty())
                            {
                                // For some reason, there are multiple locales listed.
                                // See if they have anything in common.
                                if (result != foldedText)
                                {
                                    // Not completely the same (expected), see if they are at least the same language
                                    result.erase(result.find(L'-'));
                                    foldedText.erase(foldedText.find(L'-'));

                                    if (result != foldedText)
                                    {
                                        // Not the same language, abandon having a locale and just clean them
                                        result.clear();
                                    }
                                }
                            }
                        }

                        if (copy)
                        {
                            std::copy(text.begin(), text.end(), newValueInserter);
                        }

                        return true;
                    });

                value = std::move(newValue);

                return result;
            }

            // Splits the string based on the regex matches, excluding empty/whitespace strings
            // and any values found in the exclusions.
            static std::vector<std::wstring> Split(const Regex::Expression& re, const std::wstring& value, const std::vector<std::wstring>& exclusions, bool stopOnExclusion = false)
            {
                std::vector<std::wstring> result;

                re.ForEach(value,
                    [&](bool, std::wstring_view text)
                    {
                        if (IsEmptyOrWhitespace(text))
                        {
                            return true;
                        }

                        // Do not stop for an exclusion if it is the first word found
                        if (!result.empty())
                        {
                            std::wstring foldedText = ConvertToUTF16(FoldCase(text));

                            auto bound = std::lower_bound(exclusions.begin(), exclusions.end(), foldedText);

                            if (bound != exclusions.end() && *bound == foldedText)
                            {
                                return !stopOnExclusion;
                            }
                        }

                        result.emplace_back(std::wstring{ text });
                        return true;
                    });

                return result;
            }

            // Joins all of the given strings into a single value
            static std::wstring Join(const std::vector<std::wstring>& values)
            {
                std::wstring result;

                for (const auto& v : values)
                {
                    result += v;
                }

                return result;
            }

            static constexpr Regex::Options reOptions = Regex::Options::CaseInsensitive;

            // Architecture
            Regex::Expression ArchitectureX32{ R"((?<=^|[^\p{L}\p{Nd}])(X32|X86)(?=\P{Nd}|$)(?:\sEDITION)?)", reOptions };
            Regex::Expression ArchitectureX64{ R"((?<=^|[^\p{L}\p{Nd}])(X64|AMD64|X86([\p{Pd}\p{Pc}]64))(?=\P{Nd}|$)(?:\sEDITION)?)", reOptions };
            Regex::Expression Architecture32Bit{ R"((?<=^|[^\p{L}\p{Nd}])(32[\p{Pd}\p{Pc}\p{Z}]?BIT)S?(?:\sEDITION)?)", reOptions };
            Regex::Expression Architecture64Bit{ R"((?<=^|[^\p{L}\p{Nd}])(64[\p{Pd}\p{Pc}\p{Z}]?BIT)S?(?:\sEDITION)?)", reOptions };
            Regex::Expression Architecture32Or64Bit{ R"((?<=^|[^\p{L}\p{Nd}])((64[\\\/]32|32[\\\/]64)[\p{Pd}\p{Pc}\p{Z}]?BIT)S?(?:\sEDITION)?)", reOptions };

            // Locale
            Regex::Expression Locale{ R"((?<![A-Z])((?:\p{Lu}{2,3}(-(CANS|CYRL|LATN|MONG))?-\p{Lu}{2})(?![A-Z])(?:-VALENCIA)?))", reOptions };

            // Specifically for SAP Business Objects programs
            Regex::Expression SAPPackage{ R"(^(?:[\p{Lu}\p{Nd}]+[\._])+[\p{Lu}\p{Nd}]+(?:-(?:\p{Nd}+\.)+\p{Nd}+)(?:-(?:\p{Lu}{2}(?:_\p{Lu}{2})?|CORE))(?:-(?:\p{Lu}{2}|\p{Nd}{2}))$)", reOptions };

            // Extract KB numbers from their parens to preserve them
            Regex::Expression KBNumbers{ R"(\((KB\d+)\))", reOptions };

            Regex::Expression NonLettersAndDigits{ R"([^\p{L}\p{Nd}])", reOptions };
            Regex::Expression URIProtocol{ R"((?<!\p{L})(?:http[s]?|ftp):\/\/)", reOptions }; // remove protocol from URIs

            Regex::Expression VersionDelimited{ R"(((?<!\p{L})(?:V|VER|VERSI(?:O|Ó)N|VERSÃO|VERSIE|WERSJA|BUILD|RELEASE|RC|SP)\P{L}?)?\p{Nd}+([\p{Po}\p{Pd}\p{Pc}]\p{Nd}?(RC|B|A|R|SP|K)?\p{Nd}+)+([\p{Po}\p{Pd}\p{Pc}]?[\p{L}\p{Nd}]+)*)", reOptions };
            Regex::Expression Version{ R"((FOR\s)?(?<!\p{L})(?:P|V|R|VER|VERSI(?:O|Ó)N|VERSÃO|VERSIE|WERSJA|BUILD|RELEASE|RC|SP)(?:\P{L}|\P{L}\p{L})?(\p{Nd}|\.\p{Nd})+(?:RC|B|A|R|V|SP)?\p{Nd}?)", reOptions };
            Regex::Expression VersionLetter{ R"((?<!\p{L})(?:(?:V|VER|VERSI(?:O|Ó)N|VERSÃO|VERSIE|WERSJA|BUILD|RELEASE|RC|SP)\P{L})?\p{Lu}\p{Nd}+(?:[\p{Po}\p{Pd}\p{Pc}]\p{Nd}+)+)", reOptions };
            Regex::Expression NonNestedBracket{ R"(\([^\(\)]*\)|\[[^\[\]]*\])", reOptions }; // remove things in parentheses, if there aren't parentheses nested inside
            Regex::Expression BracketEnclosed{ R"((?:\p{Ps}.*\p{Pe}|".*"))", reOptions }; // Impossible to properly handle nested parens with regex
            Regex::Expression LeadingSymbols{ R"(^[^\p{L}\p{Nd}]+)", reOptions }; // remove symbols at the beginning
            Regex::Expression TrailingNonLetters{ R"(\P{L}+$)", reOptions }; // remove non-letters at the end
            Regex::Expression PrefixParens{ R"(^\(.*?\))", reOptions }; // remove things in parentheses at the front of program names
            Regex::Expression EmptyParens{ R"((\(\s*\)|\[\s*\]|"\s*"))", reOptions }; // remove appearances of (), [], and "", with any number of spaces within
            Regex::Expression EN{ R"(\sEN\s*$)", reOptions }; // remove appearances of EN (represents English language) at the ends of program names
            Regex::Expression TrailingSymbols{ R"([^\p{L}\p{Nd}]+$)", reOptions }; // remove all non-letter/numbers at the end
            Regex::Expression FilePath{ R"(((INSTALLED\sAT|IN)\s)?[CDEF]:\\(.+?\\)*[^\s]*\\?)", reOptions }; // remove file paths
            Regex::Expression FilePathGHS{ R"(\(CHANGE #\d{1,2} TO [CDEF]:\\(.+?\\)*[^\s]*\\?\))", reOptions }; // remove file paths in certain Green Hills Software program names
            Regex::Expression FilePathParens{ R"(\([CDEF]:\\(.+?\\)*[^\s]*\\?\))", reOptions }; // remove file paths within parentheses
            Regex::Expression FilePathQuotes{ R"("[CDEF]:\\(.+?\\)*[^\s]*\\?")", reOptions }; // remove file paths within quotes
            Regex::Expression Roblox{ R"((?<=^ROBLOX\s(PLAYER|STUDIO))(\sFOR\s.*))", reOptions }; // for Roblox programs
            Regex::Expression Bomgar{ R"((?<=^BOMGAR\s(JUMP CLIENT|(ACCESS|REPRESENTATIVE) CONSOLE|BUTTON)|^EMBEDDED CALLBACK)(\s.*))", reOptions }; // for Bomgar programs
            Regex::Expression AcronymSeparators{ R"((?:(?<=^\p{L})|(?<=\P{L}\p{L}))(\.|\/)(?=\p{L}(?:\P{L}|$)))", reOptions };
            Regex::Expression NonLetters{ R"((?<=^|\s)[^\p{L}]+(?=\s|$))", reOptions }; // remove all non-letters not attached to 
            Regex::Expression ProgramNameSplit{ R"([^\p{L}\p{Nd}\+\&])", reOptions }; // used to separate 'words' in program names
            Regex::Expression PublisherNameSplit{ R"([^\p{L}\p{Nd}])", reOptions }; // used to separate 'words' in publisher names

            const std::vector<Regex::Expression*> ProgramNameRegexes
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
                &NonNestedBracket,
                &BracketEnclosed,
                &URIProtocol,
                &LeadingSymbols,
                &TrailingSymbols
            };

            const std::vector<Regex::Expression*> PublisherNameRegexes
            {
                &VersionDelimited,
                &Version,
                &NonNestedBracket,
                &BracketEnclosed,
                &URIProtocol,
                &NonLetters,
                &TrailingNonLetters,
                &AcronymSeparators
            };

            // Add values here but use Locales in code.
            const std::vector<std::wstring_view> LocaleViews
            {
                L"AF-ZA", L"AM-ET", L"AR-AE", L"AR-BH", L"AR-DZ", L"AR-EG", L"AR-IQ", L"AR-JO", L"AR-KW", L"AR-LB", L"AR-LY",
                L"AR-MA", L"ARN-CL", L"AR-OM", L"AR-QA", L"AR-SA", L"AR-SY", L"AR-TN", L"AR-YE", L"AS-IN", L"BA-RU", L"BE-BY",
                L"BG-BG", L"BN-BD", L"BN-IN", L"BO-CN", L"BR-FR", L"CA-ES", L"CA-ES-VALENCIA",
                L"CO-FR", L"CS-CZ", L"CY-GB", L"DA-DK", L"DE-AT",
                L"DE-CH", L"DE-DE", L"DE-LI", L"DE-LU", L"DSB-DE", L"DV-MV", L"EL-GR", L"EN-AU", L"EN-BZ", L"EN-CA", L"EN-GB",
                L"EN-IE", L"EN-IN", L"EN-JM", L"EN-MY", L"EN-NZ", L"EN-PH", L"EN-SG", L"EN-TT", L"EN-US", L"EN-ZA", L"EN-ZW",
                L"ES-AR", L"ES-BO", L"ES-CL", L"ES-CO", L"ES-CR", L"ES-DO", L"ES-EC", L"ES-ES", L"ES-GT", L"ES-HN", L"ES-MX",
                L"ES-NI", L"ES-PA", L"ES-PE", L"ES-PR", L"ES-PY", L"ES-SV", L"ES-US", L"ES-UY", L"ES-VE", L"ET-EE", L"EU-ES",
                L"FA-IR", L"FI-FI", L"FIL-PH", L"FO-FO", L"FR-BE", L"FR-CA", L"FR-CH", L"FR-FR", L"FR-LU", L"FR-MC", L"FY-NL",
                L"GA-IE", L"GD-DB", L"GL-ES", L"GSW-FR", L"GU-IN", L"HE-IL", L"HI-IN", L"HR-BA", L"HR-HR", L"HSB-DE", L"HU-HU",
                L"HY-AM", L"ID-ID", L"IG-NG", L"II-CN", L"IS-IS", L"IT-CH", L"IT-IT", L"JA-JP", L"KA-GE", L"KK-KZ", L"KL-GL",
                L"KM-KH", L"KN-IN", L"KOK-IN", L"KO-KR", L"KY-KG", L"LB-LU", L"LO-LA", L"LT-LT", L"LV-LV", L"MI-NZ", L"MK-MK",
                L"ML-IN", L"MN-MN", L"MOH-CA", L"MR-IN", L"MS-BN", L"MS-MY", L"MT-MT", L"NB-NO", L"NE-NP", L"NL-BE", L"NL-NL",
                L"NN-NO", L"NSO-ZA", L"OC-FR", L"OR-IN", L"PA-IN", L"PL-PL", L"PRS-AF", L"PS-AF", L"PT-BR", L"PT-PT", L"QUT-GT",
                L"QUZ-BO", L"QUZ-EC", L"QUZ-PE", L"RM-CH", L"RO-RO", L"RU-RU", L"RW-RW", L"SAH-RU", L"SA-IN", L"SE-FI", L"SE-NO",
                L"SE-SE", L"SI-LK", L"SK-SK", L"SL-SI", L"SMA-NO", L"SMA-SE", L"SMJ-NO", L"SMJ-SE", L"SMN-FI", L"SMS-FI", L"SQ-AL",
                L"SV-FI", L"SV-SE", L"SW-KE", L"SYR-SY", L"TA-IN", L"TE-IN", L"TH-TH", L"TK-TM", L"TN-ZA", L"TR-TR", L"TT-RU",
                L"UG-CN", L"UK-UA", L"UR-PK", L"VI-VN", L"WO-SN", L"XH-ZA", L"YO-NG", L"ZH-CN", L"ZH-HK", L"ZH-MO", L"ZH-SG",
                L"ZH-TW", L"ZU-ZA", L"AZ-CYRL-AZ", L"AZ-LATN-AZ", L"BS-CYRL-BA", L"BS-LATN-BA", L"HA-LATN-NG", L"IU-CANS-CA",
                L"IU-LATN-CA", L"MN-MONG-CN", L"SR-CYRL-BA", L"SR-CYRL-CS", L"SR-CYRL-ME", L"SR-CYRL-RS", L"SR-LATN-BA",
                L"SR-LATN-CS", L"SR-LATN-ME", L"SR-LATN-RS", L"TG-CYRL-TJ", L"TZM-LATN-DZ", L"UZ-CYRL-UZ", L"UZ-LATN-UZ",
            };

            // The folded and sorted version of LocaleViews.
            const std::vector<std::wstring> Locales;

            // Add values here but use LegalEntitySuffixes in code.
            const std::vector<std::wstring_view> LegalEntitySuffixViews
            {
                // Acronyms
                L"AB", L"AD", L"AG", L"APS", L"AS", L"ASA", L"BV", L"CO", L"CV", L"DOO", L"eV", L"GES", L"GESMBH", L"GMBH", L"INC", L"KG",
                L"KS", L"PS", L"LLC", L"LP", L"LTD", L"LTDA", L"MBH", L"NV", L"PLC", L"SL", L"PTY", L"PVT", L"SA", L"SARL",
                L"SC", L"SCA", L"SL", L"SP", L"SPA", L"SRL", L"SRO",

                // Words
                L"COMPANY", L"CORP", L"CORPORATION", L"HOLDING", L"HOLDINGS", L"INCORPORATED", L"LIMITED", L"SUBSIDIARY"
            };

            // The folded and sorted version of LocaleViews.
            const std::vector<std::wstring> LegalEntitySuffixes;

            static std::vector<std::wstring> FoldAndSort(const std::vector<std::wstring_view>& input)
            {
                std::vector<std::wstring> result;
                std::transform(input.begin(), input.end(), std::back_inserter(result), [](const std::wstring_view wsv) { return Utility::ConvertToUTF16(Utility::FoldCase(wsv)); });
                std::sort(result.begin(), result.end());
                return result;
            }

            InterimNameNormalizationResult NormalizeNameInternal(std::string_view name) const
            {
                InterimNameNormalizationResult result;
                result.Name = PrepareForValidation(name);
                while (Unwrap(result.Name)); // remove wrappers

                // handle (large majority of) SAP Business Object programs
                if (SAPPackage.IsMatch(result.Name))
                {
                    return result;
                }

                result.Architecture = RemoveArchitecture(result.Name);
                result.Locale = RemoveLocale(result.Name);

                // Extract KB numbers from their parens and preserve them
                result.Name = KBNumbers.Replace(result.Name, L"$1");

                // Repeatedly remove matches for the regexes to create the minimum name
                while (RemoveAll(ProgramNameRegexes, result.Name));

                auto tokens = Split(ProgramNameSplit, result.Name, LegalEntitySuffixes);
                result.Name = Join(tokens);

                // Drop all undesired characters
                Remove(NonLettersAndDigits, result.Name);

                return result;
            }

            InterimPublisherNormalizationResult NormalizePublisherInternal(std::string_view publisher) const
            {
                InterimPublisherNormalizationResult result;

                result.Publisher = PrepareForValidation(publisher);
                while (Unwrap(result.Publisher)); // remove wrappers

                while (RemoveAll(PublisherNameRegexes, result.Publisher));

                auto tokens = Split(PublisherNameSplit, result.Publisher, LegalEntitySuffixes, true);
                result.Publisher = Join(tokens);

                // Drop all undesired characters
                Remove(NonLettersAndDigits, result.Publisher);

                return result;
            }

        public:
            NormalizationInitial() : Locales(FoldAndSort(LocaleViews)), LegalEntitySuffixes(FoldAndSort(LegalEntitySuffixViews))
            {
            }

            NormalizedName Normalize(std::string_view name, std::string_view publisher) const override
            {
                InterimNameNormalizationResult nameResult = NormalizeNameInternal(name);
                InterimPublisherNormalizationResult pubResult = NormalizePublisherInternal(publisher);

                NormalizedName result;
                result.Name(ConvertToUTF8(nameResult.Name));
                result.Architecture(nameResult.Architecture);
                result.Locale(ConvertToUTF8(nameResult.Locale));
                result.Publisher(ConvertToUTF8(pubResult.Publisher));

                return result;
            }

            NormalizedName NormalizeName(std::string_view name) const override
            {
                InterimNameNormalizationResult nameResult = NormalizeNameInternal(name);

                NormalizedName result;
                result.Name(ConvertToUTF8(nameResult.Name));
                result.Architecture(nameResult.Architecture);
                result.Locale(ConvertToUTF8(nameResult.Locale));

                return result;
            }

            std::string NormalizePublisher(std::string_view publisher) const override
            {
                InterimPublisherNormalizationResult pubResult = NormalizePublisherInternal(publisher);

                return ConvertToUTF8(pubResult.Publisher);
            }
        };
    }

    NameNormalizer::NameNormalizer(NormalizationVersion version)
    {
        switch (version)
        {
        case AppInstaller::Utility::NormalizationVersion::Initial:
            m_normalizer = std::make_unique<NormalizationInitial>();
            break;
        default:
            THROW_HR(E_INVALIDARG);
        }
    }

    NormalizedName NameNormalizer::Normalize(std::string_view name, std::string_view publisher) const
    {
        return m_normalizer->Normalize(name, publisher);
    }

    NormalizedName NameNormalizer::NormalizeName(std::string_view name) const
    {
        return m_normalizer->NormalizeName(name);
    }

    std::string NameNormalizer::NormalizePublisher(std::string_view publisher) const
    {
        return m_normalizer->NormalizePublisher(publisher);
    }
}
