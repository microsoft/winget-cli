// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TableOutput.h"

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

    void TableOutputBase::Complete()
    {
        if (!IsEmpty() && !m_bufferEvaluated)
        {
            EvaluateAndFlushBuffer();
        }
    }

    size_t TableOutputBase::GetNonEmptyRowCount(size_t column) const
    {
        THROW_HR_IF(E_INVALIDARG, column >= m_columns.size());
        return static_cast<size_t>(std::count_if(m_buffer.begin(), m_buffer.end(), [column](const auto& line)
        {
            return !line[column].empty();
        }));
    }

    void TableOutputBase::EvaluateAndFlushBuffer()
    {
        for (const auto& line : m_buffer)
        {
            for (size_t i = 0; i < m_columns.size(); ++i)
            {
                m_columns[i].MaxLength = std::max(m_columns[i].MaxLength, Utility::UTF8ColumnWidth(line[i]));
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

        size_t totalRequired = 0;
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
                --widest->MaxLength;
                --extra;
            }
            totalRequired = *consoleWidth - 1;
        }

        std::vector<std::string> header;
        for (const auto& column : m_columns)
        {
            header.emplace_back(column.Name.get());
        }
        OutputLineToStream(header);
        m_reporter.Info() << std::string(totalRequired, '-') << std::endl;
        for (const auto& line : m_buffer)
        {
            OutputLineToStream(line);
        }
        m_bufferEvaluated = true;
    }

    void TableOutputBase::OutputLineToStream(const std::vector<std::string>& line)
    {
        auto out = m_reporter.Info();
        for (size_t i = 0; i < m_columns.size(); ++i)
        {
            const auto& column = m_columns[i];
            if (column.MaxLength)
            {
                size_t valueLength = Utility::UTF8ColumnWidth(line[i]);
                if (valueLength > column.MaxLength)
                {
                    size_t actualWidth;
                    out << Utility::UTF8TrimRightToColumnWidth(line[i], column.MaxLength - 1, actualWidth) << "\xE2\x80\xA6";
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
                    out << line[i];
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
