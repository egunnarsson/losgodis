
#include "losgodis/string_pool.hpp"

namespace losgodis
{

string_pool::string_pool(std::initializer_list<string_literal> list) :
    map_{ list.begin(), list.end() }
{
}

// special one that does not copy
fixed_string string_pool::get_string(string_literal string)
{
    const auto key = string.key_;
    const auto size = key.view_.size();
    const auto it = map_.find(key);
    if (it != map_.end())
    {
        return fixed_string{ it->view_.data(), size };
    }

    map_.insert(string_key{ key.view_ , key.hash_ });
    return fixed_string{ key.view_.data(), size };
}

fixed_string string_pool::get_string(string_key key)
{
    const auto size = key.view_.size();
    const auto it = map_.find(key);
    if (it != map_.end())
    {
        return fixed_string{ it->view_.data(), size };
    }

    if (page_ == nullptr || !page_->can_hold(size))
    {
        page_ = std::make_unique<detail::fixed_page>(std::move(page_));
    }

    const auto str = page_->push_back(key);
    map_.insert(string_key{ std::string_view{ str, size }, key.hash_ });
    return fixed_string{ str, size };
}

bool detail::fixed_page::can_hold(size_t size)
{
    return (size + 1) < remaining_;
}

const char* detail::fixed_page::push_back(string_key key)
{
    const auto size = key.view().size();
    const auto start = &buffer_.back() - remaining_ + 1;
    key.view().copy(start, size);
    start[size] = '\0';
    remaining_ -= size + 1;
    return start;
}

} // namespace losgodis
