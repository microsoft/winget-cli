#pragma once

#include <fstream>
#include <limits>

namespace valijson {
namespace utils {

/**
 * Load a file into a string
 *
 * @param  path  path to the file to be loaded
 * @param  dest  string into which file should be loaded
 *
 * @return  true if loaded, false otherwise
 */
inline bool loadFile(const std::string &path, std::string &dest)
{
    // Open file for reading
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }

    // Allocate space for file contents
    file.seekg(0, std::ios::end);
    const std::streamoff offset = file.tellg();
    if (offset < 0 || offset > std::numeric_limits<unsigned int>::max()) {
        return false;
    }

    dest.clear();
    dest.reserve(static_cast<unsigned int>(offset));

    // Assign file contents to destination string
    file.seekg(0, std::ios::beg);
    dest.assign(std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());

    return true;
}

}  // namespace utils
}  // namespace valijson
