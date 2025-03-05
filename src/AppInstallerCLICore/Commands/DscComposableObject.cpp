// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscComposableObject.h"

namespace AppInstaller::CLI
{
    namespace details
    {
        const Json::Value* GetProperty(const Json::Value& object, std::string_view name)
        {
            return object.find(name.data(), name.data() + name.length());
        }

        void AddProperty(Json::Value& object, std::string_view name, std::optional<Json::Value>&& value)
        {
            if (value)
            {
                object[std::string{ name }] = std::move(value).value();
            }
        }
    }
}
