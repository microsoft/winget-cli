// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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

        static constexpr std::string_view s_nullTag = "tag:yaml.org,2002:null"sv;
        static constexpr std::string_view s_boolTag = "tag:yaml.org,2002:bool"sv;
        static constexpr std::string_view s_strTag = "tag:yaml.org,2002:str"sv;
        static constexpr std::string_view s_intTag = "tag:yaml.org,2002:int"sv;
        static constexpr std::string_view s_floatTag = "tag:yaml.org,2002:float"sv;
        static constexpr std::string_view s_timestampTag = "tag:yaml.org,2002:timestamp"sv;
        static constexpr std::string_view s_seqTag = "tag:yaml.org,2002:seq"sv;
        static constexpr std::string_view s_mapTag = "tag:yaml.org,2002:map"sv;

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

        Node::TagType ConvertToTagType(const std::string& tag)
        {
            if (tag == s_strTag)
            {
                return Node::TagType::Str;
            }
            else if (tag == s_seqTag)
            {
                return Node::TagType::Seq;
            }
            else if (tag == s_mapTag)
            {
                return Node::TagType::Map;
            }
            else if (tag == s_boolTag)
            {
                return Node::TagType::Bool;
            }
            else if (tag == s_intTag)
            {
                return Node::TagType::Int;
            }
            else if (tag == s_floatTag)
            {
                return Node::TagType::Float;
            }
            else if (tag == s_timestampTag)
            {
                return Node::TagType::Timestamp;
            }
            else if (tag == s_nullTag)
            {
                return Node::TagType::Null;
            }

            return Node::TagType::Unknown;
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

        m_tagType = ConvertToTagType(m_tag);
    }

    void Node::SetScalar(std::string value)
    {
        Require(Type::Scalar);
        m_scalar = std::move(value);
    }

    void Node::SetScalar(std::string value, bool isQuoted)
    {
        this->SetScalar(value);

        // For untagged scalar nodes, libyaml always assigns the generic string
        // tag. Here we just try our best and assume that if the value is unquoted
        // then is not necessarily a string.
        // TODO: handle float and timestamps
        if (!isQuoted && this->GetTagType() == TagType::Str)
        {
            // Integer
            // 0 | -? [1-9] [0-9]*
            auto tryInt = this->try_as<int64_t>();
            if (tryInt.has_value())
            {
                m_tagType = TagType::Int;
                return;
            }

            // Boolean. Either 'true' or 'false'
            auto tryBool = this->try_as<bool>();
            if (tryBool.has_value())
            {
                m_tagType = TagType::Bool;
            }
        }
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

    std::optional<std::string> Node::try_as_dispatch(std::string*) const
    {
        return std::optional{ m_scalar };
    }

    std::wstring Node::as_dispatch(std::wstring*) const
    {
        return Utility::ConvertToUTF16(m_scalar);
    }

    std::optional<std::wstring> Node::try_as_dispatch(std::wstring*) const
    {
        return Utility::TryConvertToUTF16(m_scalar);
    }

    int64_t Node::as_dispatch(int64_t*) const
    {
        return std::stoll(m_scalar);
    }

    std::optional<int64_t> Node::try_as_dispatch(int64_t*) const
    {
        try
        {
            return std::optional{ std::stoll(m_scalar) };
        }
        catch(...)
        {
            return {};
        }
    }

    int Node::as_dispatch(int*) const
    {
        // To allow HResult representation
        return static_cast<int>(std::stoll(m_scalar, 0, 0));
    }

    std::optional<int> Node::try_as_dispatch(int*) const
    {
        try
        {
            return std::optional{ static_cast<int>(std::stoll(m_scalar, 0, 0)) };
        }
        catch (...)
        {
            return {};
        }
    }

    bool Node::as_dispatch(bool*) const
    {
        bool* t = nullptr;
        auto tryToBool = this->try_as_dispatch(t);
        if (tryToBool.has_value())
        {
            return tryToBool.value();
        }
        else
        {
            THROW_HR(APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA);
        }
    }

    std::optional<bool> Node::try_as_dispatch(bool*) const
    {
        if (Utility::CaseInsensitiveEquals(m_scalar, "true"))
        {
            return std::optional{ true };
        }
        else if (Utility::CaseInsensitiveEquals(m_scalar, "false"))
        {
            return std::optional{ false };
        }

        return {};
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

    Node Load(std::istream& input, Utility::SHA256::HashBuffer* hashOut)
    {
        Wrapper::Parser parser(input, hashOut);
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

    Node Load(const std::filesystem::path& input, Utility::SHA256::HashBuffer* hashOut)
    {
        std::ifstream stream(input, std::ios_base::in | std::ios_base::binary);
        THROW_LAST_ERROR_IF(stream.fail());
        return Load(stream, hashOut);
    }

    Node Load(const std::filesystem::path& input)
    {
        return Load(input, nullptr);
    }

    Node Load(const std::filesystem::path& input, Utility::SHA256::HashBuffer& hashOut)
    {
        return Load(input, &hashOut);
    }

    Emitter::Emitter() :
        m_document(std::make_unique<Wrapper::Document>(true))
    {
        SetAllowedInputs<InputType::BeginMap, InputType::BeginSeq>();
    }

    Emitter::Emitter(Emitter&&) noexcept = default;
    Emitter& Emitter::operator=(Emitter&&) noexcept = default;

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

    Emitter& Emitter::operator<<(int value)
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
