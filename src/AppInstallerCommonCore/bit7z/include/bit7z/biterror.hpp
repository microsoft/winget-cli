/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITERROR_HPP
#define BITERROR_HPP

#include <system_error>

#include "bitdefines.hpp"

namespace bit7z {

/**
 * @brief The BitError enum struct values represent bit7z specific errors.
 */
enum struct BitError {
    Fail = 1,
    FilterNotSpecified,
    FormatFeatureNotSupported,
    IndicesNotSpecified,
    InvalidArchivePath,
    InvalidOutputBufferSize,
    InvalidCompressionMethod,
    InvalidDictionarySize,
    InvalidIndex,
    InvalidWordSize,
    ItemIsAFolder,
    ItemMarkedAsDeleted,
    NoMatchingItems,
    NoMatchingSignature,
    NonEmptyOutputBuffer,
    NullOutputBuffer,
    RequestedWrongVariantType,
    UnsupportedOperation,
    UnsupportedVariantType,
    WrongUpdateMode,
    InvalidZipPassword,
};

auto make_error_code( BitError error ) -> std::error_code;

/**
 * @brief The BitFailureSource enum struct values represent bit7z error conditions.
 * They can be used for performing queries on bit7z's `error_code`s, for the purpose
 * of grouping, classification, or error translation.
 */
enum struct BitFailureSource {
    CRCError,
    DataAfterEnd,
    DataError,
    InvalidArchive,
    InvalidArgument,
    FormatDetectionError,
    HeadersError,
    NoSuchItem,
    OperationNotSupported,
    OperationNotPermitted,
    UnavailableData,
    UnexpectedEnd,
    WrongPassword
};

auto make_error_condition( BitFailureSource failureSource ) -> std::error_condition;

}  // namespace bit7z

namespace std {
template<>
struct BIT7Z_MAYBE_UNUSED is_error_code_enum< bit7z::BitError > : public true_type {};

template <>
struct BIT7Z_MAYBE_UNUSED is_error_condition_enum< bit7z::BitFailureSource > : public true_type {};
} // namespace std

#endif //BITERROR_HPP
