// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <map>

#include "Common.h"

namespace AppInstaller::CLI
{
    struct Invocation
    {
        using ArgString = wchar_t const*;

        Invocation(int argc, ArgString* argv) : m_argc(argc), m_argv(argv) {}

        struct iterator
        {
            iterator(int arg, ArgString* argv) : m_arg(arg), m_argv(argv) {}

            iterator(const iterator&) = default;
            iterator& operator=(const iterator&) = default;

            iterator operator++() { return { ++m_arg, m_argv }; }
            iterator operator++(int) { return { m_arg++, m_argv }; }
            iterator operator--() { return { --m_arg, m_argv }; }
            iterator operator--(int) { return { m_arg--, m_argv }; }

            bool operator==(const iterator& other) { return m_arg == other.m_arg; }
            bool operator!=(const iterator& other) { return m_arg != other.m_arg; }

            ArgString operator*() const { return m_argv[m_arg]; }

            int index() const { return m_arg; }

        private:
            int m_arg;
            ArgString* m_argv;
        };

        iterator begin() { return { m_currentFirstArg, m_argv }; }
        iterator end() { return { m_argc, m_argv }; }
        void consume(const iterator& i) { m_currentFirstArg = i.index() + 1; }

        bool Contains(StringLiteralPtrRef name) const { return (m_parsedArgs.count(name) != 0); }
        const std::vector<std::wstring>* GetArgs(StringLiteralPtrRef name) const
        {
            auto itr = m_parsedArgs.find(name);
            return (itr == m_parsedArgs.end() ? nullptr : &(itr->second));
        }
        size_t GetCount(StringLiteralPtrRef name) const
        {
            auto args = GetArgs(name);
            return (args ? args->size() : 0);
        }

        bool AddArg(StringLiteralPtrRef name) { return m_parsedArgs[name].empty(); }
        void AddArg(StringLiteralPtrRef name, std::wstring value) { m_parsedArgs[name].emplace_back(std::move(value)); }

    private:
        int m_argc;
        ArgString* m_argv;
        int m_currentFirstArg = 1;

        std::map<std::wstring, std::vector<std::wstring>> m_parsedArgs;
    };
}
