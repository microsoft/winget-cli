// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TableOutput.h"
#include <string_view>

namespace AppInstaller::CLI::Execution
{
    TableOutputBase::TableOutputBase(Reporter& reporter, std::vector<Resource::LocString> header) :
        m_reporter(reporter)
    {
        THROW_HR_IF(E_INVALIDARG, header.empty());
        for (auto& name : header)
        {
            auto width = Utility::UTF8ColumnWidth(name.get());
            m_columns.push_back({ std::move(name), width });
        }
    }

    void TableOutputBase::OutputLine(std::vector<std::string> line)
    {
        THROW_HR_IF(E_INVALIDARG, line.size() != m_columns.size());
        m_buffer.emplace_back(std::move(line));
    }

    void TableOutputBase::Complete(bool showLineNumbers)
    {
        if (!IsEmpty() && !m_bufferEvaluated)
        {
            EvaluateAndFlushBuffer(showLineNumbers);
        }
    }

    void TableOutputBase::EvaluateAndFlushBuffer(bool showLineNumbers)
    {
        for (const auto& row : m_buffer)
        {
            for (size_t i = 0; i < m_columns.size(); ++i)
            {
                m_columns[i].MaxLength = std::max(m_columns[i].MaxLength, Utility::UTF8ColumnWidth(row[i]));
            }
        }

        for (auto& column : m_columns)
        {
            if (column.MaxLength)
            {
                column.MaxLength = std::max(column.MaxLength, column.MinLength);
            }
        }

        m_columns.back().SpaceAfter = false;
        for (size_t i = m_columns.size() - 1; i > 0; --i)
        {
            if (m_columns[i].MaxLength)
            {
                break;
            }
            m_columns[i - 1].SpaceAfter = false;
        }

        m_lineNumberWidth = showLineNumbers ? std::to_string(m_buffer.size()).size() : 0;
        size_t totalRequired = m_lineNumberWidth ? m_lineNumberWidth + 1 : 0;
        for (const auto& column : m_columns)
        {
            totalRequired += column.MaxLength + (column.SpaceAfter ? 1 : 0);
        }

        auto consoleWidth = GetConsoleWidth();
        if (consoleWidth && totalRequired >= *consoleWidth)
        {
            size_t extra = (totalRequired - *consoleWidth) + 1;
            while (extra)
            {
                auto widest = std::max_element(m_columns.begin(), m_columns.end(),
                    [](const auto& left, const auto& right) { return left.MaxLength < right.MaxLength; });
                if (!widest->MaxLength)
                {
                    break;
                }
                --widest->MaxLength;
                --totalRequired;
                --extra;
            }
        }

        std::vector<std::string> header;
        for (const auto& column : m_columns)
        {
            header.emplace_back(column.Name.get());
        }
        OutputLineToStream(header);
        m_reporter.Info() << std::string(totalRequired, '-') << std::endl;
        size_t lineNumber = 0;
        for (const auto& row : m_buffer)
        {
            OutputLineToStream(row, ++lineNumber);
        }
        m_bufferEvaluated = true;
    }

    void TableOutputBase::OutputLineToStream(const std::vector<std::string>& line, size_t lineNumber)
    {
        auto out = m_reporter.Info();
        if (m_lineNumberWidth)
        {
            const std::string number = lineNumber ? std::to_string(lineNumber) : "#";
            out << number << std::string(m_lineNumberWidth - number.size() + 1, ' ');
        }
        for (size_t i = 0; i < m_columns.size(); ++i)
        {
            const auto& column = m_columns[i];
            if (column.MaxLength)
            {
                std::string_view value = line[i];
                size_t valueLength = Utility::UTF8ColumnWidth(value);
                if (valueLength > column.MaxLength)
                {
                    size_t actualWidth;
                    out << Utility::UTF8TrimRightToColumnWidth(value, column.MaxLength - 1, actualWidth) << "\xE2\x80\xA6";
                    // Wide characters can leave one column unused before the ellipsis.
                    if (actualWidth != column.MaxLength - 1)
                    {
                        out << ' ';
                    }
                    if (column.SpaceAfter)
                    {
                        out << ' ';
                    }
                }
                else
                {
                    out << value;
                    if (column.SpaceAfter)
                    {
                        out << std::string(column.MaxLength - valueLength + 1, ' ');
                    }
                }
            }
        }
        out << std::endl;
    }
}
