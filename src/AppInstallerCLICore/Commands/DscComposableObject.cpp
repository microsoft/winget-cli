// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscComposableObject.h"

namespace AppInstaller::CLI
{
    namespace
    {
        std::string GetTypeString(Json::ValueType type)
        {
            switch (type)
            {
            case Json::nullValue:
                return "null";
            case Json::intValue:
            case Json::uintValue:
            case Json::realValue:
                return "number";
            case Json::stringValue:
                return "string";
            case Json::booleanValue:
                return "boolean";
            case Json::arrayValue:
                return "array";
            case Json::objectValue:
                return "object";
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }
    }

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

        void AddPropertySchema(
            Json::Value& object,
            std::string_view name,
            DscComposablePropertyFlag flags,
            Json::ValueType type,
            std::string_view description,
            const std::vector<std::string>& enumValues,
            const std::optional<std::string>& defaultValue)
        {
            Json::Value& propertiesObject = object["properties"];

            if (propertiesObject.isNull())
            {
                propertiesObject = Json::Value{ Json::ValueType::objectValue };
            }

            std::string nameString{ name };

            Json::Value property{ Json::ValueType::objectValue };

            if (type != Json::ValueType::objectValue)
            {
                property["type"] = GetTypeString(type);
            }

            property["description"] = std::string{ description };

            if (!enumValues.empty())
            {
                Json::Value enumArray{ Json::ValueType::arrayValue };

                for (const std::string& enumValue : enumValues)
                {
                    enumArray.append(enumValue);
                }

                property["enum"] = std::move(enumArray);
            }

            if (defaultValue)
            {
                property["default"] = defaultValue.value();
            }

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
