// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionReporter.h"
#include "Resources.h"

#include <array>
#include <iterator>
#include <string>
#include <vector>


namespace AppInstaller::CLI::Execution
{
    struct TableOutputBase
    {
        TableOutputBase(Reporter& reporter, std::vector<Resource::LocString> header);

        // Buffers rows until Complete() computes column widths and renders the table.
        void OutputLine(std::vector<std::string> line);
        void Complete(bool showLineNumbers = false);
        bool IsEmpty() const { return m_buffer.empty(); }
        size_t GetRowCount() const { return m_buffer.size(); }

    private:
        // A column in the table.
        struct Column
        {
            Resource::LocString Name;
            size_t MinLength = 0;
            size_t MaxLength = 0;
            bool SpaceAfter = true;
        };

        Reporter& m_reporter;
        std::vector<Column> m_columns;
        std::vector<std::vector<std::string>> m_buffer;
        size_t m_lineNumberWidth = 0;
        bool m_bufferEvaluated = false;

        void EvaluateAndFlushBuffer(bool showLineNumbers);
        void OutputLineToStream(const std::vector<std::string>& line, size_t lineNumber = 0);
    };

    // Retains fixed-size headers and rows for existing table callers.
    template <size_t FieldCount>
    struct TableOutput : public TableOutputBase
    {
        using header_t = std::array<Resource::LocString, FieldCount>;
        using line_t = std::array<std::string, FieldCount>;

        TableOutput(Reporter& reporter, header_t&& header) :
            TableOutputBase(reporter, { std::make_move_iterator(header.begin()), std::make_move_iterator(header.end()) }) {}

        void OutputLine(line_t&& line)
        {
            TableOutputBase::OutputLine({ std::make_move_iterator(line.begin()), std::make_move_iterator(line.end()) });
        }
    };
}
