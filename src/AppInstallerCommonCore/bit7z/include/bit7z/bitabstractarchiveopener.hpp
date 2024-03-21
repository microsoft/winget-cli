/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITABSTRACTARCHIVEOPENER_HPP
#define BITABSTRACTARCHIVEOPENER_HPP

#include <vector>
#include <map>

#include "bitabstractarchivehandler.hpp"
#include "bitformat.hpp"

namespace bit7z {

using std::ostream;

/**
 * @brief The BitAbstractArchiveOpener abstract class represents a generic archive opener.
 */
class BitAbstractArchiveOpener : public BitAbstractArchiveHandler {
    public:
        BitAbstractArchiveOpener( const BitAbstractArchiveOpener& ) = delete;

        BitAbstractArchiveOpener( BitAbstractArchiveOpener&& ) = delete;

        auto operator=( const BitAbstractArchiveOpener& ) -> BitAbstractArchiveOpener& = delete;

        auto operator=( BitAbstractArchiveOpener&& ) -> BitAbstractArchiveOpener& = delete;

        ~BitAbstractArchiveOpener() override = default;

        /**
         * @return the archive format used by the archive opener.
         */
        BIT7Z_NODISCARD auto format() const noexcept -> const BitInFormat& override;

        /**
         * @return the archive format used by the archive opener.
         */
        BIT7Z_NODISCARD auto extractionFormat() const noexcept -> const BitInFormat&;

    protected:
        BitAbstractArchiveOpener( const Bit7zLibrary& lib,
                                  const BitInFormat& format,
                                  const tstring& password = {} );

    private:
        const BitInFormat& mFormat;
};

}  // namespace bit7z

#endif // BITABSTRACTARCHIVEOPENER_HPP
