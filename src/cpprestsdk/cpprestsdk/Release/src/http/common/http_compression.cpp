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

#include "stdafx.h"

// CPPREST_EXCLUDE_COMPRESSION is set if we're on a platform that supports compression but we want to explicitly disable
// it. CPPREST_EXCLUDE_BROTLI is set if we want to explicitly disable Brotli compression support.
// CPPREST_EXCLUDE_WEBSOCKETS is a flag that now essentially means "no external dependencies". TODO: Rename

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS) && !defined(CPPREST_EXCLUDE_COMPRESSION)
#define CPPREST_HTTP_COMPRESSION
#endif // !defined(CPPREST_EXCLUDE_WEBSOCKETS) && !defined(CPPREST_EXCLUDE_COMPRESSION)

#if defined(CPPREST_HTTP_COMPRESSION)
#include <zlib.h>
// zconf.h may define compress
#ifdef compress
#undef compress
#endif
#if !defined(CPPREST_EXCLUDE_BROTLI)
#define CPPREST_BROTLI_COMPRESSION
#endif // CPPREST_EXCLUDE_BROTLI
#if defined(CPPREST_BROTLI_COMPRESSION)
#include <brotli/decode.h>
#include <brotli/encode.h>
#endif // CPPREST_BROTLI_COMPRESSION
#endif

namespace web
{
namespace http
{
namespace compression
{
namespace builtin
{
#if defined(CPPREST_HTTP_COMPRESSION)
// A shared base class for the gzip and deflate compressors
class zlib_compressor_base : public compress_provider
{
public:
    static const utility::string_t GZIP;
    static const utility::string_t DEFLATE;

    zlib_compressor_base(int windowBits,
                         int compressionLevel = Z_DEFAULT_COMPRESSION,
                         int method = Z_DEFLATED,
                         int strategy = Z_DEFAULT_STRATEGY,
                         int memLevel = MAX_MEM_LEVEL)
        : m_algorithm(windowBits >= 16 ? GZIP : DEFLATE)
    {
        m_state = deflateInit2(&m_stream, compressionLevel, method, windowBits, memLevel, strategy);
    }

    const utility::string_t& algorithm() const { return m_algorithm; }

    size_t compress(const uint8_t* input,
                    size_t input_size,
                    uint8_t* output,
                    size_t output_size,
                    operation_hint hint,
                    size_t& input_bytes_processed,
                    bool& done)
    {
        if (m_state == Z_STREAM_END || (hint != operation_hint::is_last && !input_size))
        {
            input_bytes_processed = 0;
            done = (m_state == Z_STREAM_END);
            return 0;
        }

        if (m_state != Z_OK && m_state != Z_BUF_ERROR && m_state != Z_STREAM_ERROR)
        {
            throw std::runtime_error("Prior unrecoverable compression stream error " + std::to_string(m_state));
        }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-compare"
#endif // __clang__
        if (input_size > (std::numeric_limits<uInt>::max)() || output_size > (std::numeric_limits<uInt>::max)())
#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
        {
            throw std::runtime_error("Compression input or output size out of range");
        }

        m_stream.next_in = const_cast<Bytef*>(input);
        m_stream.avail_in = static_cast<uInt>(input_size);
        m_stream.next_out = const_cast<Bytef*>(output);
        m_stream.avail_out = static_cast<uInt>(output_size);

        m_state = deflate(&m_stream, (hint == operation_hint::is_last) ? Z_FINISH : Z_PARTIAL_FLUSH);
        if (m_state != Z_OK && m_state != Z_STREAM_ERROR &&
            !(hint == operation_hint::is_last && (m_state == Z_STREAM_END || m_state == Z_BUF_ERROR)))
        {
            throw std::runtime_error("Unrecoverable compression stream error " + std::to_string(m_state));
        }

        input_bytes_processed = input_size - m_stream.avail_in;
        done = (m_state == Z_STREAM_END);
        return output_size - m_stream.avail_out;
    }

    pplx::task<operation_result> compress(
        const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint)
    {
        operation_result r;

        try
        {
            r.output_bytes_produced =
                compress(input, input_size, output, output_size, hint, r.input_bytes_processed, r.done);
        }
        catch (...)
        {
            pplx::task_completion_event<operation_result> ev;
            ev.set_exception(std::current_exception());
            return pplx::create_task(ev);
        }

        return pplx::task_from_result<operation_result>(r);
    }

    void reset()
    {
        m_state = deflateReset(&m_stream);
        if (m_state != Z_OK)
        {
            throw std::runtime_error("Failed to reset zlib compressor " + std::to_string(m_state));
        }
    }

    ~zlib_compressor_base() { (void)deflateEnd(&m_stream); }

private:
    int m_state {Z_BUF_ERROR};
    z_stream m_stream {};
    const utility::string_t& m_algorithm;
};

const utility::string_t zlib_compressor_base::GZIP(algorithm::GZIP);
const utility::string_t zlib_compressor_base::DEFLATE(algorithm::DEFLATE);

// A shared base class for the gzip and deflate decompressors
class zlib_decompressor_base : public decompress_provider
{
public:
    zlib_decompressor_base(int windowBits)
        : m_algorithm(windowBits >= 16 ? zlib_compressor_base::GZIP : zlib_compressor_base::DEFLATE)
    {
        m_state = inflateInit2(&m_stream, windowBits);
    }

    const utility::string_t& algorithm() const { return m_algorithm; }

    size_t decompress(const uint8_t* input,
                      size_t input_size,
                      uint8_t* output,
                      size_t output_size,
                      operation_hint hint,
                      size_t& input_bytes_processed,
                      bool& done)
    {
        if (m_state == Z_STREAM_END || !input_size)
        {
            input_bytes_processed = 0;
            done = (m_state == Z_STREAM_END);
            return 0;
        }

        if (m_state != Z_OK && m_state != Z_BUF_ERROR && m_state != Z_STREAM_ERROR)
        {
            throw std::runtime_error("Prior unrecoverable decompression stream error " + std::to_string(m_state));
        }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-compare"
#endif // __clang__
        if (input_size > (std::numeric_limits<uInt>::max)() || output_size > (std::numeric_limits<uInt>::max)())
#if defined(__clang__)
#pragma clang diagnostic pop
#endif // __clang__
        {
            throw std::runtime_error("Compression input or output size out of range");
        }

        m_stream.next_in = const_cast<Bytef*>(input);
        m_stream.avail_in = static_cast<uInt>(input_size);
        m_stream.next_out = const_cast<Bytef*>(output);
        m_stream.avail_out = static_cast<uInt>(output_size);

        m_state = inflate(&m_stream, (hint == operation_hint::is_last) ? Z_FINISH : Z_PARTIAL_FLUSH);
        if (m_state != Z_OK && m_state != Z_STREAM_ERROR && m_state != Z_STREAM_END && m_state != Z_BUF_ERROR)
        {
            // Z_BUF_ERROR is a success code for Z_FINISH, and the caller can continue as if operation_hint::is_last was
            // not given
            throw std::runtime_error("Unrecoverable decompression stream error " + std::to_string(m_state));
        }

        input_bytes_processed = input_size - m_stream.avail_in;
        done = (m_state == Z_STREAM_END);
        return output_size - m_stream.avail_out;
    }

    pplx::task<operation_result> decompress(
        const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint)
    {
        operation_result r;

        try
        {
            r.output_bytes_produced =
                decompress(input, input_size, output, output_size, hint, r.input_bytes_processed, r.done);
        }
        catch (...)
        {
            pplx::task_completion_event<operation_result> ev;
            ev.set_exception(std::current_exception());
            return pplx::create_task(ev);
        }

        return pplx::task_from_result<operation_result>(r);
    }

    void reset()
    {
        m_state = inflateReset(&m_stream);
        if (m_state != Z_OK)
        {
            throw std::runtime_error("Failed to reset zlib decompressor " + std::to_string(m_state));
        }
    }

    ~zlib_decompressor_base() { (void)inflateEnd(&m_stream); }

private:
    int m_state {Z_BUF_ERROR};
    z_stream m_stream {};
    const utility::string_t& m_algorithm;
};

class gzip_compressor : public zlib_compressor_base
{
public:
    gzip_compressor() : zlib_compressor_base(31) // 15 is MAX_WBITS in zconf.h; add 16 for gzip
    {
    }

    gzip_compressor(int compressionLevel, int method, int strategy, int memLevel)
        : zlib_compressor_base(31, compressionLevel, method, strategy, memLevel)
    {
    }
};

class gzip_decompressor : public zlib_decompressor_base
{
public:
    gzip_decompressor() : zlib_decompressor_base(31) // 15 is MAX_WBITS in zconf.h; add 16 for gzip
    {
    }
};

class deflate_compressor : public zlib_compressor_base
{
public:
    deflate_compressor() : zlib_compressor_base(15) // 15 is MAX_WBITS in zconf.h
    {
    }

    deflate_compressor(int compressionLevel, int method, int strategy, int memLevel)
        : zlib_compressor_base(15, compressionLevel, method, strategy, memLevel)
    {
    }
};

class deflate_decompressor : public zlib_decompressor_base
{
public:
    deflate_decompressor() : zlib_decompressor_base(0) // deflate auto-detect
    {
    }
};

#if defined(CPPREST_BROTLI_COMPRESSION)
class brotli_compressor : public compress_provider
{
public:
    static const utility::string_t BROTLI;

    brotli_compressor(uint32_t window = BROTLI_DEFAULT_WINDOW,
                      uint32_t quality = BROTLI_DEFAULT_QUALITY,
                      uint32_t mode = BROTLI_DEFAULT_MODE,
                      uint32_t block = 0,
                      uint32_t nomodel = 0,
                      uint32_t hint = 0)
        : m_window(window)
        , m_quality(quality)
        , m_mode(mode)
        , m_block(block)
        , m_nomodel(nomodel)
        , m_hint(hint)
        , m_algorithm(BROTLI)
    {
        (void)reset();
    }

    const utility::string_t& algorithm() const { return m_algorithm; }

    size_t compress(const uint8_t* input,
                    size_t input_size,
                    uint8_t* output,
                    size_t output_size,
                    operation_hint hint,
                    size_t& input_bytes_processed,
                    bool& done)
    {
        if (m_done || (hint != operation_hint::is_last && !input_size))
        {
            input_bytes_processed = 0;
            done = m_done;
            return 0;
        }

        if (m_state != BROTLI_TRUE)
        {
            throw std::runtime_error("Prior unrecoverable compression stream error");
        }

        const uint8_t* next_in = input;
        size_t avail_in = 0;
        uint8_t* next_out = output;
        size_t avail_out = output_size;
        size_t total_out;

        if (BrotliEncoderHasMoreOutput(m_stream) == BROTLI_TRUE)
        {
            // Drain any compressed bytes remaining from a prior call
            do
            {
                m_state = BrotliEncoderCompressStream(
                    m_stream, BROTLI_OPERATION_FLUSH, &avail_in, &next_in, &avail_out, &next_out, &total_out);
            } while (m_state == BROTLI_TRUE && avail_out && BrotliEncoderHasMoreOutput(m_stream) == BROTLI_TRUE);
        }

        if (m_state == BROTLI_TRUE && avail_out && input_size)
        {
            // Compress the caller-supplied buffer
            avail_in = input_size;
            do
            {
                m_state = BrotliEncoderCompressStream(
                    m_stream, BROTLI_OPERATION_FLUSH, &avail_in, &next_in, &avail_out, &next_out, &total_out);
            } while (m_state == BROTLI_TRUE && avail_out && BrotliEncoderHasMoreOutput(m_stream) == BROTLI_TRUE);
        }
        else
        {
            // We're not compressing any new data; ensure calculation sanity
            input_size = 0;
        }

        if (m_state != BROTLI_TRUE)
        {
            throw std::runtime_error("Unrecoverable compression stream error");
        }

        if (hint == operation_hint::is_last)
        {
            if (avail_out)
            {
                // Make one more pass to finalize the compressed stream
                _ASSERTE(!avail_in);
                m_state = BrotliEncoderCompressStream(
                    m_stream, BROTLI_OPERATION_FINISH, &avail_in, &next_in, &avail_out, &next_out, &total_out);
                if (m_state != BROTLI_TRUE)
                {
                    throw std::runtime_error("Unrecoverable error finalizing compression stream");
                }
                m_done = (BrotliEncoderIsFinished(m_stream) == BROTLI_TRUE);
            }
        }

        input_bytes_processed = input_size - avail_in;
        done = m_done;
        return output_size - avail_out;
    }

    pplx::task<operation_result> compress(
        const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint)
    {
        operation_result r;

        try
        {
            r.output_bytes_produced =
                compress(input, input_size, output, output_size, hint, r.input_bytes_processed, r.done);
        }
        catch (...)
        {
            pplx::task_completion_event<operation_result> ev;
            ev.set_exception(std::current_exception());
            return pplx::create_task(ev);
        }

        return pplx::task_from_result<operation_result>(r);
    }

    void reset()
    {
        if (m_stream)
        {
            BrotliEncoderDestroyInstance(m_stream);
        }

        m_stream = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
        m_state = m_stream ? BROTLI_TRUE : BROTLI_FALSE;

        if (m_state == BROTLI_TRUE && m_window != BROTLI_DEFAULT_WINDOW)
        {
            m_state = BrotliEncoderSetParameter(m_stream, BROTLI_PARAM_LGWIN, m_window);
        }
        if (m_state == BROTLI_TRUE && m_quality != BROTLI_DEFAULT_QUALITY)
        {
            m_state = BrotliEncoderSetParameter(m_stream, BROTLI_PARAM_QUALITY, m_quality);
        }
        if (m_state == BROTLI_TRUE && m_mode != BROTLI_DEFAULT_MODE)
        {
            m_state = BrotliEncoderSetParameter(m_stream, BROTLI_PARAM_MODE, m_mode);
        }
        if (m_state == BROTLI_TRUE && m_block != 0)
        {
            m_state = BrotliEncoderSetParameter(m_stream, BROTLI_PARAM_LGBLOCK, m_block);
        }
        if (m_state == BROTLI_TRUE && m_nomodel != 0)
        {
            m_state = BrotliEncoderSetParameter(m_stream, BROTLI_PARAM_DISABLE_LITERAL_CONTEXT_MODELING, m_nomodel);
        }
        if (m_state == BROTLI_TRUE && m_hint != 0)
        {
            m_state = BrotliEncoderSetParameter(m_stream, BROTLI_PARAM_SIZE_HINT, m_hint);
        }

        if (m_state != BROTLI_TRUE)
        {
            throw std::runtime_error("Failed to reset Brotli compressor");
        }
    }

    ~brotli_compressor()
    {
        if (m_stream)
        {
            BrotliEncoderDestroyInstance(m_stream);
        }
    }

private:
    BROTLI_BOOL m_state {BROTLI_FALSE};
    BrotliEncoderState* m_stream {nullptr};
    bool m_done {false};
    uint32_t m_window;
    uint32_t m_quality;
    uint32_t m_mode;
    uint32_t m_block;
    uint32_t m_nomodel;
    uint32_t m_hint;
    const utility::string_t& m_algorithm;
};

const utility::string_t brotli_compressor::BROTLI(algorithm::BROTLI);

class brotli_decompressor : public decompress_provider
{
public:
    brotli_decompressor() : m_algorithm(brotli_compressor::BROTLI)
    {
        try
        {
            reset();
        }
        catch (...)
        {
        }
    }

    const utility::string_t& algorithm() const { return m_algorithm; }

    size_t decompress(const uint8_t* input,
                      size_t input_size,
                      uint8_t* output,
                      size_t output_size,
                      operation_hint hint,
                      size_t& input_bytes_processed,
                      bool& done)
    {
        if (m_state == BROTLI_DECODER_RESULT_SUCCESS /* || !input_size*/)
        {
            input_bytes_processed = 0;
            done = (m_state == BROTLI_DECODER_RESULT_SUCCESS);
            return 0;
        }

        if (m_state == BROTLI_DECODER_RESULT_ERROR)
        {
            throw std::runtime_error("Prior unrecoverable decompression stream error");
        }

        const uint8_t* next_in = input;
        size_t avail_in = input_size;
        uint8_t* next_out = output;
        size_t avail_out = output_size;
        size_t total_out;

        // N.B. we ignore 'hint' here.  We could instead call BrotliDecoderDecompress() if it's set, but we'd either
        // have to first allocate a guaranteed-large-enough buffer and then copy out of it, or we'd have to call
        // reset() if it failed due to insufficient output buffer space (and we'd need to use
        // BrotliDecoderGetErrorCode() to tell if that's why it failed)
        (void)hint;
        m_state = BrotliDecoderDecompressStream(m_stream, &avail_in, &next_in, &avail_out, &next_out, &total_out);
        if (m_state == BROTLI_DECODER_RESULT_ERROR)
        {
            throw std::runtime_error("Unrecoverable decompression stream error");
        }

        input_bytes_processed = input_size - avail_in;
        done = (m_state == BROTLI_DECODER_RESULT_SUCCESS);
        return output_size - avail_out;
    }

    pplx::task<operation_result> decompress(
        const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint)
    {
        operation_result r;

        try
        {
            r.output_bytes_produced =
                decompress(input, input_size, output, output_size, hint, r.input_bytes_processed, r.done);
        }
        catch (...)
        {
            pplx::task_completion_event<operation_result> ev;
            ev.set_exception(std::current_exception());
            return pplx::create_task(ev);
        }

        return pplx::task_from_result<operation_result>(r);
    }

    void reset()
    {
        if (m_stream)
        {
            BrotliDecoderDestroyInstance(m_stream);
        }

        m_stream = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        m_state = m_stream ? BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT : BROTLI_DECODER_RESULT_ERROR;

        if (m_state != BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
        {
            throw std::runtime_error("Failed to reset Brotli decompressor");
        }
    }

    ~brotli_decompressor()
    {
        if (m_stream)
        {
            BrotliDecoderDestroyInstance(m_stream);
        }
    }

private:
    BrotliDecoderResult m_state {BROTLI_DECODER_RESULT_ERROR};
    BrotliDecoderState* m_stream {nullptr};
    const utility::string_t& m_algorithm;
};
#endif // CPPREST_BROTLI_COMPRESSION
#endif // CPPREST_HTTP_COMPRESSION

// Generic internal implementation of the compress_factory API
class generic_compress_factory : public compress_factory
{
public:
    ~generic_compress_factory() CPPREST_NOEXCEPT {}
    generic_compress_factory(const utility::string_t& algorithm,
                             std::function<std::unique_ptr<compress_provider>()> make_compressor)
        : m_algorithm(algorithm), _make_compressor(make_compressor)
    {
    }

    const utility::string_t& algorithm() const { return m_algorithm; }

    std::unique_ptr<compress_provider> make_compressor() const { return _make_compressor(); }

private:
    const utility::string_t m_algorithm;
    std::function<std::unique_ptr<compress_provider>()> _make_compressor;
};

// Generic internal implementation of the decompress_factory API
class generic_decompress_factory : public decompress_factory
{
public:
    ~generic_decompress_factory() CPPREST_NOEXCEPT {}
    generic_decompress_factory(const utility::string_t& algorithm,
                               uint16_t weight,
                               std::function<std::unique_ptr<decompress_provider>()> make_decompressor)
        : m_algorithm(algorithm), m_weight(weight), _make_decompressor(make_decompressor)
    {
    }

    const utility::string_t& algorithm() const { return m_algorithm; }

    uint16_t weight() const { return m_weight; }

    std::unique_ptr<decompress_provider> make_decompressor() const { return _make_decompressor(); }

private:
    const utility::string_t m_algorithm;
    uint16_t m_weight;
    std::function<std::unique_ptr<decompress_provider>()> _make_decompressor;
};

// "Private" algorithm-to-factory tables for namespace static helpers
static const std::vector<std::shared_ptr<compress_factory>> g_compress_factories
#if defined(CPPREST_HTTP_COMPRESSION)
    = {std::make_shared<generic_compress_factory>(
           algorithm::GZIP,
           []() -> std::unique_ptr<compress_provider> { return utility::details::make_unique<gzip_compressor>(); }),
       std::make_shared<generic_compress_factory>(
           algorithm::DEFLATE,
           []() -> std::unique_ptr<compress_provider> { return utility::details::make_unique<deflate_compressor>(); }),
#if defined(CPPREST_BROTLI_COMPRESSION)
       std::make_shared<generic_compress_factory>(
           algorithm::BROTLI,
           []() -> std::unique_ptr<compress_provider> { return utility::details::make_unique<brotli_compressor>(); })
#endif // CPPREST_BROTLI_COMPRESSION
};
#else  // CPPREST_HTTP_COMPRESSION
    ;
#endif // CPPREST_HTTP_COMPRESSION

static const std::vector<std::shared_ptr<decompress_factory>> g_decompress_factories
#if defined(CPPREST_HTTP_COMPRESSION)
    = {std::make_shared<generic_decompress_factory>(
           algorithm::GZIP,
           500,
           []() -> std::unique_ptr<decompress_provider> { return utility::details::make_unique<gzip_decompressor>(); }),
       std::make_shared<generic_decompress_factory>(algorithm::DEFLATE,
                                                    500,
                                                    []() -> std::unique_ptr<decompress_provider> {
                                                        return utility::details::make_unique<deflate_decompressor>();
                                                    }),
#if defined(CPPREST_BROTLI_COMPRESSION)
       std::make_shared<generic_decompress_factory>(algorithm::BROTLI,
                                                    500,
                                                    []() -> std::unique_ptr<decompress_provider> {
                                                        return utility::details::make_unique<brotli_decompressor>();
                                                    })
#endif // CPPREST_BROTLI_COMPRESSION
};
#else  // CPPREST_HTTP_COMPRESSION
    ;
#endif // CPPREST_HTTP_COMPRESSION

bool supported() { return !g_compress_factories.empty(); }

bool algorithm::supported(const utility::string_t& algorithm)
{
    for (auto& factory : g_compress_factories)
    {
        if (utility::details::str_iequal(algorithm, factory->algorithm()))
        {
            return true;
        }
    }

    return false;
}

static std::unique_ptr<compress_provider> _make_compressor(
    const std::vector<std::shared_ptr<compress_factory>>& factories, const utility::string_t& algorithm)
{
    for (auto& factory : factories)
    {
        if (factory && utility::details::str_iequal(algorithm, factory->algorithm()))
        {
            return factory->make_compressor();
        }
    }

    return std::unique_ptr<compress_provider>();
}

std::unique_ptr<compress_provider> make_compressor(const utility::string_t& algorithm)
{
    return _make_compressor(g_compress_factories, algorithm);
}

static std::unique_ptr<decompress_provider> _make_decompressor(
    const std::vector<std::shared_ptr<decompress_factory>>& factories, const utility::string_t& algorithm)
{
    for (auto& factory : factories)
    {
        if (factory && utility::details::str_iequal(algorithm, factory->algorithm()))
        {
            return factory->make_decompressor();
        }
    }

    return std::unique_ptr<decompress_provider>();
}

std::unique_ptr<decompress_provider> make_decompressor(const utility::string_t& algorithm)
{
    return _make_decompressor(g_decompress_factories, algorithm);
}

std::shared_ptr<compress_factory> get_compress_factory(const utility::string_t& algorithm)
{
    for (auto& factory : g_compress_factories)
    {
        if (utility::details::str_iequal(algorithm, factory->algorithm()))
        {
            return factory;
        }
    }

    return std::shared_ptr<compress_factory>();
}

std::shared_ptr<decompress_factory> get_decompress_factory(const utility::string_t& algorithm)
{
    for (auto& factory : g_decompress_factories)
    {
        if (utility::details::str_iequal(algorithm, factory->algorithm()))
        {
            return factory;
        }
    }

    return std::shared_ptr<decompress_factory>();
}

std::unique_ptr<compress_provider> make_gzip_compressor(int compressionLevel, int method, int strategy, int memLevel)
{
#if defined(CPPREST_HTTP_COMPRESSION)
    return utility::details::make_unique<gzip_compressor>(compressionLevel, method, strategy, memLevel);
#else  // CPPREST_HTTP_COMPRESSION
    (void)compressionLevel;
    (void)method;
    (void)strategy;
    (void)memLevel;
    return std::unique_ptr<compress_provider>();
#endif // CPPREST_HTTP_COMPRESSION
}

std::unique_ptr<compress_provider> make_deflate_compressor(int compressionLevel, int method, int strategy, int memLevel)
{
#if defined(CPPREST_HTTP_COMPRESSION)
    return utility::details::make_unique<deflate_compressor>(compressionLevel, method, strategy, memLevel);
#else  // CPPREST_HTTP_COMPRESSION
    (void)compressionLevel;
    (void)method;
    (void)strategy;
    (void)memLevel;
    return std::unique_ptr<compress_provider>();
#endif // CPPREST_HTTP_COMPRESSION
}

std::unique_ptr<compress_provider> make_brotli_compressor(
    uint32_t window, uint32_t quality, uint32_t mode, uint32_t block, uint32_t nomodel, uint32_t hint)
{
#if defined(CPPREST_HTTP_COMPRESSION) && defined(CPPREST_BROTLI_COMPRESSION)
    return utility::details::make_unique<brotli_compressor>(window, quality, mode, block, nomodel, hint);
#else  // CPPREST_BROTLI_COMPRESSION
    (void)window;
    (void)quality;
    (void)mode;
    (void)block;
    (void)nomodel;
    (void)hint;
    return std::unique_ptr<compress_provider>();
#endif // CPPREST_BROTLI_COMPRESSION
}
} // namespace builtin

std::shared_ptr<compress_factory> make_compress_factory(
    const utility::string_t& algorithm, std::function<std::unique_ptr<compress_provider>()> make_compressor)
{
    return std::make_shared<builtin::generic_compress_factory>(algorithm, make_compressor);
}

std::shared_ptr<decompress_factory> make_decompress_factory(
    const utility::string_t& algorithm,
    uint16_t weight,
    std::function<std::unique_ptr<decompress_provider>()> make_decompressor)
{
    return std::make_shared<builtin::generic_decompress_factory>(algorithm, weight, make_decompressor);
}

namespace details
{
namespace builtin
{
const std::vector<std::shared_ptr<decompress_factory>> get_decompress_factories()
{
    return web::http::compression::builtin::g_decompress_factories;
}
} // namespace builtin

static bool is_http_whitespace(const utility::char_t ch) { return ch == _XPLATSTR(' ') || ch == _XPLATSTR('\t'); }

static void remove_surrounding_http_whitespace(const utility::string_t& encoding, size_t& start, size_t& length)
{
    while (length > 0 && is_http_whitespace(encoding.at(start)))
    {
        start++;
        length--;
    }
    while (length > 0 && is_http_whitespace(encoding.at(start + length - 1)))
    {
        length--;
    }
}

std::unique_ptr<compress_provider> get_compressor_from_header(
    const utility::string_t& encoding,
    header_types type,
    const std::vector<std::shared_ptr<compress_factory>>& factories)
{
    const std::vector<std::shared_ptr<compress_factory>>& f =
        factories.empty() ? web::http::compression::builtin::g_compress_factories : factories;
    std::unique_ptr<compress_provider> compressor;
    struct _tuple
    {
        size_t start;
        size_t length;
        size_t rank;
    } t;
    std::vector<_tuple> tokens;
    size_t highest;
    size_t mark;
    size_t end;
    size_t n;
    bool first;

    _ASSERTE(type == header_types::te || type == header_types::accept_encoding);

    // See https://tools.ietf.org/html/rfc7230#section-4.3 (TE) and
    // https://tools.ietf.org/html/rfc7231#section-5.3.4 (Accept-Encoding) for details

    n = 0;
    highest = 0;
    first = true;
    while (n != utility::string_t::npos)
    {
        // Tokenize by commas first
        mark = encoding.find(_XPLATSTR(','), n);
        t.start = n;
        t.rank = static_cast<size_t>(-1);
        if (mark == utility::string_t::npos)
        {
            t.length = encoding.size() - n;
            n = utility::string_t::npos;
        }
        else
        {
            t.length = mark - n;
            n = mark + 1;
        }

        // Then remove leading and trailing whitespace
        remove_surrounding_http_whitespace(encoding, t.start, t.length);

        // Next split at the semicolon, if any, and deal with rank and additional whitespace
        mark = encoding.find(_XPLATSTR(';'), t.start);
        if (mark < t.start + t.length)
        {
            end = t.start + t.length - 1;
            t.length = mark - t.start;
            while (t.length > 0 && is_http_whitespace(encoding.at(t.start + t.length - 1)))
            {
                // Skip trailing whitespace in encoding type
                t.length--;
            }
            if (mark < end)
            {
                // Check for an optional ranking, max. length "q=0.999"
                mark = encoding.find(_XPLATSTR("q="), mark + 1);
                if (mark != utility::string_t::npos && mark + 1 < end && end - mark <= 6)
                {
                    // Determine ranking; leading whitespace has been implicitly skipped by find().
                    // The ranking always starts with '1' or '0' per standard, and has at most 3 decimal places
                    mark += 1;
                    t.rank = 1000 * (encoding.at(mark + 1) - _XPLATSTR('0'));
                    if (mark + 2 < end && encoding.at(mark + 2) == _XPLATSTR('.'))
                    {
                        // This is a real number rank; convert decimal part to hundreds and apply it
                        size_t factor = 100;
                        mark += 2;
                        for (size_t i = mark + 1; i <= end; i++)
                        {
                            t.rank += (encoding.at(i) - _XPLATSTR('0')) * factor;
                            factor /= 10;
                        }
                    }
                    if (t.rank > 1000)
                    {
                        throw http_exception(status_codes::BadRequest, "Invalid q-value in header");
                    }
                }
            }
        }

        if (!t.length)
        {
            if (!first || n != utility::string_t::npos)
            {
                // An entirely empty header is OK per RFC, but an extraneous comma is not
                throw http_exception(status_codes::BadRequest, "Empty field in header");
            }
            return std::unique_ptr<compress_provider>();
        }

        if (!compressor)
        {
            if (t.rank == static_cast<size_t>(1000) || t.rank == static_cast<size_t>(-1))
            {
                // Immediately try to instantiate a compressor for any unranked or top-ranked algorithm
                compressor = web::http::compression::builtin::_make_compressor(f, encoding.substr(t.start, t.length));
            }
            else if (t.rank)
            {
                // Store off remaining ranked algorithms, sorting as we go
                if (t.rank >= highest)
                {
                    tokens.emplace_back(t);
                    highest = t.rank;
                }
                else
                {
                    for (auto x = tokens.begin(); x != tokens.end(); x++)
                    {
                        if (t.rank <= x->rank)
                        {
                            tokens.emplace(x, t);
                            break;
                        }
                    }
                }
            }
            // else a rank of 0 means "not permitted"
        }
        // else we've chosen a compressor; we're just validating the rest of the header

        first = false;
    }
    // Note: for Accept-Encoding, we don't currently explicitly handle "identity;q=0" and "*;q=0"

    if (compressor)
    {
        return compressor;
    }

    // If we're here, we didn't match the caller's compressor above;
    // try any that we saved off in order of highest to lowest rank
    for (auto t = tokens.rbegin(); t != tokens.rend(); t++)
    {
        auto coding = encoding.substr(t->start, t->length);

        // N.B for TE, "trailers" will simply fail to instantiate a
        // compressor; ditto for "*" and "identity" for Accept-Encoding
        auto compressor = web::http::compression::builtin::_make_compressor(f, coding);
        if (compressor)
        {
            return compressor;
        }
        if (type == header_types::accept_encoding && utility::details::str_iequal(coding, _XPLATSTR("identity")))
        {
            // The client specified a preference for "no encoding" vs. anything else we might still have
            return std::unique_ptr<compress_provider>();
        }
    }

    return std::unique_ptr<compress_provider>();
}

std::unique_ptr<decompress_provider> get_decompressor_from_header(
    const utility::string_t& encoding,
    header_types type,
    const std::vector<std::shared_ptr<decompress_factory>>& factories)
{
    const std::vector<std::shared_ptr<decompress_factory>>& f =
        factories.empty() ? web::http::compression::builtin::g_decompress_factories : factories;
    std::unique_ptr<decompress_provider> decompressor;
    utility::string_t token;
    size_t start;
    size_t length;
    size_t comma;
    size_t n;

    _ASSERTE(type == header_types::transfer_encoding || type == header_types::content_encoding);

    n = 0;
    while (n != utility::string_t::npos)
    {
        // Tokenize by commas first
        comma = encoding.find(_XPLATSTR(','), n);
        start = n;
        if (comma == utility::string_t::npos)
        {
            length = encoding.size() - n;
            n = utility::string_t::npos;
        }
        else
        {
            length = comma - n;
            n = comma + 1;
        }

        // Then remove leading and trailing whitespace
        remove_surrounding_http_whitespace(encoding, start, length);

        if (!length)
        {
            throw http_exception(status_codes::BadRequest, "Empty field in header");
        }

        // Immediately try to instantiate a decompressor
        token = encoding.substr(start, length);
        auto d = web::http::compression::builtin::_make_decompressor(f, token);
        if (d)
        {
            if (decompressor)
            {
                status_code code = status_codes::NotImplemented;
                if (type == header_types::content_encoding)
                {
                    code = status_codes::UnsupportedMediaType;
                }
                throw http_exception(code, "Multiple compression algorithms not supported for a single request");
            }

            // We found our decompressor; store it off while we process the rest of the header
            decompressor = std::move(d);
        }
        else
        {
            if (n != utility::string_t::npos)
            {
                if (type == header_types::transfer_encoding &&
                    utility::details::str_iequal(_XPLATSTR("chunked"), token))
                {
                    throw http_exception(status_codes::BadRequest,
                                         "Chunked must come last in the Transfer-Encoding header");
                }
            }
            if (!decompressor && !f.empty() && (n != utility::string_t::npos || type == header_types::content_encoding))
            {
                // The first encoding type did not match; throw an informative
                // exception with an encoding-type-appropriate HTTP error code
                status_code code = status_codes::NotImplemented;
                if (type == header_types::content_encoding)
                {
                    code = status_codes::UnsupportedMediaType;
                }
                throw http_exception(code, "Unsupported encoding type");
            }
        }
    }

    if (type == header_types::transfer_encoding && !utility::details::str_iequal(_XPLATSTR("chunked"), token))
    {
        throw http_exception(status_codes::BadRequest, "Transfer-Encoding header missing chunked");
    }

    // Either the response is compressed and we have a decompressor that can handle it, or
    // built-in compression is not enabled and we don't have an alternate set of decompressors
    return decompressor;
}

utility::string_t build_supported_header(header_types type,
                                         const std::vector<std::shared_ptr<decompress_factory>>& factories)
{
    const std::vector<std::shared_ptr<decompress_factory>>& f =
        factories.empty() ? web::http::compression::builtin::g_decompress_factories : factories;
    utility::string_t result;
    bool start;

    _ASSERTE(type == header_types::te || type == header_types::accept_encoding);

    // Add all specified algorithms and their weights to the header
    start = true;
    for (auto& factory : f)
    {
        if (factory)
        {
            auto weight = factory->weight();

            if (!start)
            {
                result += _XPLATSTR(", ");
            }
            result += factory->algorithm();
            if (weight <= 1000)
            {
                result += _XPLATSTR(";q=");
                result += utility::conversions::details::to_string_t(weight / 1000);
                result += _XPLATSTR('.');
                result += utility::conversions::details::to_string_t(weight % 1000);
            }
            start = false;
        }
    }

    if (start && type == header_types::accept_encoding)
    {
        // Request that no encoding be applied
        result += _XPLATSTR("identity;q=1, *;q=0");
    }

    return result;
}
} // namespace details
} // namespace compression
} // namespace http
} // namespace web
