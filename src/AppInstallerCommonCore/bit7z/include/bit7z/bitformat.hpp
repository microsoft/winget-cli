/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITFORMAT_HPP
#define BITFORMAT_HPP

#include <bitset>
#include <type_traits>

#include "bitcompressionmethod.hpp"
#include "bitdefines.hpp"
#include "bittypes.hpp"

namespace bit7z {

/**
 * @brief The FormatFeatures enum specifies the features supported by an archive file format.
 */
enum struct FormatFeatures : unsigned {
    MultipleFiles = 1u << 0,    ///< The format can compress/extract multiple files         (2^0 = 0000001)
    SolidArchive = 1u << 1,     ///< The format supports solid archives                     (2^1 = 0000010)
    CompressionLevel = 1u << 2, ///< The format is able to use different compression levels (2^2 = 0000100)
    Encryption = 1u << 3,       ///< The format supports archive encryption                 (2^3 = 0001000)
    HeaderEncryption = 1u << 4, ///< The format can encrypt the file names                  (2^4 = 0010000)
    MultipleMethods = 1u << 5   ///< The format can use different compression methods       (2^6 = 0100000)
};

template< typename Enum >
using underlying_type_t = typename std::underlying_type< Enum >::type;

template< typename Enum >
inline constexpr auto to_underlying( Enum enum_value ) noexcept -> underlying_type_t< Enum > {
    return static_cast< underlying_type_t< Enum > >( enum_value );
}

inline constexpr auto operator|( FormatFeatures lhs, FormatFeatures rhs ) noexcept -> FormatFeatures {
    return static_cast< FormatFeatures >( to_underlying( lhs ) | to_underlying( rhs ) );
}

using FormatFeaturesType = underlying_type_t< FormatFeatures >;

inline constexpr auto operator&( FormatFeatures lhs, FormatFeatures rhs ) noexcept -> FormatFeaturesType {
    return to_underlying( lhs ) & to_underlying( rhs );
}

/**
 * @brief The BitInFormat class specifies an extractable archive format.
 *
 * @note Usually, the user of the library should not create new formats and, instead,
 * use the ones provided by the BitFormat namespace.
 */
class BitInFormat {
    public:
        //non-copyable
        BitInFormat( const BitInFormat& other ) = delete;

        auto operator=( const BitInFormat& other ) -> BitInFormat& = delete;

        //non-movable
        BitInFormat( BitInFormat&& other ) = delete;

        auto operator=( BitInFormat&& other ) -> BitInFormat& = delete;

        ~BitInFormat() = default;

        /**
         * @brief Constructs a BitInFormat object with the ID value used by the 7z SDK.
         * @param value  the value of the format in the 7z SDK.
         */
        constexpr explicit BitInFormat( unsigned char value ) noexcept: mValue( value ) {}

        /**
         * @return the value of the format in the 7z SDK.
         */
        BIT7Z_NODISCARD auto value() const noexcept -> unsigned char;

        /**
         * @param other  the target object to compare to.
         * @return a boolean value indicating whether this format is equal to the "other" or not.
         */
        auto operator==( BitInFormat const& other ) const noexcept -> bool;

        /**
         * @param other  the target object to compare to.
         * @return a boolean value indicating whether this format is different from the "other" or not.
         */
        auto operator!=( BitInFormat const& other ) const noexcept -> bool;

    private:
        unsigned char mValue;
};

/**
 * @brief The BitInOutFormat class specifies a format available for creating new archives and extract old ones.
 *
 * @note Usually, the user of the library should not create new formats and, instead,
 * use the ones provided by the BitFormat namespace.
 */
class BitInOutFormat final : public BitInFormat {
    public:
        /**
         * @brief Constructs a BitInOutFormat object with an ID value, an extension and a set of supported features.
         *
         * @param value         the value of the format in the 7z SDK.
         * @param ext           the default file extension of the archive format.
         * @param defaultMethod the default method used for compressing the archive format.
         * @param features      the set of features supported by the archive format
         */
        constexpr BitInOutFormat( unsigned char value,
                                  const tchar* ext,
                                  BitCompressionMethod defaultMethod,
                                  FormatFeatures features ) noexcept
            : BitInFormat( value ), mExtension( ext ), mDefaultMethod( defaultMethod ), mFeatures( features ) {}

        //non-copyable
        BitInOutFormat( const BitInOutFormat& other ) = delete;

        auto operator=( const BitInOutFormat& other ) -> BitInOutFormat& = delete;

        //non-movable
        BitInOutFormat( BitInOutFormat&& other ) = delete;

        auto operator=( BitInOutFormat&& other ) -> BitInOutFormat& = delete;

        ~BitInOutFormat() = default;

        /**
         * @return the default file extension of the archive format.
         */
        BIT7Z_NODISCARD
        auto extension() const noexcept -> const tchar*;

        /**
         * @return the bitset of the features supported by the format.
         */
        BIT7Z_NODISCARD
        auto features() const noexcept -> FormatFeatures;

        /**
         * @brief Checks if the format has a specific feature (see FormatFeatures enum).
         *
         * @param feature   feature to be checked.
         *
         * @return a boolean value indicating whether the format has the given feature.
         */
        BIT7Z_NODISCARD
        auto hasFeature( FormatFeatures feature ) const noexcept -> bool;

        /**
         * @return the default method used for compressing the archive format.
         */
        BIT7Z_NODISCARD
        auto defaultMethod() const noexcept -> BitCompressionMethod;

    private:
        const tchar* mExtension;
        BitCompressionMethod mDefaultMethod;
        FormatFeatures mFeatures;
};

/**
 * @brief The namespace that contains a set of archive formats usable with bit7z classes.
 */
namespace BitFormat {
#ifdef BIT7Z_AUTO_FORMAT
/**
 * @brief Automatic Format Detection (available only when compiling bit7z using the `BIT7Z_AUTO_FORMAT` option).
 */
extern const BitInFormat Auto;
#endif
extern const BitInFormat Rar;       ///< RAR Archive Format
extern const BitInFormat Arj;       ///< ARJ Archive Format
//NOLINTNEXTLINE(*-identifier-length)
extern const BitInFormat Z;         ///< Z Archive Format
extern const BitInFormat Lzh;       ///< LZH Archive Format
extern const BitInFormat Cab;       ///< CAB Archive Format
extern const BitInFormat Nsis;      ///< NSIS Archive Format
extern const BitInFormat Lzma;      ///< LZMA Archive Format
extern const BitInFormat Lzma86;    ///< LZMA86 Archive Format
extern const BitInFormat Ppmd;      ///< PPMD Archive Format
extern const BitInFormat Vhdx;      ///< VHDX Archive Format
extern const BitInFormat COFF;      ///< COFF Archive Format
extern const BitInFormat Ext;       ///< EXT Archive Format
extern const BitInFormat VMDK;      ///< VMDK Archive Format
extern const BitInFormat VDI;       ///< VDI Archive Format
extern const BitInFormat QCow;      ///< QCOW Archive Format
extern const BitInFormat GPT;       ///< GPT Archive Format
extern const BitInFormat Rar5;      ///< RAR5 Archive Format
extern const BitInFormat IHex;      ///< IHEX Archive Format
extern const BitInFormat Hxs;       ///< HXS Archive Format
//NOLINTNEXTLINE(*-identifier-length)
extern const BitInFormat TE;        ///< TE Archive Format
extern const BitInFormat UEFIc;     ///< UEFIc Archive Format
extern const BitInFormat UEFIs;     ///< UEFIs Archive Format
extern const BitInFormat SquashFS;  ///< SquashFS Archive Format
extern const BitInFormat CramFS;    ///< CramFS Archive Format
extern const BitInFormat APM;       ///< APM Archive Format
extern const BitInFormat Mslz;      ///< MSLZ Archive Format
extern const BitInFormat Flv;       ///< FLV Archive Format
extern const BitInFormat Swf;       ///< SWF Archive Format
extern const BitInFormat Swfc;      ///< SWFC Archive Format
extern const BitInFormat Ntfs;      ///< NTFS Archive Format
extern const BitInFormat Fat;       ///< FAT Archive Format
extern const BitInFormat Mbr;       ///< MBR Archive Format
extern const BitInFormat Vhd;       ///< VHD Archive Format
//NOLINTNEXTLINE(*-identifier-length)
extern const BitInFormat Pe;        ///< PE Archive Format
extern const BitInFormat Elf;       ///< ELF Archive Format
extern const BitInFormat Macho;     ///< MACHO Archive Format
extern const BitInFormat Udf;       ///< UDF Archive Format
extern const BitInFormat Xar;       ///< XAR Archive Format
extern const BitInFormat Mub;       ///< MUB Archive Format
extern const BitInFormat Hfs;       ///< HFS Archive Format
extern const BitInFormat Dmg;       ///< DMG Archive Format
extern const BitInFormat Compound;  ///< COMPOUND Archive Format
extern const BitInFormat Iso;       ///< ISO Archive Format
extern const BitInFormat Chm;       ///< CHM Archive Format
extern const BitInFormat Split;     ///< SPLIT Archive Format
extern const BitInFormat Rpm;       ///< RPM Archive Format
extern const BitInFormat Deb;       ///< DEB Archive Format
extern const BitInFormat Cpio;      ///< CPIO Archive Format

extern const BitInOutFormat Zip;        ///< ZIP Archive Format
extern const BitInOutFormat BZip2;      ///< BZIP2 Archive Format
extern const BitInOutFormat SevenZip;   ///< 7Z Archive Format
//NOLINTNEXTLINE(*-identifier-length)
extern const BitInOutFormat Xz;         ///< XZ Archive Format
extern const BitInOutFormat Wim;        ///< WIM Archive Format
extern const BitInOutFormat Tar;        ///< TAR Archive Format
extern const BitInOutFormat GZip;       ///< GZIP Archive Format
}  // namespace BitFormat


#ifdef BIT7Z_AUTO_FORMAT
#define BIT7Z_DEFAULT_FORMAT = BitFormat::Auto
#else
#define BIT7Z_DEFAULT_FORMAT
#endif

}  // namespace bit7z

#endif // BITFORMAT_HPP
