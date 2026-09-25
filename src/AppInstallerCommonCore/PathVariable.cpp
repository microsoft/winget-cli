// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PathVariable.h"
#include <winget/Filesystem.h>

using namespace AppInstaller::Utility;

namespace AppInstaller::Registry::Environment
{
    namespace
    {
        constexpr std::wstring_view s_PathName = L"Path";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";

        void EnsurePathValueEndsWithSemicolon(std::string& value)
        {
            if (!value.empty() && value.back() != ';')
            {
                value += ';';
            }
        }

        // Preserves trailing slash for drive root (e.g. "C:\")
        constexpr size_t s_DriveRootLength = 3;

        // Cleans up a raw path entry by stripping leading/trailing whitespace, semicolons, and enclosing quotes.
        // Returns true if the entry was modified.
        bool CleanPathEntry(std::wstring& entry)
        {
            bool modified = false;
            bool changed = true;
            while (changed)
            {
                changed = false;
                size_t prevLength = entry.length();
                Utility::Trim(entry);
                if (entry.length() != prevLength)
                {
                    modified = true;
                }
                while (!entry.empty() && entry.front() == L';')
                {
                    entry.erase(0, 1);
                    changed = true;
                    modified = true;
                }
                while (!entry.empty() && entry.back() == L';')
                {
                    entry.pop_back();
                    changed = true;
                    modified = true;
                }
                prevLength = entry.length();
                Utility::Trim(entry);
                if (entry.length() != prevLength)
                {
                    modified = true;
                }
                if (entry.size() >= 2 && entry.front() == L'"' && entry.back() == L'"')
                {
                    entry = entry.substr(1, entry.size() - 2);
                    changed = true;
                    modified = true;
                }
            }
            return modified;
        }

        std::wstring NormalizeAndExpandPath(const std::filesystem::path& path)
        {
            std::wstring trimmedEntry = Utility::Normalize(path.wstring());
            CleanPathEntry(trimmedEntry);

            if (trimmedEntry.empty())
            {
                return {};
            }

            std::wstring expanded;
            try
            {
                expanded = Utility::ExpandEnvironmentVariables(trimmedEntry);
            }
            catch (...)
            {
                AICLI_LOG(Core, Warning, << "Failed to expand environment variables for PATH entry: " << Utility::ConvertToUTF8(trimmedEntry));
                expanded = trimmedEntry;
            }

            std::filesystem::path expandedPath{ expanded };
            expandedPath.make_preferred();
            std::wstring result = expandedPath.wstring();
            while (result.size() > s_DriveRootLength && result.back() == L'\\')
            {
                result.pop_back();
            }

            return Utility::Normalize(result);
        }

        std::string ExpandPathValue(const std::string& value)
        {
            std::wstring wideValue = ConvertToUTF16(value);
            std::vector<std::wstring> pathEntries = Split(wideValue, L';');
            std::wstring result;
            for (const std::wstring& pathEntry : pathEntries)
            {
                if (!pathEntry.empty())
                {
                    std::wstring expanded = NormalizeAndExpandPath(pathEntry);
                    if (!expanded.empty())
                    {
                        result += expanded;
                        result += L';';
                    }
                }
            }
            return ConvertToUTF8(result);
        }
    }

    PathVariable::PathVariable(Manifest::ScopeEnum scope, bool readOnly) : m_scope(scope), m_readOnly(readOnly)
    {
        if (m_readOnly)
        {
            if (m_scope == Manifest::ScopeEnum::Machine)
            {
                m_key = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, std::wstring{ s_PathSubkey_Machine });
            }
            else
            {
                m_key = Registry::Key::OpenIfExists(HKEY_CURRENT_USER, std::wstring{ s_PathSubkey_User });
            }
        }
        else
        {
            if (m_scope == Manifest::ScopeEnum::Machine)
            {
                m_key = Registry::Key::Create(HKEY_LOCAL_MACHINE, std::wstring{ s_PathSubkey_Machine });
            }
            else
            {
                m_key = Registry::Key::Create(HKEY_CURRENT_USER, std::wstring{ s_PathSubkey_User });
            }
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    PathVariable::PathVariable(Manifest::ScopeEnum scope, Registry::Key key, bool readOnly, bool broadcastEnvironmentChange) :
        m_scope(scope), m_key(std::move(key)), m_readOnly(readOnly), m_broadcastEnvironmentChange(broadcastEnvironmentChange)
    {
    }
#endif

    std::string PathVariable::GetPathValue()
    {
        std::wstring pathName = std::wstring{ s_PathName };
        auto pathValue = m_key[pathName];
        if (pathValue.has_value())
        {
            return Normalize(pathValue->GetValue<Value::Type::String>());
        }
        return {};
    }

    bool PathVariable::ContainsInternal(const std::wstring& targetExpanded)
    {
        if (targetExpanded.empty())
        {
            return false;
        }

        std::wstring widePathValue = ConvertToUTF16(GetPathValue());
        std::vector<std::wstring> pathEntries = Split(widePathValue, L';');
        for (const std::wstring& pathEntry : pathEntries)
        {
            if (!pathEntry.empty() && Utility::CaseInsensitiveEquals(NormalizeAndExpandPath(pathEntry), targetExpanded))
            {
                return true;
            }
        }

        return false;
    }

    bool PathVariable::Contains(const std::filesystem::path& target)
    {
        return ContainsInternal(NormalizeAndExpandPath(target));
    }

    bool PathVariable::Remove(const std::filesystem::path& target)
    {
        THROW_HR_IF(E_ACCESSDENIED, m_readOnly);

        std::wstring targetExpanded = NormalizeAndExpandPath(target);
        if (targetExpanded.empty())
        {
            return false;
        }

        std::wstring widePathValue = ConvertToUTF16(GetPathValue());
        std::vector<std::wstring> pathEntries = Split(widePathValue, L';');
        std::wstring result;
        bool removed = false;

        for (const std::wstring& pathEntry : pathEntries)
        {
            if (pathEntry.empty())
            {
                continue;
            }

            if (Utility::CaseInsensitiveEquals(NormalizeAndExpandPath(pathEntry), targetExpanded))
            {
                removed = true;
            }
            else
            {
                result += pathEntry;
                result += L';';
            }
        }

        if (removed)
        {
            SetPathValue(ConvertToUTF8(result));
            return true;
        }

        return false;
    }

    bool PathVariable::Append(const std::filesystem::path& target)
    {
        THROW_HR_IF(E_ACCESSDENIED, m_readOnly);

        std::wstring targetExpanded = NormalizeAndExpandPath(target);
        if (targetExpanded.empty())
        {
            return false;
        }

        if (!ContainsInternal(targetExpanded))
        {
            // Store the path in environment variable form when possible (e.g. %LOCALAPPDATA%\Microsoft\WinGet\Links)
            // rather than as a fully expanded path, so that the entry keeps working when the underlying
            // folder location changes, such as after a user profile rename.
            bool allowUserVariables = (m_scope != Manifest::ScopeEnum::Machine);
            std::wstring cleanTarget = Utility::Normalize(target.wstring());
            CleanPathEntry(cleanTarget);
            std::string targetString = Normalize(AppInstaller::Filesystem::GetUnexpandedPath(cleanTarget, allowUserVariables).u8string());
            while (!targetString.empty() && targetString.back() == ';')
            {
                targetString.pop_back();
            }
            std::string pathValue = GetPathValue();
            if (!pathValue.empty())
            {
                EnsurePathValueEndsWithSemicolon(pathValue);
            }
            pathValue += targetString;
            EnsurePathValueEndsWithSemicolon(pathValue);
            SetPathValue(pathValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    void PathVariable::SetPathValue(const std::string& value)
    {
        THROW_HR_IF(E_ACCESSDENIED, m_readOnly);

        std::wstring pathName = std::wstring{ s_PathName };
        m_key.SetValue(pathName, ConvertToUTF16(value), REG_EXPAND_SZ);
        if (m_broadcastEnvironmentChange)
        {
            SendNotifyMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Environment"));
        }
    }

    bool RefreshPathVariableForCurrentProcess()
    {
        // Path values must be expanded before assigning to process environment for proper refresh.
        std::string systemPathValue = ExpandPathValue(PathVariable(Manifest::ScopeEnum::Machine, true).GetPathValue());
        std::string userPathValue = ExpandPathValue(PathVariable(Manifest::ScopeEnum::User, true).GetPathValue());
        std::wstring pathValue = ConvertToUTF16(systemPathValue + userPathValue);
        return _wputenv_s(L"PATH", pathValue.c_str()) == 0;
    }
}