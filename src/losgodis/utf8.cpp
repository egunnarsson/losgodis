
#include "losgodis/utf8.hpp"

namespace losgodis::utf8
{

validation_result validate(byte_range range)
{
    size_t codepoint_count = 0;
    size_t i = 0;
    while (i < range.size())
    {
        const auto b1 = range[i];
        if (b1 < 0x80u) // 0xxxxxxx
        {
            i++;
            codepoint_count++;
        }
        else if (b1 < 0xC0u) // 10xxxxxx continuation byte
        {
            return { validation_error::unexpected_continuation_byte, range.to_utf8(i), codepoint_count };
        }
        else if (b1 < 0xE0u) // 110xxxxx 10xxxxxx
        {
            if (i + 1 >= range.size()) { return { validation_error::unexpected_end, range.to_utf8(i), codepoint_count }; }

            const auto b2 = range[i + 1];
            if ((b2 ^ 0x80u) & 0xC0u) { return { validation_error::unexpected_non_continuation_byte, range.to_utf8(i), codepoint_count }; }

            uint32_t codepoint = ((b1 & 0x1Fu) << 6) | (b2 & 0x3Fu);
            if (codepoint < 0x7Fu) { return { validation_error::overlong_enocoding, range.to_utf8(i), codepoint_count }; }

            i += 2;
            codepoint_count++;
        }
        else if (b1 < 0xF0u) // 1110xxxx 10xxxxxx 10xxxxxx
        {
            if (i + 2 >= range.size()) { return { validation_error::unexpected_end, range.to_utf8(i), codepoint_count }; }

            const auto b2 = range[i + 1];
            const auto b3 = range[i + 2];
            if ((b2 ^ 0x80u | b3 ^ 0x80u) & 0xC0u) { return { validation_error::unexpected_non_continuation_byte, range.to_utf8(i), codepoint_count }; }

            uint32_t codepoint = ((b1 & 0xFu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
            if (codepoint <= 0x7FFu) { return { validation_error::overlong_enocoding, range.to_utf8(i), codepoint_count }; }

            i += 3;
            codepoint_count++;
        }
        else if (b1 < 0xF8u) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        {
            if (i + 3 >= range.size()) { return { validation_error::unexpected_end, range.to_utf8(i), codepoint_count }; }

            const auto b2 = range[i + 1];
            const auto b3 = range[i + 2];
            const auto b4 = range[i + 3];
            if ((b2 ^ 0x80u | b3 ^ 0x80u | b4 ^ 0x80u) & 0xC0u) { return { validation_error::unexpected_non_continuation_byte, range.to_utf8(i), codepoint_count }; }

            uint32_t codepoint = ((b1 & 0x7u) << 18) | ((b2 & 0x3Fu) << 12) | ((b3 & 0x3Fu) << 6) | (b4 & 0x3Fu);
            if (codepoint > 0x10FFFFu) { return { validation_error::invalid_codepoint, range.to_utf8(i), codepoint_count }; }
            if (codepoint <= 0xFFFFu) { return { validation_error::overlong_enocoding, range.to_utf8(i), codepoint_count }; }

            i += 4;
            codepoint_count++;
        }
        else // 11111xxx
        {
            return { validation_error::invalid_byte, range.to_utf8(i), codepoint_count };
        }
    }

    return { validation_error::success, range.to_utf8(i), codepoint_count };
}

// do not check invalid_codepoint and overlong_enocoding
validation_result validate_quick(byte_range range)
{
    size_t codepoint_count = 0;
    size_t i = 0;
    while (i < range.size())
    {
        const auto b1 = range[i];
        if (b1 < 0x80u) // 0xxxxxxx
        {
            i++;
            codepoint_count++;
        }
        else if (b1 < 0xC0u) // 10xxxxxx continuation byte
        {
            return { validation_error::unexpected_continuation_byte, range.to_utf8(i), codepoint_count };
        }
        else if (b1 < 0xE0u) // 110xxxxx 10xxxxxx
        {
            if (i + 1 >= range.size()) { return { validation_error::unexpected_end, range.to_utf8(i), codepoint_count }; }

            const auto b2 = range[i + 1];
            if ((b2 ^ 0x80u) & 0xC0u) { return { validation_error::unexpected_non_continuation_byte, range.to_utf8(i), codepoint_count }; }

            i += 2;
            codepoint_count++;
        }
        else if (b1 < 0xF0u) // 1110xxxx 10xxxxxx 10xxxxxx
        {
            if (i + 2 >= range.size()) { return { validation_error::unexpected_end, range.to_utf8(i), codepoint_count }; }

            const auto b2 = range[i + 1];
            const auto b3 = range[i + 2];
            if ((b2 ^ 0x80u | b3 ^ 0x80u) & 0xC0u) { return { validation_error::unexpected_non_continuation_byte, range.to_utf8(i), codepoint_count }; }

            i += 3;
            codepoint_count++;
        }
        else if (b1 < 0xF8u) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        {
            if (i + 3 >= range.size()) { return { validation_error::unexpected_end, range.to_utf8(i), codepoint_count }; }

            const auto b2 = range[i + 1];
            const auto b3 = range[i + 2];
            const auto b4 = range[i + 3];
            if ((b2 ^ 0x80u | b3 ^ 0x80u | b4 ^ 0x80u) & 0xC0u) { return { validation_error::unexpected_non_continuation_byte, range.to_utf8(i), codepoint_count }; }

            i += 4;
            codepoint_count++;
        }
        else // 11111xxx
        {
            return { validation_error::invalid_byte, range.to_utf8(i), codepoint_count };
        }
    }

    return { validation_error::success, range.to_utf8(i), codepoint_count };
}

} // namespace losgodis::utf8
