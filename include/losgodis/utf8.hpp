#pragma once

/*

    iterator

Iterator for a utf8 range. Iterates over the unicode codepoints of the range.
Exhibits undefined behaviour if the underlying data is not a valid utf8 string,
specifically there is no guarantee that the iterator will reach the end.


    utf8_range

Represents a utf8 string. Can be constructed either by calling one of the 
validate functions, or manually in which case it is up to the user to ensure
that the underlying bytes represents a valid utf8 string.


    byte_range

Represents a range of bytes, it is the input to the validate functions.


    validate / validate_quick

Validates a range of bytes and returns validation_result, which indicates
any potential errors, contains a utf8 range up to the first invalid byte, and 
the number of codepoints in the utf8 range. validate_quick will not check for
invalid unicode codepoints and overlong encodings.

*/

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace losgodis::utf8
{

// char32_t is no fun, what can you do with it? :(
using codepoint_t = uint32_t;

class iterator
{

public:

    iterator(const char* str) : str_{ str } {}

    codepoint_t operator*() const
    {
        const auto byte0 = byte(0);
        if (byte0 < 0x80u) // 0xxxxxxx
        {
            return byte0;
        }
        else if (byte0 < 0xE0u) // 110xxxxx 10xxxxxx
        {
            return ((byte0 & 0x1Fu) << 6) | (byte(1) & 0x3Fu);
        }
        else if (byte0 < 0xF0u) // 1110xxxx 10xxxxxx 10xxxxxx
        {
            return ((byte0 & 0xFu) << 12) | ((byte(1) & 0x3Fu) << 6) | (byte(2) & 0x3Fu);
        }
        else // if (b1 < 0xF8u) // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        {
            return ((byte0 & 0x7u) << 18) | ((byte(1) & 0x3Fu) << 12) | ((byte(2) & 0x3Fu) << 6) | (byte(3) & 0x3Fu);
        }
    }

    iterator& operator++()
    {
        const auto byte0 = byte(0);
        if (byte0 <= 0x7Fu)
        {
            str_ += 1;
        }
        else if (byte0 <= 0xBFu)
        {
            assert(false);
            str_ += 1;
        }
        else if (byte0 <= 0xDFu)
        {
            str_ += 2;
        }
        else if (byte0 <= 0xEFu)
        {
            str_ += 3;
        }
        else if (byte0 <= 0xF7u)
        {
            str_ += 4;
        }
        else
        {
            assert(false);
            str_ += 1;
        }
        return *this;
    }

    bool operator==(const iterator& RHS) const { return str_ == RHS.str_; }
    bool operator!=(const iterator& RHS) const { return str_ != RHS.str_; }

private:

    uint8_t byte(size_t offset) const { return static_cast<uint8_t>(*(str_ + offset)); }

    const char* str_;
};

class utf8_range
{

public:

    utf8_range(const char* start, size_t byte_count) :start_{ start }, byte_count_{ byte_count }{}

    iterator begin() const { return iterator{ start_ }; }
    iterator end() const { return iterator{ start_ + byte_count_ }; }

private:

    const char* start_;
    size_t      byte_count_;
};

class byte_range
{

public:

    byte_range(const char* start, size_t byte_count) :start_{ start }, byte_count_{ byte_count }{}

    size_t      size() const { return byte_count_; }
    const char* begin() const { return start_; }

    uint8_t operator[](size_t index) const { return static_cast<uint8_t>(*(start_ + index)); }

    utf8_range to_utf8(size_t size) const { return utf8_range{ start_, size }; }

private:

    const char* start_;
    size_t      byte_count_;
};

enum class validation_error
{
    success = 0,
    invalid_byte, // an invalid byte
    invalid_codepoint, // an invalid codepoint
    overlong_enocoding, // codepoint using more bytes than needed
    unexpected_continuation_byte, // continuation at start of new codepoint
    unexpected_non_continuation_byte, // codepoint unexpected end
    unexpected_end // end in the middle of a codepoint
};

// On error range.end() is the first problematic byte
struct validation_result
{
    validation_error error;
    utf8_range       range;
    size_t           codepoint_count; // maybe put in range, but then allow it to be unknown, for user ranges
};

validation_result validate(byte_range range);

// do not check invalid_codepoint and overlong_enocoding
validation_result validate_quick(byte_range range);

} // namespace losgodis::utf8
