// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::YAML
{
    // A location within the stream.
    struct Mark
    {
        Mark() = default;

        int line = -1;
        int column = -1;
    };

    // A YAML node.
    struct Node
    {
        // The node's type.
        enum class Type
        {
            Invalid,
            None,
            Scalar,
            Sequence,
            Mapping
        };

        Node() : m_type(Type::Invalid) {}
        Node(Type type, std::string tag);

        void SetScalar(std::string value);

        template <typename... Args>
        Node& AddSequenceNode(Args&&... args)
        {
            Require(Type::Sequence);
            return m_sequence->emplace_back(std::forward<Args>(args)...);
        }

        template <typename... Args>
        Node& AddMappingNode(Node&& key, Args&&... args)
        {
            Require(Type::Mapping);
            auto result = m_mapping->emplace(std::move(key), Node(std::forward<Args>(args)...));
            // TODO: Handle repeat key better
            THROW_HR_IF(E_UNEXPECTED, !result.second);
            return result.first->second;
        }

        bool IsDefined() const { return m_type != Type::Invalid; }
        bool IsNull() const { return m_type == Type::Invalid || m_type == Type::None; }
        bool IsScalar() const { return m_type == Type::Scalar; }
        bool IsSequence() const { return m_type == Type::Sequence; }
        bool IsMap() const { return m_type == Type::Mapping; }

        explicit operator bool() const { return IsDefined(); }

        template <typename T>
        T as() const
        {
            return as_dispatch(reinterpret_cast<T*>(nullptr));
        }

        bool operator<(const Node& other) const;

        Node& operator[](std::string_view key);
        const Node& operator[](std::string_view key) const;

        Node& operator[](size_t index);
        const Node& operator[](size_t index) const;

        size_t size() const;

        // TODO: Figure out Marks
        Mark Mark() const { return m_mark; }

        const std::vector<Node>& Sequence() const;
        const std::map<Node, Node>& Mapping() const;

    private:
        Node(std::string_view key) : m_type(Type::Scalar), m_scalar(key) {}

        void Require(Type type) const;
        void RequireCollection() const;

        std::string as_dispatch(std::string*) const;
        int64_t as_dispatch(int64_t*) const;
        bool as_dispatch(bool*) const;

        Type m_type;
        std::string m_tag;
        YAML::Mark m_mark;
        std::string m_scalar;
        std::optional<std::vector<Node>> m_sequence;
        std::optional<std::map<Node, Node>> m_mapping;
    };

    Node Load(std::string_view input);
    Node Load(std::istream& input);

    // Any emitter event.
    // Not using enum class to enable current code to work.
    enum EmitterEvent
    {
        BeginSeq,
        EndSeq,
        BeginMap,
        EndMap,
        Key,
        Value,
    };

    // A YAML emitter.
    struct Emitter
    {
        Emitter& operator<<(EmitterEvent event);
        Emitter& operator<<(std::string_view value);
        Emitter& operator<<(bool value);

        std::string str() const;
    };
}
