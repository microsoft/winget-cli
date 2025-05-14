// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerLogging.h>
#include <winget/LocIndependent.h>
#include "Resources.h"
#include <json/json.h>
#include <optional>
#include <string>
#include <vector>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    // Flags that define how to treat properties.
    enum DscComposablePropertyFlag
    {
        None = 0x0,
        Required = 0x1,
        CopyToOutput = 0x2,
    };

    DEFINE_ENUM_FLAG_OPERATORS(DscComposablePropertyFlag);

    namespace details
    {
        // Gets a property or null if not present.
        const Json::Value* GetProperty(const Json::Value& object, std::string_view name);

        // Adds the given property and value to the object, if provided.
        void AddProperty(Json::Value& object, std::string_view name, std::optional<Json::Value>&& value);

        // Gets the default schema object.
        Json::Value GetBaseSchema(const std::string& title);

        // Adds a property to the schema object.
        void AddPropertySchema(
            Json::Value& object,
            std::string_view name,
            DscComposablePropertyFlag flags,
            Json::ValueType type,
            std::string_view description,
            const std::vector<std::string>& enumValues,
            const std::optional<std::string>& defaultValue);
    }

    template <typename PropertyType>
    struct GetJsonTypeValue
    {
        static_assert(false, "Implement for this type.");
    };

    template <>
    struct GetJsonTypeValue<bool>
    {
        static bool Get(const Json::Value& value)
        {
            return value.asBool();
        }

        static Json::ValueType SchemaType()
        {
            return Json::ValueType::booleanValue;
        }
    };

    template <>
    struct GetJsonTypeValue<std::string>
    {
        static std::string Get(const Json::Value& value)
        {
            return value.asString();
        }

        static Json::ValueType SchemaType()
        {
            return Json::ValueType::stringValue;
        }
    };

    template <>
    struct GetJsonTypeValue<Json::Value>
    {
        static Json::Value Get(const Json::Value& value)
        {
            return value;
        }

        static Json::ValueType SchemaType()
        {
            return Json::ValueType::objectValue;
        }
    };

    // Template useful for composing objects for DSC resources.
    // Properties should be of the shape:
    //
    // struct Property
    // {
    //      using PropertyType = { bool, std::string };
    //      static std::string_view Name();
    //      static void FromJson(Property*, const Json::Value*);
    //      static std::optional<Json::Value> ToJson(const Property*);
    //
    //      const PropertyType& PROPERTY_NAME() const;
    //      void PROPERTY_NAME(const PropertyType&);
    // }
    template <typename... Property>
    struct DscComposableObject : public Property...
    {
        DscComposableObject() = default;

        DscComposableObject(const std::optional<Json::Value>& input, bool ignoreFieldRequirements = false)
        {
            THROW_HR_IF(E_POINTER, !input && !ignoreFieldRequirements);
            if (input)
            {
                FromJson(input.value(), ignoreFieldRequirements);
            }
        }

        // Read values for each property
        void FromJson(const Json::Value& input, bool ignoreFieldRequirements = false)
        {
            (FoldHelper{}, ..., Property::FromJson(this, details::GetProperty(input, Property::Name()), ignoreFieldRequirements));
        }

        // Populate JSON object with properties.
        Json::Value ToJson()
        {
            Json::Value result{ Json::ValueType::objectValue };
            (FoldHelper{}, ..., details::AddProperty(result, Property::Name(), Property::ToJson(this)));
            return result;
        }

        // Copies the appropriate values to a new object for output.
        DscComposableObject CopyForOutput() const
        {
            DscComposableObject result;
            (FoldHelper{}, ..., Property::CopyForOutput(this, &result));
            return result;
        }

        // Get the JSON Schema for this object
        static Json::Value Schema(const std::string& title)
        {
            Json::Value result = details::GetBaseSchema(title);
            (FoldHelper{}, ..., details::AddPropertySchema(result, Property::Name(), Property::Flags, GetJsonTypeValue<typename Property::PropertyType>::SchemaType(), Property::Description(), Property::EnumValues(), Property::Default()));
            return result;
        }
    };

    template <typename Derived, typename PropertyTypeT, DscComposablePropertyFlag PropertyFlags>
    struct DscComposableProperty
    {
        using PropertyType = PropertyTypeT;
        static constexpr DscComposablePropertyFlag Flags = PropertyFlags;

        static void FromJson(Derived* self, const Json::Value* value, bool ignoreFieldRequirements)
        {
            if (value && !value->isNull())
            {
                self->m_value = GetJsonTypeValue<PropertyType>::Get(*value);
            }
            else
            {
                if (!ignoreFieldRequirements && WI_IsFlagSet(PropertyFlags, DscComposablePropertyFlag::Required))
                {
                    THROW_HR_MSG(WINGET_CONFIG_ERROR_MISSING_FIELD, "Required property `%hs` not provided.", Derived::Name().data());
                }
                else
                {
                    self->m_value = std::nullopt;
                }
            }
        }

        static std::optional<Json::Value> ToJson(const Derived* self)
        {
            if constexpr (WI_IsFlagSet(PropertyFlags, DscComposablePropertyFlag::Required))
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_MISSING_FIELD, !self->m_value);
                return self->m_value.value();
            }
            else
            {
                return self->m_value ? std::optional<Json::Value>{ self->m_value.value() } : std::nullopt;
            }
        }

        static void CopyForOutput(const Derived* self, Derived* other)
        {
            if constexpr (WI_IsFlagSet(PropertyFlags, DscComposablePropertyFlag::CopyToOutput))
            {
                other->m_value = self->m_value;
            }
        }

    protected:
        std::optional<PropertyType> m_value;
    };

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL_START(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_, _enum_vals_, _default_) \
    struct _property_type_ : public DscComposableProperty<_property_type_, _value_type_, _flags_> \
    { \
        static std::string_view Name() { return _json_name_; } \
        static Resource::LocString Description() { return _description_; } \
        static std::vector<std::string> EnumValues() { return std::vector<std::string> _enum_vals_; } \
        static std::optional<std::string> Default() { return _default_; } \
        std::optional<PropertyType>& _property_name_() { return m_value; } \
        const std::optional<PropertyType>& _property_name_() const { return m_value; } \
        void _property_name_(std::optional<PropertyType> value) { m_value = std::move(value); } \

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_, _enum_vals_, _default_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL_START(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_, _enum_vals_, _default_) \
    };

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(_property_type_, _value_type_, _property_name_, _json_name_, _description_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, DscComposablePropertyFlag::None, _description_, {}, {})

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_, {}, {})

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_DEFAULT(_property_type_, _value_type_, _property_name_, _json_name_, _description_, _default_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, DscComposablePropertyFlag::None, _description_, {}, _default_)

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(_property_type_, _value_type_, _property_name_, _json_name_, _description_, _enum_vals_, _default_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, DscComposablePropertyFlag::None, _description_, _enum_vals_, _default_)

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM_FLAGS(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_, _enum_vals_, _default_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_, _enum_vals_, _default_)

    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL_START(StandardExistProperty, bool, Exist, "_exist", DscComposablePropertyFlag::None, Resource::String::DscResourcePropertyDescriptionExist, {}, {})
        bool ShouldExist() const { return m_value.value_or(true); }
    };

    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(StandardInDesiredStateProperty, bool, InDesiredState, "_inDesiredState", Resource::String::DscResourcePropertyDescriptionInDesiredState);
}
