// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <AppInstallerLanguageUtilities.h>
#include <json/json.h>
#include <optional>

namespace AppInstaller::CLI
{
    namespace details
    {
        // Gets a property or null if not present.
        const Json::Value* GetProperty(const Json::Value& object, std::string_view name);

        // Adds the given property and value to the object, if provided.
        void AddProperty(Json::Value& object, std::string_view name, std::optional<Json::Value>&& value);
    }

    // Template useful for composing objects for DSC resources.
    // Properties should be of the shape:
    //
    // struct Property
    // {
    //      using Type = { bool, std::string };
    //      static std::string_view Name();
    //      static void FromJson(Property*, const Json::Value*);
    //      static std::optional<Json::Value> ToJson(const Property*);
    //
    //      const Type& PROPERTY_NAME() const;
    //      void PROPERTY_NAME(const Type&);
    // }
    template <typename... Property>
    struct DscComposableObject : public Property...
    {
        DscComposableObject(const std::optional<Json::Value>& input)
        {
            THROW_HR_IF(E_POINTER, !input);
            FromJson(input.value());
        }

        // Read values for each property
        void FromJson(const Json::Value& input)
        {
            (FoldHelper{}, ..., Property::FromJson(this, details::GetProperty(input, Property::Name())));
        }

        // Populate JSON object with properties.
        Json::Value ToJson()
        {
            Json::Value result{ Json::ValueType::objectValue };
            (FoldHelper{}, ..., details::AddProperty(result, Property::Name(), Property::ToJson(this)));
            return result;
        }

        // Get the JSON Schema for this object
        static Json::Value Schema()
        {
            Json::Value result = details::GetBaseSchema();
        }
    };

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
    };

    template <>
    struct GetJsonTypeValue<std::string>
    {
        static std::string Get(const Json::Value& value)
        {
            return value.asString();
        }
    };

    template <typename Derived, typename PropertyType, bool Required = false>
    struct DscComposableProperty
    {
        using Type = PropertyType;

        static void FromJson(Derived* self, const Json::Value* value)
        {
            if (value)
            {
                self->m_value = GetJsonTypeValue<PropertyType>::Get(*value);
            }
            else
            {
                if constexpr (Required)
                {
                    THROW_HR(WINGET_CONFIG_ERROR_MISSING_FIELD);
                }
                else
                {
                    self->m_value = std::nullopt;
                }
            }
        }

        static std::optional<Json::Value> ToJson(const Derived* self)
        {
            if constexpr (Required)
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_MISSING_FIELD, !self->m_value);
                return self->m_value.value();
            }
            else
            {
                return self->m_value ? std::optional<Json::Value>{ self->m_value.value() } : std::nullopt;
            }
        }

    protected:
        std::optional<Type> m_value;
    };

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, _required_) \
    struct _property_type_ : public DscComposableProperty<_property_type_, _value_type_, _required_> \
    { \
        static std::string_view Name() { return _json_name_; } \
        const std::optional<Type>& _property_name_() const { return m_value; } \
        void _property_name_(std::optional<Type> value) { m_value = std::move(value); } \
    };

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(_property_type_, _value_type_, _property_name_, _json_name_) WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, false)
#define WINGET_DSC_DEFINE_COMPOSABLE_REQUIRED_PROPERTY(_property_type_, _value_type_, _property_name_, _json_name_) WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, true)

    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(StandardExistProperty, bool, Exist, "_exist");
}
