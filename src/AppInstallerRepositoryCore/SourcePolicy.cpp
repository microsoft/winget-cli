// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourcePolicy.h"
#include "SourceList.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include <winget/GroupPolicy.h>


namespace AppInstaller::Repository
{
    using namespace Settings;

    namespace
    {
        // Checks whether a default source is enabled with the current settings.
        // onlyExplicit determines whether we consider the not-configured state to be enabled or not.
        bool IsDefaultSourceEnabled(std::string_view sourceToLog, ExperimentalFeature::Feature feature, bool onlyExplicit, TogglePolicy::Policy policy)
        {
            if (!ExperimentalFeature::IsEnabled(feature))
            {
                // No need to log here
                return false;
            }

            if (onlyExplicit)
            {
                // No need to log here
                return GroupPolicies().GetState(policy) == PolicyState::Enabled;
            }

            if (!GroupPolicies().IsEnabled(policy))
            {
                AICLI_LOG(Repo, Info, << "The default source " << sourceToLog << " is disabled due to Group Policy");
                return false;
            }

            return true;
        }

        bool IsWingetCommunityDefaultSourceEnabled(bool onlyExplicit = false)
        {
            return IsDefaultSourceEnabled(WingetCommunityDefaultSourceName(), ExperimentalFeature::Feature::None, onlyExplicit, TogglePolicy::Policy::DefaultSource);
        }

        bool IsWingetMSStoreDefaultSourceEnabled(bool onlyExplicit = false)
        {
            return IsDefaultSourceEnabled(WingetMSStoreDefaultSourceName(), ExperimentalFeature::Feature::ExperimentalMSStore, onlyExplicit, TogglePolicy::Policy::MSStoreSource);
        }

        template<ValuePolicy P>
        std::optional<SourceFromPolicy> FindSourceInPolicy(std::string_view name, std::string_view type, std::string_view arg)
        {
            auto sourcesOpt = GroupPolicies().GetValueRef<P>();
            if (!sourcesOpt.has_value())
            {
                return std::nullopt;
            }

            const auto& sources = sourcesOpt->get();
            auto source = std::find_if(
                sources.begin(),
                sources.end(),
                [&](const SourceFromPolicy& policySource)
                {
                    return Utility::ICUCaseInsensitiveEquals(name, policySource.Name) && Utility::ICUCaseInsensitiveEquals(type, policySource.Type) && arg == policySource.Arg;
                });

            if (source == sources.end())
            {
                return std::nullopt;
            }

            return *source;
        }

        template<ValuePolicy P>
        bool IsSourceInPolicy(std::string_view name, std::string_view type, std::string_view arg)
        {
            return FindSourceInPolicy<P>(name, type, arg).has_value();
        }
    }

    TogglePolicy::Policy GetPolicyBlockingUserSource(std::string_view name, std::string_view type, std::string_view arg, bool isTombstone)
    {
        // Reasons for not allowing:
        //  1. The source is a tombstone for default source that is explicitly enabled
        //  2. The source is a default source that is disabled
        //  3. The source has the same name as a default source that is explicitly enabled (to prevent shadowing)
        //  4. Allowed sources are disabled, blocking all user sources
        //  5. There is an explicit list of allowed sources and this source is not in it
        //
        // We don't need to check sources added by policy as those have higher priority.
        //
        // Use the name and arg to match sources as we don't have the identifier before adding.

        // Case 1:
        // The source is a tombstone and we need the policy to be explicitly enabled.
        if (isTombstone)
        {
            if (name == WingetCommunityDefaultSourceName() && IsWingetCommunityDefaultSourceEnabled(true))
            {
                return TogglePolicy::Policy::DefaultSource;
            }

            if (name == WingetMSStoreDefaultSourceName() && IsWingetMSStoreDefaultSourceEnabled(true))
            {
                return TogglePolicy::Policy::MSStoreSource;
            }

            // Any other tombstone is allowed
            return TogglePolicy::Policy::None;
        }

        // Case 2:
        //  - The source is not a tombstone and we don't need the policy to be explicitly enabled.
        //  - Check only against the source argument and type as the user source may have a different name.
        //  - Do a case insensitive check as the domain portion of the URL is case insensitive,
        //    and we don't need case sensitivity for the rest as we control the domain.
        if (Utility::CaseInsensitiveEquals(arg, WingetCommunityDefaultSourceArg()) &&
            Utility::CaseInsensitiveEquals(type, Microsoft::PreIndexedPackageSourceFactory::Type()))
        {
            return IsWingetCommunityDefaultSourceEnabled(false) ? TogglePolicy::Policy::None : TogglePolicy::Policy::DefaultSource;
        }

        if (Utility::CaseInsensitiveEquals(arg, WingetMSStoreDefaultSourceArg()) &&
            Utility::CaseInsensitiveEquals(type, Microsoft::PreIndexedPackageSourceFactory::Type()))
        {
            return IsWingetMSStoreDefaultSourceEnabled(false) ? TogglePolicy::Policy::None : TogglePolicy::Policy::MSStoreSource;
        }

        // Case 3:
        // If the source has the same name as a default source, it is shadowing with a different argument
        // (as it didn't match above). We only care if Group Policy requires the default source.
        if (name == WingetCommunityDefaultSourceName() && IsWingetCommunityDefaultSourceEnabled(true))
        {
            AICLI_LOG(Repo, Warning, << "User source is not allowed as it shadows the default source. Name [" << name << "]. Arg [" << arg << "] Type [" << type << ']');
            return TogglePolicy::Policy::DefaultSource;
        }

        if (name == WingetMSStoreDefaultSourceName() && IsWingetMSStoreDefaultSourceEnabled(true))
        {
            AICLI_LOG(Repo, Warning, << "User source is not allowed as it shadows the default MS Store source. Name [" << name << "]. Arg [" << arg << "] Type [" << type << ']');
            return TogglePolicy::Policy::MSStoreSource;
        }

        // Case 4:
        // The guard in the source add command should already block adding.
        // This check drops existing user sources.
        auto allowedSourcesPolicy = GroupPolicies().GetState(TogglePolicy::Policy::AllowedSources);
        if (allowedSourcesPolicy == PolicyState::Disabled)
        {
            AICLI_LOG(Repo, Warning, << "User sources are disabled by Group Policy");
            return TogglePolicy::Policy::AllowedSources;
        }

        // Case 5:
        if (allowedSourcesPolicy == PolicyState::Enabled)
        {
            if (!IsSourceInPolicy<ValuePolicy::AllowedSources>(name, type, arg))
            {
                AICLI_LOG(Repo, Warning, << "Source is not in the Group Policy allowed list. Name [" << name << "]. Arg [" << arg << "] Type [" << type << ']');
                return TogglePolicy::Policy::AllowedSources;
            }
        }

        return TogglePolicy::Policy::None;
    }
}
