// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <yaml.h>
#include "AppInstallerLanguageUtilities.h"

#include <iostream>
#include <string_view>
#include <vector>


namespace AppInstaller::YAML
{
    // A libyaml yaml_document_t.
    // A parsed document, created by the Parser.
    struct Document
    {
        Document();

        Document(const Document&) = delete;
        Document& operator=(const Document&) = delete;

        Document(Document&&) noexcept = default;
        Document& operator=(Document&&) noexcept = default;

        ~Document();

        yaml_document_t* operator&() { return &m_document; }

        // Determines whether the document has a root node.
        bool HasRoot();

        // Gets the node referenced by the index.
        yaml_node_t* GetNode(yaml_node_item_t index);

    private:

        DestructionToken m_token;
        yaml_document_t m_document;
    };

    // A libyaml yaml_parser_t.
    // The core parser construct for reading bytes directly.
    struct Parser
    {
        Parser(std::string_view input);
        Parser(std::istream& input);

        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;

        Parser(Parser&&) noexcept = default;
        Parser& operator=(Parser&&) noexcept = default;

        ~Parser();

        yaml_parser_t* operator&() { return &m_parser; }

        // Loads the next document from the input, if one exists.
        Document Load();

    private:
        static int StreamReadHandler(
            void* data,
            unsigned char* buffer,
            size_t size,
            size_t* size_read);

        DestructionToken m_token;
        yaml_parser_t m_parser;
        std::optional<std::vector<unsigned char>> m_inputBytes;
        std::istream* m_inputStream = nullptr;
    };
}
