#pragma once

/*

    pool

A pool of strings. Can be used to avoid duplicates of often used strings,
and the pooled strings can be compared by their pointer value. Call 
get_string to get the pooled version of a string. The difference beetween 
pool and std::unordered_map<std::string> is the memory layout, the string
data is stored in larger chunks or pages. There is an overload of 
get_string that accepts string literals, if used when the string is not 
pooled yet the memory of the literal will be used by the fixed_string, 
instead copying the data to the chunk/page.


    fixed_string

A pooled string. Will be fixed in memory, hence the name. fixed_strings 
from the same pool can be checked for equality by comparing the data 
pointer. It has conversion operators for common string types.


    string_key

Used to add string and do lookup in the pool.


    string_literal

Special string_key that will not be copied when added to the pool. Can be 
constructed with the string literal operator _key.

*/

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

namespace losgodis
{

// Change if needed, needs to be atleast large enough to hold biggest string
constexpr size_t page_size = 4096;

class string_pool;
class string_key;
class string_literal;

namespace detail
{

// Simple constexpr hash, todo: add a way to override
// http://isthe.com/chongo/tech/comp/fnv/
constexpr size_t hash(const char* data, size_t size)
{
    constexpr size_t prime = sizeof(size_t) == 8 ? 1099511628211 : 16777619;
    constexpr size_t offset = sizeof(size_t) == 8 ? 14695981039346656037 : 2166136261;

    size_t hash = offset;
    for (size_t i = 0; i < size; ++i)
    {
        auto b = static_cast<size_t>(static_cast<uint8_t>(*(data + i)));
        hash ^= b;
        hash *= prime;
    }
    return hash;
}

class fixed_page
{

public:

    fixed_page(std::unique_ptr<fixed_page> previousPage) :previousPage_{ std::move(previousPage) }, remaining_{ page_size } {}

    bool        can_hold(size_t size);
    const char* push_back(string_key key);

private:

    std::unique_ptr<fixed_page> previousPage_; // depending on how many pages we make this will make a deep recursion when deleting?
    /* ~fixed_page() {
        while(previousPage_) { previousPage_ = std::move(previousPage_->previousPage_); }
    } */
    std::array<char, page_size> buffer_;
    size_t                      remaining_;
};

} // namespace detail

class fixed_string
{

public:

    fixed_string(const fixed_string&) noexcept = default;
    fixed_string& operator=(const fixed_string& other) noexcept = default;

    const char* data() const { return data_; }
    size_t           size() const { return size_; }

    const char* c_str() const { return data_; }
    std::string      str() const { return std::string{ data_, size_ }; }
    std::string_view view() const { return std::string_view{ data_, size_ }; }

    explicit operator const char* () const { return c_str(); }
    explicit operator std::string() const { return str(); }
    explicit operator std::string_view() const { return view(); }

private:

    friend string_pool;

    // only allow string_pool to create them
    fixed_string(const char* s, size_t size) noexcept :data_{ s }, size_{ size } {}

    const char* data_;
    size_t      size_;
};

inline bool operator==(fixed_string a, fixed_string b)
{
    return a.data() == b.data();
}

inline bool operator!=(fixed_string a, fixed_string b)
{
    return a.data() != b.data();
}

class string_key
{

public:

    constexpr explicit string_key(std::string_view string) noexcept :
        view_{ string },
        hash_{ detail::hash(string.data(), string.size()) }
    {
    }

    constexpr std::string_view view() const { return view_; }
    constexpr size_t           hash() const { return hash_; }

private:

    friend string_pool;
    friend constexpr string_literal operator"" _key(const char*, std::size_t);

    constexpr string_key(std::string_view view, size_t hash) noexcept :view_{ view }, hash_{ hash } {}

    std::string_view view_;
    size_t           hash_;
};

// needed for internal purposes
constexpr bool operator==(string_key a, string_key b)
{
    return a.view() == b.view();
}

} // namespace losgodis

namespace std
{

// needed for internal purposes
template <>
class hash<losgodis::string_key>
{

public:

    size_t operator()(const losgodis::string_key& key) const { return key.hash(); }
};

template <>
class hash<losgodis::fixed_string>
{

public:

    // maybe store hash to avoid hashing the pointer
    size_t operator()(const losgodis::fixed_string& str) const { return std::hash<const char*>{}(str.data()); }
};

} // namespace std

namespace losgodis
{

class string_literal
{

public:

    const string_key key_;

    // used to insert into string_pool map
    operator string_key() const { return key_; }

private:

    friend constexpr string_literal operator"" _key(const char*, std::size_t);

    constexpr explicit string_literal(string_key key) :key_{ key } {}
};

constexpr string_literal operator"" _key(const char* str, std::size_t size)
{
    return string_literal{ string_key { std::string_view{ str, size }, detail::hash(str, size) } };
}

class string_pool
{

public:

    string_pool() = default;
    // put a bunch of literals in pool without copying string data
    string_pool(std::initializer_list<string_literal> list);

    fixed_string get_string(string_key string);
    fixed_string get_string(string_literal string); // special one that does not copy
    fixed_string get_string(std::string_view string) { return get_string(string_key{ string }); }
    // maybe dont have this one? you have to make a string_view?
    // fixed_string getString(const char* string) { return getString(string_key{ string }); }
    fixed_string get_string(const std::string& string) { return get_string(string_key{ string }); }

private:

    /*
    struct entry
    {
        std::string_view view;
        hash_t           hash;
    };
    */

    using entry = string_key; // entry need to enforce null termination! but string_key does not
    std::unordered_set<entry> map_;
    //std::unordered_map<string_key, fixed_string> map_;

    std::unique_ptr<detail::fixed_page> page_;
};

#if 0

struct pooled_string
{
    // if we want relocateble block we want to use index instead of ptrs
    // but then you need to know of the pool to get the actual string

    //const int index;
    //const int counter;
    const size_t id_;
    void* const pool_;

    size_t index() const; // mask id
    size_t counter() const; // mask id

    size_t size() const;
    const char* c_str() const;
};

struct dynamic_pool
{
    struct entry
    {
        size_t                  counter_;
        size_t                  size_;
        std::unique_ptr<char[]> str_;
    };
    std::vector<entry> pool_;
    size_t             firstFree_;

    pooled_string get_string(string_key str);
    void remove_string(string_key str);
};

#endif

} // namespace losgodis
