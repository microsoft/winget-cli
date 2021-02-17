// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <pch.h>
#include "winget/Yaml.h"
#include "YamlWrapper.h"
#include "AppInstallerErrors.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"


namespace AppInstaller::YAML
{
    using namespace std::string_view_literals;

    namespace
    {
        Node s_globalInvalidNode;

        std::string_view GetExceptionTypeStringView(Exception::Type type)
        {
            switch (type)
            {
            case Exception::Type::None:
                return "None"sv;
            case Exception::Type::Memory:
                return "Memory"sv;
            case Exception::Type::Reader:
                return "Reader"sv;
            case Exception::Type::Scanner:
                return "Scanner"sv;
            case Exception::Type::Parser:
                return "Parser"sv;
            case Exception::Type::Composer:
                return "Composer"sv;
            case Exception::Type::Writer:
                return "Writer"sv;
            case Exception::Type::Emitter:
                return "Emitter"sv;
            }

            return "Unknown"sv;
        }

        void OutputExceptionHeader(std::ostringstream& out, Exception::Type type)
        {
            out << "[YAML:" << GetExceptionTypeStringView(type) << "] ";
        }

        void OutputMark(std::ostringstream& out, const Mark& mark)
        {
            out << "[line " << mark.line << "; col " << mark.column << ']';
        }
    }

    Exception::Exception(Type type) :
        wil::ResultException(APPINSTALLER_CLI_ERROR_LIBYAML_ERROR)
    {
        std::ostringstream out;
        OutputExceptionHeader(out, type);

        if (type == Type::Memory)
        {
            out << "Unable to (re)allocate memory";
        }
        else
        {
            out << "An unknown error occurred";
        }

        m_what = out.str();
    }

    Exception::Exception(Type type, const char* problem, size_t offset, int value) :
        wil::ResultException(APPINSTALLER_CLI_ERROR_LIBYAML_ERROR)
    {
        std::ostringstream out;
        OutputExceptionHeader(out, type);

        out << (problem ? problem : "Unexplained error");

        if (value != -1)
        {
            out << " [" << value << ']';
        }

        out << " at " << offset;

        m_what = out.str();
    }

    Exception::Exception(Type type, const char* problem, const Mark& problemMark, const char* context, const Mark& contextMark) :
        wil::ResultException(APPINSTALLER_CLI_ERROR_LIBYAML_ERROR)
    {
        std::ostringstream out;
        OutputExceptionHeader(out, type);

        if (context)
        {
            out << context << ' ';
            OutputMark(out, contextMark);
            out << ' ' << (problem ? problem : "unexplained error");
        }
        else
        {
            out << (problem ? problem : "Unexplained error");
        }

        out << ' ';
        OutputMark(out, problemMark);

        m_what = out.str();
    }

    Exception::Exception(Type type, const char* problem) :
        wil::ResultException(APPINSTALLER_CLI_ERROR_LIBYAML_ERROR)
    {
        std::ostringstream out;
        OutputExceptionHeader(out, type);

        out << (problem ? problem : "Unexplained error");

        m_what = out.str();
    }

    const char* Exception::what() const noexcept
    {
        return m_what.c_str();
    }

    Node::Node(Type type, std::string tag, const YAML::Mark& mark) :
        m_type(type), m_tag(std::move(tag)), m_mark(mark)
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
        auto itrs = m_mapping->equal_range(key);

        if (itrs.first == itrs.second)
        {
            return s_globalInvalidNode;
        }

        Node& result = itrs.first->second;

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY, ++itrs.first != itrs.second);

        return result;
    }

    const Node& Node::operator[](std::string_view key) const
    {
        Require(Type::Mapping);
        auto itrs = m_mapping->equal_range(key);

        if (itrs.first == itrs.second)
        {
            return s_globalInvalidNode;
        }

        const Node& result = itrs.first->second;

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY, ++itrs.first != itrs.second);

        return result;
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
        switch (m_type)
        {
        case Type::Invalid:
        case Type::None:
        case Type::Scalar:
            return 0;
        case Type::Sequence:
            return m_sequence->size();
        case Type::Mapping:
            return m_mapping->size();
        }

        THROW_HR(E_UNEXPECTED);
    }

    const std::vector<Node>& Node::Sequence() const
    {
        Require(Type::Sequence);
        return m_sequence.value();
    }

    const std::multimap<Node, Node>& Node::Mapping() const
    {
        Require(Type::Mapping);
        return m_mapping.value();
    }

    void Node::Require(Type type) const
    {
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INVALID_OPERATION, m_type != type);
    }

    std::string Node::as_dispatch(std::string*) const
    {
        return m_scalar;
    }

    int64_t Node::as_dispatch(int64_t*) const
    {
        return std::stoll(m_scalar);
    }

    int Node::as_dispatch(int*) const
    {
        // To allow HResult representation
        return static_cast<int>(std::stoll(m_scalar, 0, 0));
    }

    bool Node::as_dispatch(bool*) const
    {
        if (Utility::CaseInsensitiveEquals(m_scalar, "true"))
        {
            return true;
        }
        else if (Utility::CaseInsensitiveEquals(m_scalar, "false"))
        {
            return false;
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA);
        }
    }

    Node Load(std::string_view input)
    {
        Wrapper::Parser parser(input);
        Wrapper::Document document = parser.Load();

        if (document.HasRoot())
        {
            return document.GetRoot();
        }
        else
        {
            return {};
        }
    }

    Node Load(const std::string& input)
    {
        return Load(static_cast<std::string_view>(input));
    }

    Node Load(std::istream& input)
    {
        Wrapper::Parser parser(input);
        Wrapper::Document document = parser.Load();

        if (document.HasRoot())
        {
            return document.GetRoot();
        }
        else
        {
            return {};
        }
    }

    Node Load(const std::filesystem::path& input)
    {
        std::ifstream stream(input, std::ios_base::in | std::ios_base::binary);
        THROW_LAST_ERROR_IF(stream.fail());
        return Load(stream);
    }

    Emitter::Emitter() :
        m_document(std::make_unique<Wrapper::Document>(true))
    {
        SetAllowedInputs<InputType::BeginMap, InputType::BeginSeq>();
    }

    Emitter::Emitter(Emitter&&) = default;
    Emitter& Emitter::operator=(Emitter&&) = default;

    Emitter::~Emitter() = default;

    Emitter& Emitter::operator<<(EmitterEvent event)
    {
        switch (event)
        {
        case AppInstaller::YAML::BeginSeq:
        {
            CheckInput(InputType::BeginSeq);
            int id = m_document->AddSequence();
            AppendNode(id);
            m_containers.emplace(id, false);
            SetAllowedInputsForContainer();
            break;
        }
        case AppInstaller::YAML::EndSeq:
            CheckInput(InputType::EndSeq);
            m_containers.pop();
            SetAllowedInputsForContainer();
            break;
        case AppInstaller::YAML::BeginMap:
        {
            CheckInput(InputType::BeginMap);
            int id = m_document->AddMapping();
            AppendNode(id);
            m_containers.emplace(id, true);
            SetAllowedInputsForContainer();
            break;
        }
        case AppInstaller::YAML::EndMap:
            CheckInput(InputType::EndMap);
            m_containers.pop();
            SetAllowedInputsForContainer();
            break;
        case AppInstaller::YAML::Key:
            CheckInput(InputType::Key);
            m_scalarInfo = InputType::Key;
            SetAllowedInputs<InputType::Scalar>();
            break;
        case AppInstaller::YAML::Value:
            CheckInput(InputType::Value);
            m_scalarInfo = InputType::Value;
            SetAllowedInputs<InputType::Scalar, InputType::BeginMap, InputType::BeginSeq>();
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return *this;
    }

    Emitter& Emitter::operator<<(std::string_view value)
    {
        CheckInput(InputType::Scalar);

        int id = m_document->AddScalar(value);

        if (!m_scalarInfo)
        {
            // Part of a sequence
            AppendNode(id);
            // No change to allowed inputs
        }
        else if (m_scalarInfo.value() == InputType::Key)
        {
            m_keyId = id;
            m_scalarInfo = std::nullopt;
            SetAllowedInputs<InputType::Value, InputType::BeginMap, InputType::BeginSeq>();
        }
        else if (m_scalarInfo.value() == InputType::Value)
        {
            // Mapping pair complete
            AppendNode(id);
            m_scalarInfo = std::nullopt;
            SetAllowedInputsForContainer();
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE);
        }

        return *this;
    }

    Emitter& Emitter::operator<<(int64_t value)
    {
        std::ostringstream stream;
        stream << value;
        return operator<<(stream.str());
    }

    Emitter& Emitter::operator<<(bool value)
    {
        return operator<<(value ? "true"sv : "false"sv);
    }

    std::string Emitter::str()
    {
        std::ostringstream stream;
        Wrapper::Emitter emitter(stream);

        emitter.Dump(*m_document);
        emitter.Flush();

        return stream.str();
    }

    void Emitter::Emit(std::ostream& out)
    {
        Wrapper::Emitter emitter(out);

        emitter.Dump(*m_document);
        emitter.Flush();
    }

    void Emitter::AppendNode(int id)
    {
        if (!m_containers.empty())
        {
            ContainerInfo& ci = m_containers.top();

            if (ci.IsMapping)
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE, !m_keyId);
                m_document->AppendMappingPair(ci.Id, m_keyId.value(), id);
                m_keyId = std::nullopt;
            }
            else
            {
                m_document->AppendSequenceItem(ci.Id, id);
            }
        }
    }

    size_t Emitter::GetInputBitmask(InputType type)
    {
        return static_cast<size_t>(1) << static_cast<size_t>(type);
    }

    void Emitter::CheckInput(InputType type)
    {
        if ((m_allowedInputs & GetInputBitmask(type)) == 0)
        {
            AICLI_LOG(YAML, Error, << "Invalid emitter input [0x" <<
                std::hex << std::setw(2) << std::setfill('0') << GetInputBitmask(type) << "], expected one of [0x" <<
                std::hex << std::setw(2) << std::setfill('0') << m_allowedInputs << "]");
            THROW_HR(APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE);
        }
    }

    void Emitter::SetAllowedInputsForContainer()
    {
        if (m_containers.empty())
        {
            m_allowedInputs = 0;
        }
        else
        {
            if (m_containers.top().IsMapping)
            {
                SetAllowedInputs<InputType::Key, InputType::EndMap>();
            }
            else
            {
                SetAllowedInputs<InputType::Scalar, InputType::BeginMap, InputType::BeginSeq, InputType::EndSeq>();
            }
        }
    }
}
