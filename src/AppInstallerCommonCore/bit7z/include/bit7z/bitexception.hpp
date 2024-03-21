/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITEXCEPTION_HPP
#define BITEXCEPTION_HPP

#include <vector>
#include <system_error>

#include "bitdefines.hpp"
#include "bittypes.hpp"
#include "bitwindows.hpp"

namespace bit7z {

using std::system_error;
using FailedFiles = std::vector< std::pair< tstring, std::error_code > >;

auto make_hresult_code( HRESULT res ) noexcept -> std::error_code;

auto last_error_code() noexcept -> std::error_code;

/**
 * @brief The BitException class represents a generic exception thrown from the bit7z classes.
 */
class BitException final : public system_error {
    public:
#ifdef _WIN32
        using native_code_type = HRESULT;
#else
        using native_code_type = int;
#endif

        /**
         * @brief Constructs a BitException object with the given message, and the specific files that failed.
         *
         * @param message   the message associated with the exception object.
         * @param files     the vector of files that failed, with the corresponding error codes.
         * @param code      the HRESULT code associated with the exception object.
         */
        explicit BitException( const char* message, std::error_code code, FailedFiles&& files = {} );

        /**
         * @brief Constructs a BitException object with the given message, and the specific file that failed.
         *
         * @param message   the message associated with the exception object.
         * @param code      the HRESULT code associated with the exception object.
         * @param file      the file that failed during the operation.
         */
        BitException( const char* message, std::error_code code, tstring&& file );

        /**
         * @brief Constructs a BitException object with the given message, and the specific file that failed.
         *
         * @param message   the message associated with the exception object.
         * @param code      the HRESULT code associated with the exception object.
         * @param file      the file that failed during the operation.
         */
        BitException( const char* message, std::error_code code, const tstring& file );

        /**
         * @brief Constructs a BitException object with the given message.
         *
         * @param message   the message associated with the exception object.
         * @param code      the HRESULT code associated with the exception object.
         */
        explicit BitException( const std::string& message, std::error_code code );

        /**
         * @return the native error code (e.g., HRESULT on Windows, int elsewhere)
         * corresponding to the exception's std::error_code.
         */
        BIT7Z_NODISCARD auto nativeCode() const noexcept -> native_code_type;

        /**
         * @return the HRESULT error code corresponding to the exception's std::error_code.
         */
        BIT7Z_NODISCARD auto hresultCode() const noexcept -> HRESULT;

        /**
         * @return the POSIX error code corresponding to the exception's std::error_code.
         */
        BIT7Z_NODISCARD auto posixCode() const noexcept -> int;

        /**
         * @return the vector of files that caused the exception to be thrown, along with the corresponding
         *         error codes.
         */
        BIT7Z_NODISCARD auto failedFiles() const noexcept -> const FailedFiles&;

    private:
        FailedFiles mFailedFiles;
};

}  // namespace bit7z

#endif // BITEXCEPTION_HPP
