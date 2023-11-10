/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: Compression and decompression interfaces
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

namespace web
{
namespace http
{
namespace compression
{
/// <summary>
/// Hint as to whether a compress or decompress call is meant to be the last for a particular HTTP request or reply
/// </summary>
enum operation_hint
{
    is_last, // Used for the expected last compress() call, or for an expected single decompress() call
    has_more // Used when further compress() calls will be made, or when multiple decompress() calls may be required
};

/// <summary>
/// Result structure for asynchronous compression and decompression operations
/// </summary>
struct operation_result
{
    size_t input_bytes_processed; // From the input buffer
    size_t output_bytes_produced; // To the output buffer
    bool done; // For compress, set when 'last' is true and there was enough space to complete compression;
               // for decompress, set if the end of the decompression stream has been reached
};

/// <summary>
/// Compression interface for use with HTTP requests
/// </summary>
class compress_provider
{
public:
    virtual const utility::string_t& algorithm() const = 0;
    virtual size_t compress(const uint8_t* input,
                            size_t input_size,
                            uint8_t* output,
                            size_t output_size,
                            operation_hint hint,
                            size_t& input_bytes_processed,
                            bool& done) = 0;
    virtual pplx::task<operation_result> compress(
        const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint) = 0;
    virtual void reset() = 0;
    virtual ~compress_provider() = default;
};

/// <summary>
/// Decompression interface for use with HTTP requests
/// </summary>
class decompress_provider
{
public:
    virtual const utility::string_t& algorithm() const = 0;
    virtual size_t decompress(const uint8_t* input,
                              size_t input_size,
                              uint8_t* output,
                              size_t output_size,
                              operation_hint hint,
                              size_t& input_bytes_processed,
                              bool& done) = 0;
    virtual pplx::task<operation_result> decompress(
        const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint) = 0;
    virtual void reset() = 0;
    virtual ~decompress_provider() = default;
};

/// <summary>
/// Factory interface for compressors for use with received HTTP requests
/// </summary>
class compress_factory
{
public:
    virtual const utility::string_t& algorithm() const = 0;
    virtual std::unique_ptr<compress_provider> make_compressor() const = 0;
    virtual ~compress_factory() = default;
};

/// <summary>
/// Factory interface for decompressors for use with HTTP requests
/// </summary>
class decompress_factory
{
public:
    virtual const utility::string_t& algorithm() const = 0;
    virtual uint16_t weight() const = 0;
    virtual std::unique_ptr<decompress_provider> make_decompressor() const = 0;
    virtual ~decompress_factory() = default;
};

/// <summary>
/// Built-in compression support
/// </summary>
namespace builtin
{
/// <summary>
/// Test whether cpprestsdk was built with built-in compression support
/// <returns>True if cpprestsdk was built with built-in compression support, and false if not.</returns>
/// </summary>
_ASYNCRTIMP bool supported();

/// <summary>
// String constants for each built-in compression algorithm, for convenient use with the factory functions
/// </summary>
namespace algorithm
{
#if defined(_MSC_VER) && _MSC_VER < 1900
const utility::char_t* const GZIP = _XPLATSTR("gzip");
const utility::char_t* const DEFLATE = _XPLATSTR("deflate");
const utility::char_t* const BROTLI = _XPLATSTR("br");
#else // ^^^ VS2013 and before ^^^ // vvv VS2015+, and everything else vvv
constexpr const utility::char_t* const GZIP = _XPLATSTR("gzip");
constexpr const utility::char_t* const DEFLATE = _XPLATSTR("deflate");
constexpr const utility::char_t* const BROTLI = _XPLATSTR("br");
#endif

/// <summary>
/// Test whether cpprestsdk was built with built-in compression support and
/// the supplied string matches a supported built-in algorithm
/// <param name="algorithm">The name of the algorithm to test for built-in support.</param>
/// <returns>True if cpprestsdk was built with built-in compression support and
/// the supplied string matches a supported built-in algorithm, and false if not.</returns>
/// <summary>
_ASYNCRTIMP bool supported(const utility::string_t& algorithm);
} // namespace algorithm

/// <summary>
/// Factory function to instantiate a built-in compression provider with default parameters by compression algorithm
/// name.
/// </summary>
/// <param name="algorithm">The name of the algorithm for which to instantiate a provider.</param>
/// <returns>
/// A caller-owned pointer to a provider of the requested-type, or to nullptr if no such built-in type exists.
/// </returns>
_ASYNCRTIMP std::unique_ptr<compress_provider> make_compressor(const utility::string_t& algorithm);

/// <summary>
/// Factory function to instantiate a built-in decompression provider with default parameters by compression algorithm
/// name.
/// </summary>
/// <param name="algorithm">The name of the algorithm for which to instantiate a provider.</param>
/// <returns>
/// A caller-owned pointer to a provider of the requested-type, or to nullptr if no such built-in type exists.
/// </returns>
_ASYNCRTIMP std::unique_ptr<decompress_provider> make_decompressor(const utility::string_t& algorithm);

/// <summary>
/// Factory function to obtain a pointer to a built-in compression provider factory by compression algorithm name.
/// </summary>
/// <param name="algorithm">The name of the algorithm for which to find a factory.</param>
/// <returns>
/// A caller-owned pointer to a provider of the requested-type, or to nullptr if no such built-in type exists.
/// </returns>
_ASYNCRTIMP std::shared_ptr<compress_factory> get_compress_factory(const utility::string_t& algorithm);

/// <summary>
/// Factory function to obtain a pointer to a built-in decompression provider factory by compression algorithm name.
/// </summary>
/// <param name="algorithm">The name of the algorithm for which to find a factory.</param>
/// <returns>
/// A caller-owned pointer to a provider of the requested-type, or to nullptr if no such built-in type exists.
/// </returns>
_ASYNCRTIMP std::shared_ptr<decompress_factory> get_decompress_factory(const utility::string_t& algorithm);

/// <summary>
// Factory function to instantiate a built-in gzip compression provider with caller-selected parameters.
/// </summary>
/// <returns>
/// A caller-owned pointer to a gzip compression provider, or to nullptr if the library was built without built-in
/// compression support.
/// </returns>
_ASYNCRTIMP std::unique_ptr<compress_provider> make_gzip_compressor(int compressionLevel,
                                                                    int method,
                                                                    int strategy,
                                                                    int memLevel);

/// <summary>
// Factory function to instantiate a built-in deflate compression provider with caller-selected parameters.
/// </summary>
/// <returns>
/// A caller-owned pointer to a deflate compression provider, or to nullptr if the library was built without built-in
/// compression support..
/// </returns>
_ASYNCRTIMP std::unique_ptr<compress_provider> make_deflate_compressor(int compressionLevel,
                                                                       int method,
                                                                       int strategy,
                                                                       int memLevel);

/// <summary>
// Factory function to instantiate a built-in Brotli compression provider with caller-selected parameters.
/// </summary>
/// <returns>
/// A caller-owned pointer to a Brotli compression provider, or to nullptr if the library was built without built-in
/// compression support.
/// </returns>
_ASYNCRTIMP std::unique_ptr<compress_provider> make_brotli_compressor(
    uint32_t window, uint32_t quality, uint32_t mode, uint32_t block, uint32_t nomodel, uint32_t hint);
} // namespace builtin

/// <summary>
/// Factory function to instantiate a compression provider factory by compression algorithm name.
/// </summary>
/// <param name="algorithm">The name of the algorithm supported by the factory.  Must match that returned by the
/// <c>web::http::compression::compress_provider</c> type instantiated by the factory's make_compressor function.
/// The supplied string is copied, and thus need not remain valid once the call returns.</param>
/// <param name="make_compressor">A factory function to be used to instantiate a compressor matching the factory's
/// reported algorithm.</param>
/// <returns>
/// A pointer to a generic provider factory implementation configured with the supplied parameters.
/// </returns>
/// <remarks>
/// This method may be used to conveniently instantiate a factory object for a caller-selected <c>compress_provider</c>.
/// That provider may be of the caller's own design, or it may be one of the built-in types.  As such, this method may
/// be helpful when a caller wishes to build vectors containing a mix of custom and built-in providers.
/// </remarks>
_ASYNCRTIMP std::shared_ptr<compress_factory> make_compress_factory(
    const utility::string_t& algorithm, std::function<std::unique_ptr<compress_provider>()> make_compressor);

/// <summary>
/// Factory function to instantiate a decompression provider factory by compression algorithm name.
/// </summary>
/// <param name="algorithm">The name of the algorithm supported by the factory.  Must match that returned by the
/// <c>web::http::compression::decompress_provider</c> type instantiated by the factory's make_decompressor function.
/// The supplied string is copied, and thus need not remain valid once the call returns.</param>
/// <param name="weight">A numeric weight for the compression algorithm, times 1000,  for use as a "quality value" when
/// requesting that the server send a compressed response.  Valid values are between 0 and 1000, inclusive, where higher
/// values indicate more preferred algorithms, and 0 indicates that the algorithm is not allowed; values greater than
/// 1000 are treated as 1000.</param>
/// <param name="make_decompressor">A factory function to be used to instantiate a decompressor matching the factory's
/// reported algorithm.</param>
/// <returns>
/// A pointer to a generic provider factory implementation configured with the supplied parameters.
/// </returns>
/// <remarks>
/// This method may be used to conveniently instantiate a factory object for a caller-selected
/// <c>decompress_provider</c>.  That provider may be of the caller's own design, or it may be one of the built-in
/// types.  As such, this method may be helpful when a caller wishes to change the weights of built-in provider types,
/// to use custom providers without explicitly implementing a <c>decompress_factory</c>, or to build vectors containing
/// a mix of custom and built-in providers.
/// </remarks>
_ASYNCRTIMP std::shared_ptr<decompress_factory> make_decompress_factory(
    const utility::string_t& algorithm,
    uint16_t weight,
    std::function<std::unique_ptr<decompress_provider>()> make_decompressor);

namespace details
{
/// <summary>
/// Header type enum for use with compressor and decompressor header parsing and building functions
/// </summary>
enum header_types
{
    transfer_encoding,
    content_encoding,
    te,
    accept_encoding
};

/// <summary>
/// Factory function to instantiate an appropriate compression provider, if any.
/// </summary>
/// <param name="encoding">A TE or Accept-Encoding header to interpret.</param>
/// <param name="type">Specifies the type of header whose contents are in the encoding parameter; valid values are
/// <c>header_type::te</c> and <c>header_type::accept_encoding</c>.</param>
/// <param name="preferred">A compressor object of the caller's preferred (possibly custom) type, which is used if
/// possible.</param>
/// <param name="factories">A collection of factory objects for use in construction of an appropriate compressor, if
/// any. If empty or not supplied, the set of supported built-in compressors is used.</param>
/// <returns>
/// A pointer to a compressor object that is acceptable per the supplied header, or to nullptr if no matching
/// algorithm is found.
/// </returns>
_ASYNCRTIMP std::unique_ptr<compress_provider> get_compressor_from_header(
    const utility::string_t& encoding,
    header_types type,
    const std::vector<std::shared_ptr<compress_factory>>& factories = std::vector<std::shared_ptr<compress_factory>>());

/// <summary>
/// Factory function to instantiate an appropriate decompression provider, if any.
/// </summary>
/// <param name="encoding">A Transfer-Encoding or Content-Encoding header to interpret.</param>
/// <param name="type">Specifies the type of header whose contents are in the encoding parameter; valid values are
/// <c>header_type::transfer_encoding</c> and <c>header_type::content_encoding</c>.</param>
/// <param name="factories">A collection of factory objects for use in construction of an appropriate decompressor,
/// if any. If empty or not supplied, the set of supported built-in compressors is used.</param>
/// <returns>
/// A pointer to a decompressor object that is acceptable per the supplied header, or to nullptr if no matching
/// algorithm is found.
/// </returns>
_ASYNCRTIMP std::unique_ptr<decompress_provider> get_decompressor_from_header(
    const utility::string_t& encoding,
    header_types type,
    const std::vector<std::shared_ptr<decompress_factory>>& factories =
        std::vector<std::shared_ptr<decompress_factory>>());

/// <summary>
/// Helper function to compose a TE or Accept-Encoding header with supported, and possibly ranked, compression
/// algorithms.
/// </summary>
/// <param name="type">Specifies the type of header to be built; valid values are <c>header_type::te</c> and
/// <c>header_type::accept_encoding</c>.</param>
/// <param name="factories">A collection of factory objects for use in header construction. If empty or not
/// supplied, the set of supported built-in compressors is used.</param>
/// <returns>
/// A well-formed header, without the header name, specifying the acceptable ranked compression types.
/// </returns>
_ASYNCRTIMP utility::string_t build_supported_header(header_types type,
                                                     const std::vector<std::shared_ptr<decompress_factory>>& factories =
                                                         std::vector<std::shared_ptr<decompress_factory>>());
} // namespace details
} // namespace compression
} // namespace http
} // namespace web
