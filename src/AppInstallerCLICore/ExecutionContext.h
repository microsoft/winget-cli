// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerRepositorySource.h>
#include <Manifest/Manifest.h>
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"
#include "CompletionData.h"

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>


// Terminates the Context with some logging to indicate the location.
// Also returns from the current function.
#define AICLI_TERMINATE_CONTEXT_ARGS(_context_,_hr_) \
    do { \
        HRESULT AICLI_TERMINATE_CONTEXT_ARGS_hr = _hr_; \
        _context_.Terminate(AICLI_TERMINATE_CONTEXT_ARGS_hr, __FILE__, __LINE__); \
        return; \
    } while(0,0)

// Terminates the Context namd 'context' with some logging to indicate the location.
// Also returns from the current function.
#define AICLI_TERMINATE_CONTEXT(_hr_)   AICLI_TERMINATE_CONTEXT_ARGS(context,_hr_)

namespace AppInstaller::CLI::Workflow
{
    struct WorkflowTask;
}

namespace AppInstaller::CLI::Execution
{
    // Names a piece of data stored in the context by a workflow step.
    // Must start at 0 to enable direct access to variant in Context.
    // Max must be last and unused.
    enum class Data : size_t
    {
        Source,
        SearchResult,
        SourceList,
        Manifest,
        Installer,
        HashPair,
        InstallerPath,
        LogPath,
        InstallerArgs,
        CompletionData,
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

        template <>
        struct DataMapping<Data::Manifest>
        {
            using value_t = Manifest::Manifest;
        };

        template <>
        struct DataMapping<Data::Installer>
        {
            using value_t = std::optional<Manifest::ManifestInstaller>;
        };

        template <>
        struct DataMapping<Data::HashPair>
        {
            using value_t = std::pair<std::vector<uint8_t>, std::vector<uint8_t>>;
        };

        template <>
        struct DataMapping<Data::InstallerPath>
        {
            using value_t = std::filesystem::path;
        };

        template <>
        struct DataMapping<Data::LogPath>
        {
            using value_t = std::filesystem::path;
        };

        template <>
        struct DataMapping<Data::InstallerArgs>
        {
            using value_t = std::string;
        };

        template <>
        struct DataMapping<Data::CompletionData>
        {
            using value_t = CLI::CompletionData;
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

        virtual ~Context();

        // The path for console input/output for all functionality.
        Reporter Reporter;

        // The arguments given to execute with.
        Args Args;

        // Creates a copy of this context as it was at construction.
        Context Clone();

        // Enables reception of CTRL signals.
        // Only one context can be enabled to handle CTRL signals at a time.
        void EnableCtrlHandler(bool enabled = true);

        // Applies changes based on the parsed args.
        void UpdateForArgs();

        // Returns a value indicating whether the context is terminated.
        bool IsTerminated() const { return m_isTerminated; }

        // Gets the HRESULT reason for the termination.
        HRESULT GetTerminationHR() const { return m_terminationHR; }

        // Set the context to the terminated state.
        void Terminate(HRESULT hr, std::string_view file = {}, size_t line = {});

        // Adds a value to the context data, or overwrites an existing entry.
        // This must be used to create the initial data entry, but Get can be used to modify.
        template <Data D>
        void Add(typename details::DataMapping<D>::value_t&& v)
        {
            m_data[D].emplace<details::DataIndex(D)>(std::forward<typename details::DataMapping<D>::value_t>(v));
        }

        // Return a value indicating whether the given data type is stored in the context.
        bool Contains(Data d) { return (m_data.find(d) != m_data.end()); }

        // Gets context data; which can be modified in place.
        template <Data D>
        typename details::DataMapping<D>::value_t& Get()
        {
            auto itr = m_data.find(D);
            THROW_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), itr == m_data.end(), "Get(%d)", D);
            return std::get<details::DataIndex(D)>(itr->second);
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Enable tests to override behavior
        virtual bool ShouldExecuteWorkflowTask(const Workflow::WorkflowTask&) { return true; }
#endif

    private:
        // Clone the reporter for this constructor.
        Context(Execution::Reporter& reporter) : Reporter(reporter, Execution::Reporter::clone_t{}) {}

        DestructionToken m_disableCtrlHandlerOnExit = false;
        bool m_isTerminated = false;
        HRESULT m_terminationHR = S_OK;
        std::map<Data, details::DataVariant> m_data;
        size_t m_CtrlSignalCount = 0;
    };
}
