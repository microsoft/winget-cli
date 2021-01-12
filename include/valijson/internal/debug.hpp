#pragma once

#include <string>

namespace valijson {
namespace internal {

template<typename AdapterType>
std::string nodeTypeAsString(const AdapterType &node) {
    if (node.isArray()) {
        return "array";
    } else if (node.isObject()) {
        return "object";
    } else if (node.isString()) {
        return "string";
    } else if (node.isNull()) {
        return "null";
    } else if (node.isInteger()) {
        return "integer";
    } else if (node.isDouble()) {
        return "double";
    } else if (node.isBool()) {
        return "bool";
    }

    return "unknown";
}

} // end namespace internal
} // end namespace valijson
