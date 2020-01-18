// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <map>
#include <vector>

namespace AppInstaller::CLI
{
    struct Invocation
    {
        using ArgString = char const*;

        Invocation(std::vector<std::string>&& args) : m_args(std::move(args)) {}

        struct iterator
        {
            iterator(size_t arg, std::vector<std::string>& args) : m_arg(arg), m_args(args) {}

            iterator(const iterator&) = default;
            iterator& operator=(const iterator&) = default;

            iterator operator++() { return { ++m_arg, m_args }; }
            iterator operator++(int) { return { m_arg++, m_args }; }
            iterator operator--() { return { --m_arg, m_args }; }
            iterator operator--(int) { return { m_arg--, m_args }; }

            bool operator==(const iterator& other) { return m_arg == other.m_arg; }
            bool operator!=(const iterator& other) { return m_arg != other.m_arg; }

            const std::string& operator*() const { return m_args[m_arg]; }
            const std::string* operator->() const { return &(m_args[m_arg]); }

            size_t index() const { return m_arg; }

        private:
            size_t m_arg;
            std::vector<std::string>& m_args;
        };

        iterator begin() { return { m_currentFirstArg, m_args }; }
        iterator end() { return { m_args.size(), m_args }; }
        void consume(const iterator& i) { m_currentFirstArg = i.index() + 1; }

        bool Contains(std::string_view name) const { return (m_parsedArgs.count(name) != 0); }
        const std::vector<std::string>* GetArgs(std::string_view name) const
        {
            auto itr = m_parsedArgs.find(name);
            return (itr == m_parsedArgs.end() ? nullptr : &(itr->second));
        }

        const std::string* GetArg(std::string_view name) const
        {
            auto itr = m_parsedArgs.find(name);

            if (itr == m_parsedArgs.end())
            {
                return nullptr;
            }

            if (itr->second.size() != 1)
            {
                throw std::runtime_error("The requested arg does not have exactly 1 value. Use GetArgs() instead.");
            }

            return &(itr->second[0]);
        }

        size_t GetCount(std::string_view name) const
        {
            auto args = GetArgs(name);
            return (args ? args->size() : 0);
        }

        bool AddArg(std::string_view name)
        {
            AICLI_LOG(CLI, Verbose, << "Found flag: " << name);
            return m_parsedArgs[name].empty();
        }
        void AddArg(std::string_view name, std::string value)
        {
            AICLI_LOG(CLI, Verbose, << "Found argument with value: " << name << " => " << value);
            m_parsedArgs[name].emplace_back(std::move(value));
        }

    private:
        std::vector<std::string> m_args;
        size_t m_currentFirstArg = 0;

        std::map<std::string_view, std::vector<std::string>> m_parsedArgs;
    };
}
