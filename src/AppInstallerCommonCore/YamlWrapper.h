// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <yaml.h>
#include "winget/Yaml.h"
#include "AppInstallerLanguageUtilities.h"

#include <iostream>
#include <string_view>
#include <vector>


namespace AppInstaller::YAML::Wrapper
{
    // A libyaml yaml_document_t.
    // A parsed document, created by the Parser.
    struct Document
    {
        // Initializes the document.
        Document(bool init = false);

        Document(const Document&) = delete;
        Document& operator=(const Document&) = delete;

        Document(Document&&) noexcept = default;
        Document& operator=(Document&&) noexcept = delete;

        ~Document();

        yaml_document_t* operator&() { return &m_document; }

        // Indicates that the document should not be deleted, as
        // it has been handed off to the emitter.
        void Detach() { m_token = false; }

        // Determines whether the document has a root node.
        bool HasRoot();

        // Gets the root node of the document, if it has one.
        Node GetRoot();

        // Adds a scalar node to the document.
        int AddScalar(std::string_view value);

        // Adds a sequence node to the document.
        int AddSequence();

        // Adds a mapping node to the document.
        int AddMapping();

        // Appends a node to the end of the sequence.
        void AppendSequenceItem(int sequence, int item);

        // Adds a pair to the mapping.
        void AppendMappingPair(int mapping, int key, int value);

    private:
        // Gets the node referenced by the index.
        yaml_node_t* GetNode(yaml_node_item_t index);

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
        Parser& operator=(Parser&&) noexcept = delete;

        ~Parser();

        yaml_parser_t* operator&() { return &m_parser; }

        // Loads the next document from the input, if one exists.
        Document Load();

    private:
        // Determines the type of encoding in use, transforming the input as necessary.
        void PrepareInput();

        DestructionToken m_token;
        yaml_parser_t m_parser;
        std::string m_input;
    };

    // A libyaml yaml_event_t.
    // The generic event type for.
    struct Event
    {
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;

        Event(Event&&) noexcept = default;
        Event& operator=(Event&&) noexcept = delete;

        ~Event();

        yaml_event_t* operator&() { return &m_event; }

        // Indicates that the event should not be deleted, as
        // it has been handed off to the emitter.
        void Detach() { m_token = false; }

        // Event creation functions.
        static Event StreamStart();
        static Event StreamEnd();
        static Event DocumentStart();
        static Event DocumentEnd();
        static Event SequenceStart();
        static Event SequenceEnd();
        static Event MappingStart();
        static Event MappingEnd();

    private:
        Event() = default;

        DestructionToken m_token;
        yaml_event_t m_event = {};
    };

    // A libyaml yaml_emitter_t.
    // Allows YAML to be written out.
    struct Emitter
    {
        Emitter(std::ostream& output);

        Emitter(const Emitter&) = delete;
        Emitter& operator=(const Emitter&) = delete;

        Emitter(Emitter&&) noexcept = default;
        Emitter& operator=(Emitter&&) noexcept = delete;

        ~Emitter();

        yaml_emitter_t* operator&() { return &m_emitter; }

        // Emits and event.
        void Emit(Event& event);
        void Emit(Event&& event);

        // Dumps a document to the emitter.
        void Dump(Document& document);

        // Flushes the emitter.
        void Flush();

    private:
        static int StreamWriteHandler(
            void* data,
            unsigned char* buffer,
            size_t size);

        void ThrowError();

        DestructionToken m_token;
        yaml_emitter_t m_emitter;
        std::ostream* m_outputStream = nullptr;
    };
}
