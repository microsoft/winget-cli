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
            if (value.back() != ';')
            {
                value += ';';
            }
        }

        std::string ExpandPathValue(const std::string& value)
        {
            std::string result;
            std::vector<std::string> pathEntries = Split(value, ';');
            for (std::string& pathEntry : pathEntries)
            {
                if (!pathEntry.empty())
                {
                    result += AppInstaller::Filesystem::GetExpandedPath(pathEntry).u8string();
                    result += ';';
                }
            }
            return result;
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

    std::string PathVariable::GetPathValue()
    {
        std::wstring pathName = std::wstring{ s_PathName };
        return Normalize(m_key[pathName]->GetValue<Value::Type::String>());
    }

    bool PathVariable::Contains(const std::filesystem::path& target)
    {
        std::string targetString = Normalize(target.u8string());
        return (GetPathValue().find(targetString) != std::string::npos);
    }

    bool PathVariable::Remove(const std::filesystem::path& target)
    {
        THROW_HR_IF(E_ACCESSDENIED, m_readOnly);

        if (Contains(target))
        {
            std::string targetString = Normalize(target.u8string());
            std::string pathValue = GetPathValue();
            FindAndReplace(pathValue, targetString, "");
            FindAndReplace(pathValue, ";;", ";");
            SetPathValue(pathValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool PathVariable::Append(const std::filesystem::path& target)
    {
        THROW_HR_IF(E_ACCESSDENIED, m_readOnly);

        if (!Contains(target))
        {
            std::string targetString = Normalize(target.u8string());
            std::string pathValue = GetPathValue();
            EnsurePathValueEndsWithSemicolon(pathValue);
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
        SendNotifyMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)TEXT("Environment"));

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