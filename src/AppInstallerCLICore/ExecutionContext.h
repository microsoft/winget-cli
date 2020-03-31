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
    // Max must be last and unused.
    enum class Data : size_t
    {
        Source,
        SearchResult,
        SourceList,
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

        template <>
        struct DataMapping<Data::SourceList>
        {
            using value_t = std::vector<Repository::SourceDetails>;
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

        // Returns a value indicating whether the context is terminated.
        bool IsTerminated() const { return m_isTerminated; }

        // Set the context to the terminated state.
        void Terminate() { m_isTerminated = true; }

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
        bool m_isTerminated = false;
        std::map<Data, details::DataVariant> m_data;
    };
}

// Passes the context to Callable T if it has not been terminated; returns the context.
template <typename T>
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, T&& t)
{
    if (!context.IsTerminated())
    {
        t(context);
    }
    return context;
}
