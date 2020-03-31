// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerRepositorySource.h>
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"

#include <map>
#include <utility>
#include <variant>

namespace AppInstaller::CLI::Execution
{
    // Names a peice of data stored in the context by a workflow step.
    // Must start at 0 to enable direct access to variant in Context.
    enum class Data : size_t
    {
        Source,
        SearchResult,
        Max
    };

    namespace details
    {
        template <Data D>
        struct DataMapping
        {
            // value_t type specifies the type of this data
        };

        template <>
        struct DataMapping<Data::Source>
        {
            using value_t = std::shared_ptr<Repository::ISource>;
        };

        template <>
        struct DataMapping<Data::SearchResult>
        {
            using value_t = Repository::SearchResult;
        };

        // Used to deduce the DataVariant type; making a variant that includes std::monostate and all DataMapping types.
        template <size_t... I>
        inline auto Deduce(std::index_sequence<I...>) { return std::variant<std::monostate, DataMapping<static_cast<Data>(I)>::value_t...>{}; }

        // Holds data of any type listed in a DataMapping.
        using DataVariant = decltype(Deduce(std::make_index_sequence<static_cast<size_t>(Data::Max)>()));

        // Gets the index into the variant for the given Data.
        constexpr inline size_t DataIndex(Data d) { return static_cast<size_t>(d) + 1; }
    }

    // The context within which all commands execute.
    // Contains input/output via Execution::Reporter and
    // arguments via Execution::Args.
    struct Context
    {
        Context(std::ostream& out, std::istream& in) : Reporter(out, in) {}

        // The path for console input/output for all functionality.
        Reporter Reporter;

        // The arguments given to execute with.
        Args Args;

        // Adds a value to the context data, or overwrites an existing entry.
        // This must be used to create the intial data entry, but Get can be used to modify.
        template <Data D>
        void Add(typename details::DataMapping<D>::value_t&& v)
        {
            m_data[D].emplace<details::DataIndex(D)>(std::forward<typename details::DataMapping<D>::value_t>(v));
        }

        // Gets context data; which can be modified in place.
        template <Data D>
        typename details::DataMapping<D>::value_t& Get()
        {
            auto itr = m_data.find(D);
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), itr == m_data.end());
            return std::get<details::DataIndex(D)>(itr->second);
        }

    private:
        std::map<Data, details::DataVariant> m_data;
    };
}