// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerLogging.h>
#include <json/json.h>
#include <optional>

using namespace std::string_view_literals;

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
        void AddPropertySchema(Json::Value& object, std::string_view name, DscComposablePropertyFlag flags, std::string_view type, std::string_view description);
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

        static std::string_view SchemaTypeName()
        {
            return "boolean"sv;
        }
    };

    template <>
    struct GetJsonTypeValue<std::string>
    {
        static std::string Get(const Json::Value& value)
        {
            return value.asString();
        }

        static std::string_view SchemaTypeName()
        {
            return "string"sv;
        }
    };

    template <>
    struct GetJsonTypeValue<Json::Value>
    {
        static Json::Value Get(const Json::Value& value)
        {
            return value;
        }

        static std::string_view SchemaTypeName()
        {
            // Indicates that the schema should not set a type
            return {};
        }
    };

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
        DscComposableObject() = default;

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

        // Copies the appropriate values to a new object for output.
        DscComposableObject CopyForOutput()
        {
            DscComposableObject result;
            (FoldHelper{}, ..., Property::CopyForOutput(this, &result));
            return result;
        }

        // Get the JSON Schema for this object
        static Json::Value Schema(const std::string& title)
        {
            Json::Value result = details::GetBaseSchema(title);
            (FoldHelper{}, ..., details::AddPropertySchema(result, Property::Name(), Property::Flags, GetJsonTypeValue<typename Property::Type>::SchemaTypeName(), Property::Description()));
            return result;
        }
    };

    template <typename Derived, typename PropertyType, DscComposablePropertyFlag PropertyFlags>
    struct DscComposableProperty
    {
        using Type = PropertyType;
        static constexpr DscComposablePropertyFlag Flags = PropertyFlags;

        static void FromJson(Derived* self, const Json::Value* value)
        {
            if (value)
            {
                self->m_value = GetJsonTypeValue<PropertyType>::Get(*value);
            }
            else
            {
                if constexpr (WI_IsFlagSet(PropertyFlags, DscComposablePropertyFlag::Required))
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
        std::optional<Type> m_value;
    };

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL_START(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_) \
    struct _property_type_ : public DscComposableProperty<_property_type_, _value_type_, _flags_> \
    { \
        static std::string_view Name() { return _json_name_; } \
        static std::string_view Description() { return _description_; } \
        std::optional<Type>& _property_name_() { return m_value; } \
        const std::optional<Type>& _property_name_() const { return m_value; } \
        void _property_name_(std::optional<Type> value) { m_value = std::move(value); } \

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_) \
    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL_START(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_) \
    };

#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(_property_type_, _value_type_, _property_name_, _json_name_, _description_) WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, DscComposablePropertyFlag::None, _description_)
#define WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_) WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL(_property_type_, _value_type_, _property_name_, _json_name_, _flags_, _description_)

    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_IMPL_START(StandardExistProperty, bool, Exist, "_exist", DscComposablePropertyFlag::None, "Indicates whether an instance should/does exist.")
        bool ShouldExist() { return m_value.value_or(true); }
    };

    WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(StandardInDesiredStateProperty, bool, InDesiredState, "_inDesiredState", "Indicates whether an instance is in the desired state.");
}
