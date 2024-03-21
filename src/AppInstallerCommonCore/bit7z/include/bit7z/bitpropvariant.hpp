/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITPROPVARIANT_HPP
#define BITPROPVARIANT_HPP

#include <chrono>
#include <cstdint>

#include "bitdefines.hpp"
#include "bittypes.hpp"
#include "bitwindows.hpp"

namespace bit7z {

/**
 * @brief A type representing a time point measured using the system clock.
 */
using time_type = std::chrono::time_point< std::chrono::system_clock >;

/**
 * @brief The BitProperty enum represents the archive/item properties that 7-zip can read or write.
 */
enum struct BitProperty : PROPID {
    NoProperty = 0,         ///<
    MainSubfile,            ///<
    HandlerItemIndex,       ///<
    Path,                   ///<
    Name,                   ///<
    Extension,              ///<
    IsDir,                  ///<
    Size,                   ///<
    PackSize,               ///<
    Attrib,                 ///<
    CTime,                  ///<
    ATime,                  ///<
    MTime,                  ///<
    Solid,                  ///<
    Commented,              ///<
    Encrypted,              ///<
    SplitBefore,            ///<
    SplitAfter,             ///<
    DictionarySize,         ///<
    CRC,                    ///<
    Type,                   ///<
    IsAnti,                 ///<
    Method,                 ///<
    HostOS,                 ///<
    FileSystem,             ///<
    User,                   ///<
    Group,                  ///<
    Block,                  ///<
    Comment,                ///<
    Position,               ///<
    Prefix,                 ///<
    NumSubDirs,             ///<
    NumSubFiles,            ///<
    UnpackVer,              ///<
    Volume,                 ///<
    IsVolume,               ///<
    Offset,                 ///<
    Links,                  ///<
    NumBlocks,              ///<
    NumVolumes,             ///<
    TimeType,               ///<
    Bit64,                  ///<
    BigEndian,              ///<
    Cpu,                    ///<
    PhySize,                ///<
    HeadersSize,            ///<
    Checksum,               ///<
    Characts,               ///<
    Va,                     ///<
    Id,                     ///<
    ShortName,              ///<
    CreatorApp,             ///<
    SectorSize,             ///<
    PosixAttrib,            ///<
    SymLink,                ///<
    Error,                  ///<
    TotalSize,              ///<
    FreeSpace,              ///<
    ClusterSize,            ///<
    VolumeName,             ///<
    LocalName,              ///<
    Provider,               ///<
    NtSecure,               ///<
    IsAltStream,            ///<
    IsAux,                  ///<
    IsDeleted,              ///<
    IsTree,                 ///<
    Sha1,                   ///<
    Sha256,                 ///<
    ErrorType,              ///<
    NumErrors,              ///<
    ErrorFlags,             ///<
    WarningFlags,           ///<
    Warning,                ///<
    NumStreams,             ///<
    NumAltStreams,          ///<
    AltStreamsSize,         ///<
    VirtualSize,            ///<
    UnpackSize,             ///<
    TotalPhySize,           ///<
    VolumeIndex,            ///<
    SubType,                ///<
    ShortComment,           ///<
    CodePage,               ///<
    IsNotArcType,           ///<
    PhySizeCantBeDetected,  ///<
    ZerosTailIsAllowed,     ///<
    TailSize,               ///<
    EmbeddedStubSize,       ///<
    NtReparse,              ///<
    HardLink,               ///<
    INode,                  ///<
    StreamId,               ///<
    ReadOnly,               ///<
    OutName,                ///<
    CopyLink                ///<
};

/**
 * @brief The BitPropVariantType enum represents the possible types that a BitPropVariant can store.
 */
enum struct BitPropVariantType : uint32_t {
    Empty,      ///< Empty BitPropVariant type
    Bool,       ///< Boolean BitPropVariant type
    String,     ///< String BitPropVariant type
    UInt8,      ///< 8-bit unsigned int BitPropVariant type
    UInt16,     ///< 16-bit unsigned int BitPropVariant type
    UInt32,     ///< 32-bit unsigned int BitPropVariant type
    UInt64,     ///< 64-bit unsigned int BitPropVariant type
    Int8,       ///< 8-bit signed int BitPropVariant type
    Int16,      ///< 16-bit signed int BitPropVariant type
    Int32,      ///< 32-bit signed int BitPropVariant type
    Int64,      ///< 64-bit signed int BitPropVariant type
    FileTime    ///< FILETIME BitPropVariant type
};

/**
 * @brief The BitPropVariant struct is a light extension to the WinAPI PROPVARIANT struct providing useful getters.
 */
struct BitPropVariant final : public PROPVARIANT {
        /**
         * @brief Constructs an empty BitPropVariant object.
         */
        BitPropVariant();

        /**
         * @brief Copy constructs this BitPropVariant from another one.
         *
         * @param other the variant to be copied.
         */
        BitPropVariant( const BitPropVariant& other );

        /**
         * @brief Move constructs this BitPropVariant from another one.
         *
         * @param other the variant to be moved.
         */
        BitPropVariant( BitPropVariant&& other ) noexcept;

        /**
         * @brief Constructs a boolean BitPropVariant
         *
         * @param value the bool value of the BitPropVariant
         */
        explicit BitPropVariant( bool value ) noexcept;

        /**
         * @brief Constructs a string BitPropVariant from a null-terminated C wide string
         *
         * @param value the null-terminated C wide string value of the BitPropVariant
         */
        explicit BitPropVariant( const wchar_t* value );

        /**
         * @brief Constructs a string BitPropVariant from a wstring
         *
         * @param value the wstring value of the BitPropVariant
         */
        explicit BitPropVariant( const std::wstring& value );

        /**
         * @brief Constructs an 8-bit unsigned integer BitPropVariant
         *
         * @param value the uint8_t value of the BitPropVariant
         */
        explicit BitPropVariant( uint8_t value ) noexcept;

        /**
         * @brief Constructs a 16-bit unsigned integer BitPropVariant
         *
         * @param value the uint16_t value of the BitPropVariant
         */
        explicit BitPropVariant( uint16_t value ) noexcept;

        /**
         * @brief Constructs a 32-bit unsigned integer BitPropVariant
         *
         * @param value the uint32_t value of the BitPropVariant
         */
        explicit BitPropVariant( uint32_t value ) noexcept;

        /**
         * @brief Constructs a 64-bit unsigned integer BitPropVariant
         *
         * @param value the uint64_t value of the BitPropVariant
         */
        explicit BitPropVariant( uint64_t value ) noexcept;

        /**
         * @brief Constructs an 8-bit integer BitPropVariant
         *
         * @param value the int8_t value of the BitPropVariant
         */
        explicit BitPropVariant( int8_t value ) noexcept;

        /**
         * @brief Constructs a 16-bit integer BitPropVariant
         *
         * @param value the int16_t value of the BitPropVariant
         */
        explicit BitPropVariant( int16_t value ) noexcept;

        /**
         * @brief Constructs a 32-bit integer BitPropVariant
         *
         * @param value the int32_t value of the BitPropVariant
         */
        explicit BitPropVariant( int32_t value ) noexcept;

        /**
         * @brief Constructs a 64-bit integer BitPropVariant
         *
         * @param value the int64_t value of the BitPropVariant
         */
        explicit BitPropVariant( int64_t value ) noexcept;

        /**
         * @brief Constructs a FILETIME BitPropVariant
         *
         * @param value the FILETIME value of the BitPropVariant
         */
        explicit BitPropVariant( FILETIME value ) noexcept;

        /**
         * @brief BitPropVariant destructor.
         *
         * @note This is not virtual to maintain the same memory layout of the base struct!
         */
        ~BitPropVariant();

        /**
         * @brief Copy assignment operator.
         *
         * @param other the variant to be copied.
         *
         * @return a reference to *this object (with the copied values from other).
         */
        auto operator=( const BitPropVariant& other ) -> BitPropVariant&;

        /**
         * @brief Move assignment operator.
         *
         * @param other the variant to be moved.
         *
         * @return a reference to *this object (with the moved values from other).
         */
        auto operator=( BitPropVariant&& other ) noexcept -> BitPropVariant&;

        /**
         * @brief Assignment operator
         *
         * @note this will work only for T types for which a BitPropVariant constructor is defined!
         *
         * @param value the value to be assigned to the object
         *
         * @return a reference to *this object having the value as new variant value
         */
        template< typename T >
        auto operator=( const T& value ) noexcept( std::is_integral< T >::value ) -> BitPropVariant& {
            *this = BitPropVariant{ value };
            return *this;
        }

        /**
         * @return the boolean value of this variant
         * (it throws an exception if the variant is not a boolean value).
         */
        BIT7Z_NODISCARD auto getBool() const -> bool;

        /**
         * @return the string value of this variant
         * (it throws an exception if the variant is not a string).
         */
        BIT7Z_NODISCARD auto getString() const -> tstring;

        /**
         * @return the native string value of this variant
         * (it throws an exception if the variant is not a string).
         */
        BIT7Z_NODISCARD auto getNativeString() const -> native_string;

        /**
         * @return the 8-bit unsigned integer value of this variant
         * (it throws an exception if the variant is not an 8-bit unsigned integer).
         */
        BIT7Z_NODISCARD auto getUInt8() const -> uint8_t;

        /**
         * @return the 16-bit unsigned integer value of this variant
         * (it throws an exception if the variant is not an 8 or 16-bit unsigned integer).
         */
        BIT7Z_NODISCARD auto getUInt16() const -> uint16_t;

        /**
         * @return the 32-bit unsigned integer value of this variant
         * (it throws an exception if the variant is not an 8, 16 or 32-bit unsigned integer).
         */
        BIT7Z_NODISCARD auto getUInt32() const -> uint32_t;

        /**
         * @return the 64-bit unsigned integer value of this variant
         * (it throws an exception if the variant is not an 8, 16, 32 or 64-bit unsigned integer).
         */
        BIT7Z_NODISCARD auto getUInt64() const -> uint64_t;

        /**
         * @return the 8-bit integer value of this variant
         * (it throws an exception if the variant is not an 8-bit integer).
         */
        BIT7Z_NODISCARD auto getInt8() const -> int8_t;

        /**
         * @return the 16-bit integer value of this variant
         * (it throws an exception if the variant is not an 8 or 16-bit integer).
         */
        BIT7Z_NODISCARD auto getInt16() const -> int16_t;

        /**
         * @return the 32-bit integer value of this variant
         * (it throws an exception if the variant is not an 8, 16 or 32-bit integer).
         */
        BIT7Z_NODISCARD auto getInt32() const -> int32_t;

        /**
         * @return the 64-bit integer value of this variant
         * (it throws an exception if the variant is not an 8, 16, 32 or 64-bit integer).
         */
        BIT7Z_NODISCARD auto getInt64() const -> int64_t;

        /**
         * @return the FILETIME value of this variant
         * (it throws an exception if the variant is not a filetime).
         */
        BIT7Z_NODISCARD auto getFileTime() const -> FILETIME;

        /**
         * @return the FILETIME value of this variant converted to std::time_point
         * (it throws an exception if the variant is not a filetime).
         */
        BIT7Z_NODISCARD auto getTimePoint() const -> time_type;

        /**
         * @return the value of this variant converted from any supported type to std::wstring.
         */
        BIT7Z_NODISCARD auto toString() const -> tstring;

        /**
         * @return a boolean value indicating whether the variant is empty.
         */
        BIT7Z_NODISCARD auto isEmpty() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is a boolean value.
         */
        BIT7Z_NODISCARD auto isBool() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is a string.
         */
        BIT7Z_NODISCARD auto isString() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8-bit unsigned integer.
         */
        BIT7Z_NODISCARD auto isUInt8() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8 or 16-bit unsigned integer.
         */
        BIT7Z_NODISCARD auto isUInt16() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8, 16 or 32-bit unsigned integer.
         */
        BIT7Z_NODISCARD auto isUInt32() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8, 16, 32 or 64-bit unsigned integer.
         */
        BIT7Z_NODISCARD auto isUInt64() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8-bit integer.
         */
        BIT7Z_NODISCARD auto isInt8() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8 or 16-bit integer.
         */
        BIT7Z_NODISCARD auto isInt16() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8, 16 or 32-bit integer.
         */
        BIT7Z_NODISCARD auto isInt32() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is an 8, 16, 32 or 64-bit integer.
         */
        BIT7Z_NODISCARD auto isInt64() const noexcept -> bool;

        /**
         * @return a boolean value indicating whether the variant is a FILETIME structure.
         */
        BIT7Z_NODISCARD auto isFileTime() const noexcept -> bool;

        /**
         * @return the BitPropVariantType of this variant.
         */
        BIT7Z_NODISCARD auto type() const -> BitPropVariantType;

        /**
         * @brief Clears the current value of the variant object
         */
        void clear() noexcept;

    private:
        void internalClear() noexcept;

        friend auto operator==( const BitPropVariant& lhs, const BitPropVariant& rhs ) noexcept -> bool;

        friend auto operator!=( const BitPropVariant& lhs, const BitPropVariant& rhs ) noexcept -> bool;
};

auto operator==( const BitPropVariant& lhs, const BitPropVariant& rhs ) noexcept -> bool;

auto operator!=( const BitPropVariant& lhs, const BitPropVariant& rhs ) noexcept -> bool;

}  // namespace bit7z

#endif // BITPROPVARIANT_HPP
