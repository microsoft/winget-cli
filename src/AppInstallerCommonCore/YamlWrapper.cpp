// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <pch.h>
#include "YamlWrapper.h"
#include "AppInstallerErrors.h"


namespace AppInstaller::YAML
{
    Document::Document() :
        m_token(true), m_document{}
    {
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

    yaml_node_t* Document::GetNode(yaml_node_item_t index)
    {
        yaml_node_t* result = yaml_document_get_node(&m_document, index);
        THROW_HR_IF(E_UNEXPECTED, !result);
        return result;
    }

    Parser::Parser(std::string_view input) : m_token(true)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_PARSER_INIT_FAILED, yaml_parser_initialize(&m_parser) == 0);

        std::vector<unsigned char> bytes;
        std::copy(input.begin(), input.end(), bytes.begin());
        m_inputBytes = std::move(bytes);

        yaml_parser_set_input_string(&m_parser, m_inputBytes->data(), m_inputBytes->size());
    }

    Parser::Parser(std::istream& input) : m_token(true), m_inputStream(&input)
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_PARSER_INIT_FAILED, yaml_parser_initialize(&m_parser) == 0);

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

        // TODO: Check result and throw
        yaml_parser_load(&m_parser, &result);

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
}
