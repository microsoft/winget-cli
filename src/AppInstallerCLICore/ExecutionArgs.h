// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <map>
#include <vector>

namespace AppInstaller::CLI
{
    struct ExecutionArgs
    {
        enum class ExecutionArgType
        {
            // Args to specify where to get app
            Query, // Query to be performed against index
            Manifest, // Provide the app manifest directly

            // Query filtering criteria and query behavior
            Id,
            Name,
            Moniker,
            Tag,
            Command,
            Source, // Index source to be queried against
            Count, // Maximun query results
            Exact, // Exact match required

            // Manifest selection behavior after an app is found
            Version,
            Channel,

            // Install behavior
            Interactive,
            Silent,
            Language,
            Log,
            Override, //Override args are (and the only args) directly passed to installer
            InstallLocation,

            // Other
            ListVersions, // Used in Show command to list all available versions of an app
        };

        const std::vector<std::string>* GetArgs(ExecutionArgType arg) const
        {
            auto itr = m_parsedArgs.find(arg);
            return (itr == m_parsedArgs.end() ? nullptr : &(itr->second));
        }

        const std::string* GetArg(ExecutionArgType arg) const
        {
            auto itr = m_parsedArgs.find(arg);

            if (itr == m_parsedArgs.end())
            {
                return nullptr;
            }

            return &(itr->second[0]);
        }

        size_t GetCount(ExecutionArgType arg) const
        {
            auto args = GetArgs(arg);
            return (args ? args->size() : 0);
        }

        bool AddArg(ExecutionArgType arg)
        {
            return m_parsedArgs[arg].empty();
        }
        void AddArg(ExecutionArgType arg, std::string value)
        {
            m_parsedArgs[arg].emplace_back(std::move(value));
        }

    private:
        std::map<ExecutionArgType, std::vector<std::string>> m_parsedArgs;
    };
}
