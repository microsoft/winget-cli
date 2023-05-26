// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PathVariable.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Registry::Environment
{
    namespace
    {
        constexpr std::wstring_view s_PathName = L"Path";
        constexpr std::wstring_view s_PathSubkey_User = L"Environment";
        constexpr std::wstring_view s_PathSubkey_Machine = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    }

    PathVariable::PathVariable(Manifest::ScopeEnum scope, bool readOnly)
    {
        if (readOnly)
        {
            if (scope == Manifest::ScopeEnum::Machine)
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
            if (scope == Manifest::ScopeEnum::Machine)
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
        if (!Contains(target))
        {
            std::string targetString = Normalize(target.u8string());
            std::string pathValue = GetPathValue();
            if (pathValue.back() != ';')
            {
                pathValue += ";";
            }

            pathValue += targetString + ";";
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
        std::wstring pathName = std::wstring{ s_PathName };
        m_key.SetValue(pathName, ConvertToUTF16(value), REG_EXPAND_SZ);
    }

    void RefreshPathVariable()
    {
        const auto& pathVariableValue = PathVariable(Manifest::ScopeEnum::User, true).GetPathValue() + PathVariable(Manifest::ScopeEnum::Machine, true).GetPathValue();
        THROW_HR_IF(E_UNEXPECTED, _putenv_s("PATH", pathVariableValue.c_str()) != 0);
    }
}