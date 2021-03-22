/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * searchfile.cpp - Simple cmd line application that uses a variety of streams features to search a file,
 *      store the results, and write results back to a new file.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/containerstream.h"
#include "cpprest/filestream.h"
#include "cpprest/producerconsumerstream.h"

using namespace utility;
using namespace concurrency::streams;

/// <summary>
/// A convenient helper function to loop asychronously until a condition is met.
/// </summary>
pplx::task<bool> _do_while_iteration(std::function<pplx::task<bool>(void)> func)
{
    pplx::task_completion_event<bool> ev;
    func().then([=](bool guard) { ev.set(guard); });
    return pplx::create_task(ev);
}
pplx::task<bool> _do_while_impl(std::function<pplx::task<bool>(void)> func)
{
    return _do_while_iteration(func).then([=](bool guard) -> pplx::task<bool> {
        if (guard)
        {
            return ::_do_while_impl(func);
        }
        else
        {
            return pplx::task_from_result(false);
        }
    });
}
pplx::task<void> do_while(std::function<pplx::task<bool>(void)> func)
{
    return _do_while_impl(func).then([](bool) {});
}

/// <summary>
/// Structure used to store individual line results.
/// </summary>
typedef std::vector<std::string> matched_lines;
namespace Concurrency
{
namespace streams
{
/// <summary>
/// Parser implementation for 'matched_lines' type.
/// </summary>
template<typename CharType>
class type_parser<CharType, matched_lines>
{
public:
    static pplx::task<matched_lines> parse(streambuf<CharType> buffer)
    {
        basic_istream<CharType> in(buffer);
        auto lines = std::make_shared<matched_lines>();
        return do_while([=]() {
                   container_buffer<std::string> line;
                   return in.read_line(line).then([=](const size_t bytesRead) {
                       if (bytesRead == 0 && in.is_eof())
                       {
                           return false;
                       }
                       else
                       {
                           lines->push_back(std::move(line.collection()));
                           return true;
                       }
                   });
               })
            .then([=]() { return matched_lines(std::move(*lines)); });
    }
};
} // namespace streams
} // namespace Concurrency
/// <summary>
/// Function to create in data from a file and search for a given string writing all lines containing the string to
/// memory_buffer.
/// </summary>
static pplx::task<void> find_matches_in_file(const string_t& fileName,
                                             const std::string& searchString,
                                             basic_ostream<char> results)
{
    return file_stream<char>::open_istream(fileName).then([=](basic_istream<char> inFile) {
        auto lineNumber = std::make_shared<int>(1);
        return ::do_while([=]() {
                   container_buffer<std::string> inLine;
                   return inFile.read_line(inLine).then([=](size_t bytesRead) {
                       if (bytesRead == 0 && inFile.is_eof())
                       {
                           return pplx::task_from_result(false);
                       }

                       else if (inLine.collection().find(searchString) != std::string::npos)
                       {
                           results.print("line ");
                           results.print((*lineNumber)++);
                           return results.print(":")
                               .then([=](size_t) {
                                   container_buffer<std::string> outLine(std::move(inLine.collection()));
                                   return results.write(outLine, outLine.collection().size());
                               })
                               .then([=](size_t) { return results.print("\r\n"); })
                               .then([=](size_t) { return true; });
                       }

                       else
                       {
                           ++(*lineNumber);
                           return pplx::task_from_result(true);
                       }
                   });
               })
            .then([=]() {
                // Close the file and results stream.
                return inFile.close() && results.close();
            });
    });
}

/// <summary>
/// Function to write out results from matched_lines type to file
/// </summary>
static pplx::task<void> write_matches_to_file(const string_t& fileName, matched_lines results)
{
    // Create a shared pointer to the matched_lines structure to copying repeatedly.
    auto sharedResults = std::make_shared<matched_lines>(std::move(results));

    return file_stream<char>::open_ostream(fileName, std::ios::trunc).then([=](basic_ostream<char> outFile) {
        auto currentIndex = std::make_shared<size_t>(0);
        return ::do_while([=]() {
                   if (*currentIndex >= sharedResults->size())
                   {
                       return pplx::task_from_result(false);
                   }

                   container_buffer<std::string> lineData((*sharedResults)[(*currentIndex)++]);
                   outFile.write(lineData, lineData.collection().size());
                   return outFile.print("\r\n").then([](size_t) { return true; });
               })
            .then([=]() { return outFile.close(); });
    });
}

#ifdef _WIN32
int wmain(int argc, wchar_t* args[])
#else
int main(int argc, char* args[])
#endif
{
    if (argc != 4)
    {
        printf("Usage: SearchFile.exe input_file search_string output_file\n");
        return -1;
    }
    const string_t inFileName = args[1];
    const std::string searchString = utility::conversions::to_utf8string(args[2]);
    const string_t outFileName = args[3];
    producer_consumer_buffer<char> lineResultsBuffer;

    // Find all matches in file.
    basic_ostream<char> outLineResults(lineResultsBuffer);
    find_matches_in_file(inFileName, searchString, outLineResults)

        // Write matches into custom data structure.
        .then([&]() {
            basic_istream<char> inLineResults(lineResultsBuffer);
            return inLineResults.extract<matched_lines>();
        })

        // Write out stored match data to a new file.
        .then([&](matched_lines lines) { return write_matches_to_file(outFileName, std::move(lines)); })

        // Wait for everything to complete.
        .wait();

    return 0;
}
