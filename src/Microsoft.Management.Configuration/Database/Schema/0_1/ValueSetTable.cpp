// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ValueSetTable.h"
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerStrings.h>
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::SQLite::Builder;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    namespace
    {
        constexpr std::string_view s_ValueSetTable_Table = "value_set"sv;
        constexpr std::string_view s_ValueSetTable_ValueSetRootRowIdIndex = "value_set_vs_idx"sv;

        // All objects start with a sentinel value at the root, which will have:
        //  ValueSetRootRowId :: Null (since we would need to reference this row)
        //  ParentRowId :: Null (as it is the root)
        //  Name :: ""
        //  Type :: Empty (a null set), Inspectable (a ValueSet), *Array (an array containing only that type of item)
        //  Value :: Null

        // The rowid of the root node or this set
        constexpr std::string_view s_ValueSetTable_Column_ValueSetRootRowId = "vs_rowid"sv;
        // The rowid of the parent object for this value
        constexpr std::string_view s_ValueSetTable_Column_ParentRowId = "parent_rowid"sv;
        // The name/index of the value
        constexpr std::string_view s_ValueSetTable_Column_Name = "name"sv;
        // Uses the integral values of Windows::Foundation::PropertyType
        constexpr std::string_view s_ValueSetTable_Column_Type = "type"sv;
        // The meaning of value depends on Type:
        //  Integral values :: int64_t (unsigned are cast to signed)
        //  Floating point values :: double
        //  Char16 :: std::string (the single character converted to UTF-8)
        //  Boolean :: bool
        //  String :: std::string
        //  Inspectable :: rowid_t (the rowid of another ValueSet root)
        //  DateTime :: int64_t (unix epoch value)
        //  TimeSpan :: int64_t (winrt duration count)
        //  Guid :: GUID
        //  *Array :: rowid_t (the rowid of another root)
        constexpr std::string_view s_ValueSetTable_Column_Value = "value"sv;

        // Gets a standard row insert statement for the table.
        // Binds:
        //  1 :: ValueSetRootRowId
        //  2 :: ParentRowId
        //  3 :: Name
        //  4 :: Type
        //  5 :: Value
        Statement GetRowInsertStatement(Connection& connection)
        {
            StatementBuilder resultBuilder;
            resultBuilder.InsertInto(s_ValueSetTable_Table).Columns({
                s_ValueSetTable_Column_ValueSetRootRowId,
                s_ValueSetTable_Column_ParentRowId,
                s_ValueSetTable_Column_Name,
                s_ValueSetTable_Column_Type,
                s_ValueSetTable_Column_Value,
            }).Values(Unbound, Unbound, Unbound, Unbound, Unbound);

            return resultBuilder.Prepare(connection);
        }
    }

    ValueSetTable::ValueSetTable(Connection& connection) : m_connection(connection) {}

    void ValueSetTable::Create()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "ValueSetTable_Create_0_1");

        StatementBuilder tableBuilder;
        tableBuilder.CreateTable(s_ValueSetTable_Table).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_ValueSetTable_Column_ValueSetRootRowId, Type::RowId),
            ColumnBuilder(s_ValueSetTable_Column_ParentRowId, Type::RowId),
            ColumnBuilder(s_ValueSetTable_Column_Name, Type::None).NotNull(),
            ColumnBuilder(s_ValueSetTable_Column_Type, Type::Int).NotNull(),
            ColumnBuilder(s_ValueSetTable_Column_Value, Type::None),
        });

        tableBuilder.Execute(m_connection);

        StatementBuilder indexBuilder;
        indexBuilder.CreateIndex(s_ValueSetTable_ValueSetRootRowIdIndex).On(s_ValueSetTable_Table).Columns(s_ValueSetTable_Column_ValueSetRootRowId);

        indexBuilder.Execute(m_connection);

        savepoint.Commit();
    }

    AppInstaller::SQLite::rowid_t ValueSetTable::Add(const winrt::Windows::Foundation::Collections::ValueSet& valueSet)
    {
        return Add(valueSet, std::nullopt);
    }

    AppInstaller::SQLite::rowid_t ValueSetTable::Add(const winrt::Windows::Foundation::Collections::ValueSet& valueSet, std::optional<rowid_t> valueSetRowId)
    {
        using namespace Windows::Foundation;

        Savepoint savepoint = Savepoint::Create(m_connection, "ValueSetTable_AddValueSet_0_1");

        Statement rowInsert = GetRowInsertStatement(m_connection);

        // Insert sentinel row
        rowInsert.Bind(1, nullptr);
        rowInsert.Bind(2, nullptr);
        rowInsert.Bind(3, ""sv);
        rowInsert.Bind(4, valueSet ? PropertyType::Inspectable : PropertyType::Empty);
        rowInsert.Bind(5, nullptr);

        rowInsert.Execute();
        rowid_t resultRowId = m_connection.GetLastInsertRowID();

        if (!valueSetRowId)
        {
            valueSetRowId = resultRowId;
        }
        rowInsert.Bind(1, valueSetRowId.value());

        if (valueSet)
        {
            for (const auto& value : valueSet)
            {
                rowInsert.Reset();

                rowInsert.Bind(2, resultRowId);
                rowInsert.Bind(3, ConvertToUTF8(value.Key()));

                IInspectable object = value.Value();

                if (Collections::ValueSet childValueSet = object.try_as<Collections::ValueSet>())
                {
                    rowInsert.Bind(4, PropertyType::Inspectable);
                    rowInsert.Bind(5, Add(childValueSet, valueSetRowId));
                }
                else if (IPropertyValue childValue = object.try_as<IPropertyValue>())
                {
                    PropertyType propertyType = childValue.Type();
                    rowInsert.Bind(4, propertyType);
                    constexpr static int32_t ArrayBaseValue = 1024;

                    if (AppInstaller::ToIntegral(propertyType) >= ArrayBaseValue)
                    {
                        switch (AppInstaller::ToEnum<PropertyType>(AppInstaller::ToIntegral(propertyType) - ArrayBaseValue))
                        {
                        case PropertyType::UInt8:
                            childValue.GetUInt8Array()
                            break;
                        case PropertyType::Int16:
                            break;
                        case PropertyType::UInt16:
                            break;
                        case PropertyType::Int32:
                            break;
                        case PropertyType::UInt32:
                            break;
                        case PropertyType::Int64:
                            break;
                        case PropertyType::UInt64:
                            break;
                        case PropertyType::Single:
                            break;
                        case PropertyType::Double:
                            break;
                        case PropertyType::Char16:
                            break;
                        case PropertyType::Boolean:
                            break;
                        case PropertyType::String:
                            break;
                        case PropertyType::Inspectable:
                            break;
                        case PropertyType::DateTime:
                            break;
                        case PropertyType::TimeSpan:
                            break;
                        case PropertyType::Guid:
                            break;
                        default:
                            THROW_WIN32(ERROR_NOT_SUPPORTED);
                        }
                    }
                    else
                    {
                        switch (propertyType)
                        {
                        case PropertyType::UInt8:
                            rowInsert.Bind(5, childValue.GetUInt8());
                            break;
                        case PropertyType::Int16:
                            rowInsert.Bind(5, childValue.GetInt16());
                            break;
                        case PropertyType::UInt16:
                            rowInsert.Bind(5, childValue.GetUInt16());
                            break;
                        case PropertyType::Int32:
                            break;
                        case PropertyType::UInt32:
                            break;
                        case PropertyType::Int64:
                            break;
                        case PropertyType::UInt64:
                            break;
                        case PropertyType::Single:
                            break;
                        case PropertyType::Double:
                            break;
                        case PropertyType::Char16:
                            break;
                        case PropertyType::Boolean:
                            break;
                        case PropertyType::String:
                            break;
                        case PropertyType::Inspectable:
                            break;
                        case PropertyType::DateTime:
                            break;
                        case PropertyType::TimeSpan:
                            break;
                        case PropertyType::Guid:
                            break;
                        default:
                            THROW_WIN32(ERROR_NOT_SUPPORTED);
                        }
                    }
                }
                else
                {
                    // ValueSet is only supposed to contain property values and value sets
                    THROW_HR(E_UNEXPECTED);
                }

                rowInsert.Execute();
            }
        }

        savepoint.Commit();
        return resultRowId;
    }

    AppInstaller::SQLite::rowid_t ValueSetTable::Add(const winrt::Windows::Foundation::Collections::IVector<winrt::hstring>& values)
    {

    }

    winrt::Windows::Foundation::Collections::ValueSet ValueSetTable::GetValueSet(AppInstaller::SQLite::rowid_t rootRowId)
    {

    }
}
