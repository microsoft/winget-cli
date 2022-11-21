#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

template <typename T>
inline constexpr std::uint8_t* bigendian_copy(T value, std::uint8_t* target) noexcept
{
    std::size_t shift = (sizeof(T) - 1) * 8;
    T mask = static_cast<T>(0xFF) << shift;
    for (std::size_t i = 0; i < sizeof(T); ++i)
    {
        *(target++) = static_cast<std::uint8_t>((value & mask) >> shift);
        mask >>= 8;
        shift -= 8;
    }

    return target;
}

struct sha1
{
    static constexpr std::size_t chunk_size_bits = 512;
    static constexpr std::size_t chunk_size_bytes = chunk_size_bits / 8;
    static constexpr std::size_t chunk_size_ints = chunk_size_bytes / 4;

    constexpr void reset() noexcept
    {
        m_sizeBytes = 0;
        m_nextChunkByte = 0;
        m_state = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    }

    constexpr void append(std::uint8_t const* data, std::uint64_t count) noexcept
    {
        while (count > 0)
        {
            auto bytesToCopy = (std::min)(count, chunk_size_bytes - m_nextChunkByte);
            for (std::size_t i = 0; i < bytesToCopy; ++i)
            {
                append_byte(data[i]);
            }

            if (m_nextChunkByte == chunk_size_bytes)
            {
                process_chunk();
            }

            count -= bytesToCopy;
            data += bytesToCopy;
        }
    }

    void append(std::string_view str) noexcept
    {
        append(reinterpret_cast<std::uint8_t const*>(str.data()), str.length());
    }

    constexpr std::array<std::uint8_t, 20> finalize() noexcept
    {
        auto const sizeBits = m_sizeBytes * 8;
        append_byte(0x80);

        constexpr auto sizeOffset = chunk_size_bytes - 8;
        while (m_nextChunkByte < sizeOffset)
        {
            append_byte(0);
        }

        // We need to append the length to the very end, which means that we may need to process a mostly empty
        // additional chunk
        if (m_nextChunkByte > sizeOffset)
        {
            while (m_nextChunkByte < chunk_size_bytes)
            {
                append_byte(0);
            }
            process_chunk();
        }

        m_currentChunk[chunk_size_ints - 2] = static_cast<std::uint32_t>(sizeBits >> 32);
        m_currentChunk[chunk_size_ints - 1] = static_cast<std::uint32_t>(sizeBits);
        m_nextChunkByte = chunk_size_bytes;
        process_chunk();

        std::array<std::uint8_t, 20> result = {};
        auto dest = result.data();
        for (auto value : m_state)
        {
            dest = bigendian_copy(value, dest);
        }

        reset();
        return result;
    }

private:

    constexpr void append_byte(std::uint8_t byte) noexcept
    {
        auto index = static_cast<std::size_t>(m_nextChunkByte / 4);
        auto pos = m_nextChunkByte % 4;
        auto shift = (3 - pos) * 8;
        m_currentChunk[index] |= (byte << shift);
        ++m_nextChunkByte;
        ++m_sizeBytes;
    }

    constexpr void process_chunk() noexcept
    {
        XLANG_ASSERT(m_nextChunkByte == chunk_size_bytes);
        auto chunkState = m_state;

        std::array<std::uint32_t, 80> w = {};
        for (auto i = 0; i < 16; ++i) w[i] = m_currentChunk[i];
        for (auto i = 16; i < 80; ++i)
        {
            w[i] = lrot(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        for (auto i = 0; i < 20; ++i)
        {
            auto f = (chunkState[1] & chunkState[2]) | (~chunkState[1] & chunkState[3]);
            rotate(chunkState, w[i], f, 0x5A827999);
        }
        for (auto i = 20; i < 40; ++i)
        {
            auto f = chunkState[1] ^ chunkState[2] ^ chunkState[3];
            rotate(chunkState, w[i], f, 0x6ED9EBA1);
        }
        for (auto i = 40; i < 60; ++i)
        {
            auto f = (chunkState[1] & chunkState[2]) | (chunkState[1] & chunkState[3]) | (chunkState[2] & chunkState[3]);
            rotate(chunkState, w[i], f, 0x8F1BBCDC);
        }
        for (auto i = 60; i < 80; ++i)
        {
            auto f = chunkState[1] ^ chunkState[2] ^ chunkState[3];
            rotate(chunkState, w[i], f, 0xCA62C1D6);
        }

        for (auto i = 0; i < 5; ++i)
        {
            m_state[i] += chunkState[i];
        }
        m_nextChunkByte = 0;
        m_currentChunk = {};
    }

    template <typename T>
    static constexpr T lrot(T value, std::size_t count) noexcept
    {
        using U = std::make_unsigned_t<T>;
        auto result = value << count;
        result |= static_cast<U>(value) >> ((sizeof(T) * 8) - count);
        return result;
    }

    static constexpr void rotate(std::array<std::uint32_t, 5>& state, std::uint32_t value, std::uint32_t f, std::uint32_t k) noexcept
    {
        auto temp = lrot(state[0], 5) + state[4] + f + k + value;
        state[4] = state[3];
        state[3] = state[2];
        state[2] = lrot(state[1], 30);
        state[1] = state[0];
        state[0] = temp;
    }

    std::uint64_t m_sizeBytes = 0;

    std::array<std::uint32_t, 16> m_currentChunk = {};
    std::uint64_t m_nextChunkByte = 0;

    std::array<std::uint32_t, 5> m_state = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
};
