// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <pch.h>
#include "YamlWrapper.h"
#include "AppInstallerErrors.h"


namespace AppInstaller::YAML::Wrapper
{
    namespace
    {
        Node::Type ConvertNodeType(yaml_node_type_t type)
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

        Exception::Type ConvertErrorType(yaml_error_type_t type)
        {
            switch (type)
            {
            case YAML_NO_ERROR:
                return Exception::Type::None;
            case YAML_MEMORY_ERROR:
                return Exception::Type::Memory;
            case YAML_READER_ERROR:
                return Exception::Type::Reader;
            case YAML_SCANNER_ERROR:
                return Exception::Type::Scanner;
            case YAML_PARSER_ERROR:
                return Exception::Type::Parser;
            case YAML_COMPOSER_ERROR:
                return Exception::Type::Composer;
            case YAML_WRITER_ERROR:
                return Exception::Type::Writer;
            case YAML_EMITTER_ERROR:
                return Exception::Type::Emitter;
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

        Mark ConvertMark(const yaml_mark_t& mark)
        {
            return { static_cast<int>(mark.line + 1), static_cast<int>(mark.column + 1) };
        }
    }

    Document::Document(bool init) :
        m_token(init), m_document{}
    {
        if (init)
        {
            if (!yaml_document_initialize(&m_document, NULL, NULL, NULL, 1, 1))
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED);
            }
        }
    }

    Document::~Document()
    {
        if (m_token)
        {
            yaml_document_delete(&m_document);
        }
    }

    bool Document::HasRoot()
    {
        return yaml_document_get_root_node(&m_document) != nullptr;
    }

    Node Document::GetRoot()
    {
        yaml_node_t* root = yaml_document_get_root_node(&m_document);

        if (!root)
        {
            return {};
        }

        Node result(ConvertNodeType(root->type), ConvertYamlString(root->tag), ConvertMark(root->start_mark));

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
                    yaml_node_t* childYamlNode = GetNode(*child);
                    Node& childNode = stackItem.node->AddSequenceNode(ConvertNodeType(childYamlNode->type), ConvertYamlString(childYamlNode->tag), ConvertMark(childYamlNode->start_mark));
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
                    yaml_node_t* keyYamlNode = GetNode(child->key);
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_MAPPING_KEY, keyYamlNode->type != YAML_SCALAR_NODE);

                    Node keyNode(ConvertNodeType(keyYamlNode->type), ConvertYamlString(keyYamlNode->tag), ConvertMark(keyYamlNode->start_mark));
                    keyNode.SetScalar(ConvertScalarToString(keyYamlNode));

                    yaml_node_t* valueYamlNode = GetNode(child->value);

                    Node& childNode = stackItem.node->AddMappingNode(std::move(keyNode), ConvertNodeType(valueYamlNode->type), ConvertYamlString(valueYamlNode->tag), ConvertMark(valueYamlNode->start_mark));
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

    int Document::AddScalar(std::string_view value)
    {
        int result = yaml_document_add_scalar(&m_document, NULL, reinterpret_cast<const yaml_char_t*>(value.data()), static_cast<int>(value.size()), YAML_ANY_SCALAR_STYLE);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED, result == 0);
        return result;
    }

    int Document::AddSequence()
    {
        int result = yaml_document_add_sequence(&m_document, NULL, YAML_ANY_SEQUENCE_STYLE);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED, result == 0);
        return result;
    }

    int Document::AddMapping()
    {
        int result = yaml_document_add_mapping(&m_document, NULL, YAML_ANY_MAPPING_STYLE);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED, result == 0);
        return result;
    }

    void Document::AppendSequenceItem(int sequence, int item)
    {
        if (!yaml_document_append_sequence_item(&m_document, sequence, item))
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED);
        }
    }

    void Document::AppendMappingPair(int mapping, int key, int value)
    {
        if (!yaml_document_append_mapping_pair(&m_document, mapping, key, value))
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED);
        }
    }

    yaml_node_t* Document::GetNode(yaml_node_item_t index)
    {
        yaml_node_t* result = yaml_document_get_node(&m_document, index);
        THROW_HR_IF(E_BOUNDS, !result);
        return result;
    }

    Parser::Parser(std::string_view input) : m_token(true)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_parser_initialize(&m_parser));

        std::vector<unsigned char> bytes(input.length());
        std::copy(input.begin(), input.end(), bytes.begin());
        m_inputBytes = std::move(bytes);

        yaml_parser_set_input_string(&m_parser, m_inputBytes->data(), m_inputBytes->size());
    }

    Parser::Parser(std::istream& input) : m_token(true), m_inputStream(&input)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_parser_initialize(&m_parser));

        yaml_parser_set_input(&m_parser, StreamReadHandler, this);
    }

    Parser::~Parser()
    {
        if (m_token)
        {
            yaml_parser_delete(&m_parser);
        }
    }

    Document Parser::Load()
    {
        Document result;

        if (!yaml_parser_load(&m_parser, &result))
        {
            Exception::Type type = ConvertErrorType(m_parser.error);

            switch (type)
            {
            case Exception::Type::Memory:
                THROW_EXCEPTION(Exception(type));
            case Exception::Type::Reader:
                THROW_EXCEPTION(Exception(type, m_parser.problem, m_parser.problem_offset, m_parser.problem_value));
            case Exception::Type::Scanner:
            case Exception::Type::Parser:
            case Exception::Type::Composer:
                THROW_EXCEPTION(Exception(type, m_parser.problem, ConvertMark(m_parser.problem_mark), m_parser.context, ConvertMark(m_parser.context_mark)));
            default:
                THROW_EXCEPTION(Exception(type, "An unexpected error type occurred in Parser::Load"));
            }
        }

        return result;
    }

    // The read handler is called when the parser needs to read more bytes from the
    // source.The handler should write not more than size bytes to the
    // buffer.The number of written bytes should be set to the size_read variable.
    //
    // @param[in, out]   data        A pointer to an application data specified by
    // yaml_parser_set_input().
    // @param[out]      buffer      The buffer to write the data from the source.
    // @param[in]       size        The size of the buffer.
    // @param[out]      size_read   The actual number of bytes read from the source.
    //
    // @returns On success, the handler should return 1.  If the handler failed,
    // the returned value should be 0.  On EOF, the handler should set the
    // size_read to 0 and return 1.
    int Parser::StreamReadHandler(
        void* data,
        unsigned char* buffer,
        size_t size,
        size_t* size_read)
    {
        Parser& parser = *reinterpret_cast<Parser*>(data);

        try
        {
            parser.m_inputStream->read(reinterpret_cast<char*>(buffer), size);
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            return 0;
        }

        *size_read = static_cast<size_t>(parser.m_inputStream->gcount());
        return 1;
    }

    Event::~Event()
    {
        if (m_token)
        {
            yaml_event_delete(&m_event);
        }
    }

    Event Event::StreamStart()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_stream_start_event_initialize(&result, YAML_UTF8_ENCODING));
        return result;
    }

    Event Event::StreamEnd()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_stream_end_event_initialize(&result));
        return result;
    }

    Event Event::DocumentStart()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_document_start_event_initialize(&result, NULL, NULL, NULL, 1));
        return result;
    }

    Event Event::DocumentEnd()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_document_end_event_initialize(&result, 1));
        return result;
    }

    Event Event::SequenceStart()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_sequence_start_event_initialize(&result, NULL, NULL, 1, YAML_ANY_SEQUENCE_STYLE));
        return result;
    }

    Event Event::SequenceEnd()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_sequence_end_event_initialize(&result));
        return result;
    }

    Event Event::MappingStart()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_mapping_start_event_initialize(&result, NULL, NULL, 1, YAML_ANY_MAPPING_STYLE));
        return result;
    }

    Event Event::MappingEnd()
    {
        Event result;
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_mapping_end_event_initialize(&result));
        return result;
    }

    Emitter::Emitter(std::ostream& output) :
        m_token(true), m_outputStream(&output)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED, !yaml_emitter_initialize(&m_emitter));
        yaml_emitter_set_output(&m_emitter, StreamWriteHandler, this);
        yaml_emitter_set_encoding(&m_emitter, YAML_UTF8_ENCODING);
    }

    Emitter::~Emitter()
    {
        if (m_token)
        {
            yaml_emitter_delete(&m_emitter);
        }
    }

    void Emitter::Emit(Event& event)
    {
        event.Detach();
        if (!yaml_emitter_emit(&m_emitter, &event))
        {
            ThrowError();
        }
    }

    void Emitter::Emit(Event&& event)
    {
        event.Detach();
        if (!yaml_emitter_emit(&m_emitter, &event))
        {
            ThrowError();
        }
    }

    void Emitter::Dump(Document& document)
    {
        document.Detach();
        if (!yaml_emitter_dump(&m_emitter, &document))
        {
            ThrowError();
        }
    }

    void Emitter::Flush()
    {
        if (!yaml_emitter_flush(&m_emitter))
        {
            ThrowError();
        }
    }

    int Emitter::StreamWriteHandler(
        void* data,
        unsigned char* buffer,
        size_t size)
    {
        Emitter& emitter = *reinterpret_cast<Emitter*>(data);

        try
        {
            emitter.m_outputStream->write(reinterpret_cast<char*>(buffer), size);
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            return 0;
        }

        return 1;
    }

    void Emitter::ThrowError()
    {
        Exception::Type type = ConvertErrorType(m_emitter.error);

        switch (type)
        {
        case Exception::Type::Memory:
            THROW_EXCEPTION(Exception(type));
        case Exception::Type::Emitter:
        case Exception::Type::Writer:
            THROW_EXCEPTION(Exception(type, m_emitter.problem));
        default:
            THROW_EXCEPTION(Exception(type, "An unexpected error type occurred in Emitter"));
        }
    }
}
