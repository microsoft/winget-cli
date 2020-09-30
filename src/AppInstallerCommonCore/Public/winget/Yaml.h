// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::YAML
{
    // A location within the stream.
    struct Mark
    {
        Mark() = default;
        Mark(size_t l, size_t c) : line(l), column(c) {}

        size_t line = 0;
        size_t column = 0;
    };

    // An exception from YAML.
    struct Exception : public wil::ResultException
    {
        // The type of error that occurred.
        enum class Type
        {
            None,
            Memory,
            Reader,
            Scanner,
            Parser,
            Composer,
            Writer,
            Emitter
        };

        // Should only be used for Memory.
        Exception(Type type);

        // Should only be used for Reader.
        Exception(Type type, const char* problem, size_t offset, int value);

        // Used for Scanner, Parser, and Composer.
        Exception(Type type, const char* problem, const Mark& problemMark, const char* context = {}, const Mark& contextMark = {});

        // Used for Writer and Emitter.
        Exception(Type type, const char* problem);

        const char* what() const noexcept override;

    private:
        std::string m_what;
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
        Node(Type type, std::string tag, const Mark& mark);

        // Sets the scalar value of the node.
        void SetScalar(std::string value);

        // Adds a child node to the sequence.
        template <typename... Args>
        Node& AddSequenceNode(Args&&... args)
        {
            Require(Type::Sequence);
            return m_sequence->emplace_back(std::forward<Args>(args)...);
        }

        // Adds a child node to the mapping.
        template <typename... Args>
        Node& AddMappingNode(Node&& key, Args&&... args)
        {
            Require(Type::Mapping);
            return m_mapping->emplace(std::move(key), Node(std::forward<Args>(args)...))->second;
        }

        bool IsDefined() const { return m_type != Type::Invalid; }
        bool IsNull() const { return m_type == Type::Invalid || m_type == Type::None || (m_type == Type::Scalar && m_scalar.empty()); }
        bool IsScalar() const { return m_type == Type::Scalar; }
        bool IsSequence() const { return m_type == Type::Sequence; }
        bool IsMap() const { return m_type == Type::Mapping; }

        explicit operator bool() const { return IsDefined(); }

        // Gets the scalar value as the requested type.
        template <typename T>
        T as() const
        {
            Require(Type::Scalar);
            T* t = nullptr;
            return as_dispatch(t);
        }

        bool operator<(const Node& other) const;

        // Gets a child node from the mapping by its name.
        Node& operator[](std::string_view key);
        const Node& operator[](std::string_view key) const;

        // Gets a child node from the sequence by its index.
        Node& operator[](size_t index);
        const Node& operator[](size_t index) const;

        // Gets the number of child nodes.
        size_t size() const;

        // Gets the mark for this node.
        const Mark& Mark() const { return m_mark; }

        // Gets the nodes in the sequence.
        const std::vector<Node>& Sequence() const;

        // Gets the nodes in the mapping.
        const std::multimap<Node, Node>& Mapping() const;

    private:
        Node(std::string_view key) : m_type(Type::Scalar), m_scalar(key) {}

        // Require certain node types to; throwing if the requirement is not met.
        void Require(Type type) const;

        // The workers for the as function.
        std::string as_dispatch(std::string*) const;
        int64_t as_dispatch(int64_t*) const;
        bool as_dispatch(bool*) const;

        Type m_type;
        std::string m_tag;
        YAML::Mark m_mark;
        std::string m_scalar;
        std::optional<std::vector<Node>> m_sequence;
        std::optional<std::multimap<Node, Node>> m_mapping;
    };

    // Loads from the input; returns the root node of the first document.
    Node Load(std::string_view input);
    Node Load(const std::string& input);
    Node Load(const std::filesystem::path& input);

    // Any emitter event.
    // Not using enum class to enable existing code to function.
    enum EmitterEvent
    {
        BeginSeq,
        EndSeq,
        BeginMap,
        EndMap,
        Key,
        Value,
    };

    // Forward declaration to allow pImpl in this Emitter.
    namespace Wrapper
    {
        struct Document;
    }

    // A YAML emitter.
    struct Emitter
    {
        Emitter();

        Emitter(const Emitter&) = delete;
        Emitter& operator=(const Emitter&) = delete;

        Emitter(Emitter&&);
        Emitter& operator=(Emitter&&);

        ~Emitter();

        // Emit events and values.
        Emitter& operator<<(EmitterEvent event);
        Emitter& operator<<(std::string_view value);
        Emitter& operator<<(int64_t value);
        Emitter& operator<<(bool value);

        // Gets the result of the emitter; can only be retrieved once.
        std::string str();

    private:
        // Appends the given node to the current container if applicable.
        void AppendNode(int id);

        std::unique_ptr<Wrapper::Document> m_document;

        // If set, stores the last Key that was set.
        std::optional<int> m_keyId;

        struct ContainerInfo
        {
            ContainerInfo(int id, bool map) : Id(id), IsMapping(map) {}

            int Id;
            bool IsMapping;
        };

        // The stack of containers being emitted.
        std::stack<ContainerInfo> m_containers;

        // *** State Machine ***

        // The type of input coming into the emitter.
        enum class InputType
        {
            Scalar,
            BeginSeq,
            EndSeq,
            BeginMap,
            EndMap,
            Key,
            Value,
        };

        // If set, defines the type of the next scalar (Key or Value).
        std::optional<InputType> m_scalarInfo;

        // Converts the intput type to a bitmask value.
        size_t GetInputBitmask(InputType type);

        // Checks the state of the emitter to ensure that the incoming value is acceptable.
        void CheckInput(InputType type);

        // The currently allowed input types.
        size_t m_allowedInputs = 0;

        template <InputType... types>
        void SetAllowedInputs()
        {
            m_allowedInputs = (GetInputBitmask(types) | ...);
        }

        // Sets the allowed inputs for the container on the top of the stack.
        void SetAllowedInputsForContainer();
    };
}
