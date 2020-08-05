// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <pch.h>
#include "winget/Yaml.h"
#include "YamlWrapper.h"
#include "AppInstallerErrors.h"


namespace AppInstaller::YAML
{
    namespace
    {
        Node s_globalInvalidNode;

        Node::Type ConvertType(yaml_node_type_t type)
        {
            switch (type)
            {
            case YAML_NO_NODE:
                return Node::Type::None;
            case YAML_SCALAR_NODE:
                return Node::Type::Scalar;
            case YAML_SEQUENCE_NODE:
                return Node::Type::Sequence;
            case YAML_MAPPING_NODE:
                return Node::Type::Mapping;
            }

            THROW_HR(E_UNEXPECTED);
        }

        std::string ConvertYamlString(yaml_char_t* string, size_t length = std::string::npos)
        {
            if (length == std::string::npos)
            {
                return { reinterpret_cast<char*>(string) };
            }
            else
            {
                return { reinterpret_cast<char*>(string), length };
            }
        }

        std::string ConvertScalarToString(yaml_node_t* node)
        {
            return ConvertYamlString(node->data.scalar.value, node->data.scalar.length);
        }

        Node GetDocumentRoot(Document& document)
        {
            yaml_node_t* root = yaml_document_get_root_node(&document);

            if (!root)
            {
                return {};
            }

            Node result(ConvertType(root->type), ConvertYamlString(root->tag));

            struct StackItem
            {
                StackItem(yaml_node_t* yn, Node* n) :
                    yamlNode(yn), node(n) {}

                yaml_node_t* yamlNode = nullptr;
                Node* node = nullptr;
                size_t childOffset = 0;
            };

            std::stack<StackItem> resultStack;
            resultStack.emplace(root, &result);

            while (!resultStack.empty())
            {
                StackItem& stackItem = resultStack.top();
                bool pop = false;

                switch (stackItem.yamlNode->type)
                {
                case YAML_NO_NODE:
                    pop = true;
                    break;
                case YAML_SCALAR_NODE:
                    stackItem.node->SetScalar(ConvertScalarToString(stackItem.yamlNode));
                    pop = true;
                    break;
                case YAML_SEQUENCE_NODE:
                {
                    yaml_node_item_t* child = stackItem.yamlNode->data.sequence.items.start + stackItem.childOffset++;
                    if (child < stackItem.yamlNode->data.sequence.items.top)
                    {
                        yaml_node_t* childYamlNode = document.GetNode(*child);
                        Node& childNode = stackItem.node->AddSequenceNode(ConvertType(childYamlNode->type), ConvertYamlString(childYamlNode->tag));
                        resultStack.emplace(childYamlNode, &childNode);
                    }
                    else
                    {
                        // We've reached the end of the sequence
                        pop = true;
                    }
                    break;
                }
                case YAML_MAPPING_NODE:
                {
                    yaml_node_pair_t* child = stackItem.yamlNode->data.mapping.pairs.start + stackItem.childOffset++;
                    if (child < stackItem.yamlNode->data.mapping.pairs.top)
                    {
                        yaml_node_t* keyYamlNode = document.GetNode(child->key);
                        THROW_HR_IF(E_UNEXPECTED, keyYamlNode->type != YAML_SCALAR_NODE);

                        Node keyNode(ConvertType(keyYamlNode->type), ConvertYamlString(keyYamlNode->tag));
                        keyNode.SetScalar(ConvertScalarToString(keyYamlNode));

                        yaml_node_t* valueYamlNode = document.GetNode(child->value);

                        Node& childNode = stackItem.node->AddMappingNode(std::move(keyNode), ConvertType(valueYamlNode->type), ConvertYamlString(valueYamlNode->tag));
                        resultStack.emplace(valueYamlNode, &childNode);
                    }
                    else
                    {
                        // We've reached the end of the mapping
                        pop = true;
                    }
                    break;
                }
                }

                if (pop)
                {
                    resultStack.pop();
                }
            }

            return result;
        }
    }

    Node::Node(Type type, std::string tag) :
        m_type(type), m_tag(std::move(tag))
    {
        if (m_type == Type::Sequence)
        {
            m_sequence = decltype(m_sequence)::value_type{};
        }
        else if (m_type == Type::Mapping)
        {
            m_mapping = decltype(m_mapping)::value_type{};
        }
    }

    void Node::SetScalar(std::string value)
    {
        Require(Type::Scalar);
        m_scalar = std::move(value);
    }

    bool Node::operator<(const Node& other) const
    {
        Require(Type::Scalar);
        other.Require(Type::Scalar);
        return this->m_scalar < other.m_scalar;
    }

    Node& Node::operator[](std::string_view key)
    {
        Require(Type::Mapping);
        auto itr = m_mapping->find(key);
        return (itr == m_mapping->end() ? s_globalInvalidNode : itr->second);
    }

    const Node& Node::operator[](std::string_view key) const
    {
        Require(Type::Mapping);
        auto itr = m_mapping->find(key);
        return (itr == m_mapping->end() ? s_globalInvalidNode : itr->second);
    }

    Node& Node::operator[](size_t index)
    {
        Require(Type::Sequence);
        return m_sequence.value()[index];
    }

    const Node& Node::operator[](size_t index) const
    {
        Require(Type::Sequence);
        return m_sequence.value()[index];
    }

    size_t Node::size() const
    {
        RequireCollection();

        if (m_type == Type::Sequence)
        {
            return m_sequence->size();
        }
        else if (m_type == Type::Mapping)
        {
            return m_mapping->size();
        }

        THROW_HR(E_UNEXPECTED);
    }

    const std::vector<Node>& Node::Sequence() const
    {
        Require(Type::Sequence);
        return m_sequence.value();
    }

    const std::map<Node, Node>& Node::Mapping() const
    {
        Require(Type::Mapping);
        return m_mapping.value();
    }

    void Node::Require(Type type) const
    {
        THROW_HR_IF(E_UNEXPECTED, m_type != type);
    }

    void Node::RequireCollection() const
    {
        THROW_HR_IF(E_UNEXPECTED, m_type != Type::Sequence && m_type != Type::Mapping);
    }

    std::string Node::as_dispatch(std::string*) const
    {
        Require(Type::Scalar);
        return m_scalar;
    }

    int64_t Node::as_dispatch(int64_t*) const
    {
        Require(Type::Scalar);
        return std::stoll(m_scalar);
    }

    bool Node::as_dispatch(bool*) const
    {
        Require(Type::Scalar);
        return m_scalar == "true";
    }

    Node Load(std::string_view input)
    {
        Parser parser(input);
        Document document = parser.Load();

        if (document.HasRoot())
        {
            return GetDocumentRoot(document);
        }
        else
        {
            return {};
        }
    }

    Node Load(std::istream& input)
    {
        Parser parser(input);
        Document document = parser.Load();

        if (document.HasRoot())
        {
            return GetDocumentRoot(document);
        }
        else
        {
            return {};
        }
    }

    Emitter& Emitter::operator<<(EmitterEvent event)
    {
        UNREFERENCED_PARAMETER(event);
        THROW_HR(E_NOTIMPL);
    }

    Emitter& Emitter::operator<<(std::string_view value)
    {
        UNREFERENCED_PARAMETER(value);
        THROW_HR(E_NOTIMPL);
    }

    Emitter& Emitter::operator<<(bool value)
    {
        UNREFERENCED_PARAMETER(value);
        THROW_HR(E_NOTIMPL);
    }

    std::string Emitter::str() const
    {
        THROW_HR(E_NOTIMPL);
    }
}
