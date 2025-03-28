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

        Json::Value GetBaseSchema(const std::string& title)
        {
            Json::Value result{ Json::ValueType::objectValue };

            result["$schema"] = "http://json-schema.org/draft-07/schema#";
            result["title"] = title;
            result["type"] = "object";
            result["additionalProperties"] = false;

            return result;
        }

        void AddPropertySchema(Json::Value& object, std::string_view name, DscComposablePropertyFlag flags, std::string_view type, std::string_view description)
        {
            Json::Value& propertiesObject = object["properties"];

            if (propertiesObject.isNull())
            {
                propertiesObject = Json::Value{ Json::ValueType::objectValue };
            }

            std::string nameString{ name };

            Json::Value property{ Json::ValueType::objectValue };

            if (!type.empty())
            {
                property["type"] = std::string{ type };
            }

            property["description"] = std::string{ description };

            propertiesObject[nameString] = std::move(property);

            if (WI_IsFlagSet(flags, DscComposablePropertyFlag::Required))
            {
                Json::Value& requiredArray = object["required"];

                if (requiredArray.isNull())
                {
                    requiredArray = Json::Value{ Json::ValueType::arrayValue };
                }

                requiredArray.append(nameString);
            }
        }
    }
}
