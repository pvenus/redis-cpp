//-------------------------------------------------------------------
//  redis-cpp
//  https://github.com/tdv/redis-cpp
//  Created:     03.2020
//  Copyright (C) 2020 tdv
//-------------------------------------------------------------------

#ifndef __REDISCPP_CORE_PROTOCOL_RESP_IN_H__
#define __REDISCPP_CORE_PROTOCOL_RESP_IN_H__

// STD
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// REDIS-CPP

#include "redis-cpp/core/protocol/resp/detail/marker.h"

namespace rediscpp::core::protocol
{
inline namespace resp
{
namespace in
{

auto get_mark(std::istream &stream)
{
    switch (stream.get())
    {
    case detail::marker::simple_string :
        return detail::marker::simple_string;
    case detail::marker::error_message :
        return detail::marker::error_message;
    case detail::marker::integer :
        return detail::marker::integer;
    case detail::marker::bulk_string :
        return detail::marker::bulk_string;
    case detail::marker::array :
        return detail::marker::array;
    default:
        break;
    }
    throw std::invalid_argument{
            "[rediscpp::core::protocol::resp::in::get_mark] "
            "Bad input format."
        };
}

template <typename T>
T get(std::istream &stream)
{
    return {stream};
}

class simple_string final
{
public:
    simple_string(std::istream &stream)
    {
        std::getline(stream, value_);
    }

    std::string_view get() const noexcept
    {
        return value_;
    }

private:
    std::string value_;
};

class error_message final
{
public:
    error_message(std::istream &stream)
    {
        std::getline(stream, value_);
    }

    std::string_view get() const noexcept
    {
        return value_;
    }

private:
    std::string value_;
};

class integer final
{
public:
    integer(std::istream &stream)
    {
        std::string string;
        std::getline(stream, string);
        value_ = std::stoll(string);
    }

    std::int64_t get() const noexcept
    {
        return value_;
    }

private:
    std::int64_t value_;
};

class binary_data final
{
public:
    binary_data(std::istream &stream)
    {
        std::string string;
        std::getline(stream, string);
        auto const length = std::stoll(string);
        if (length < 0)
            is_null_ = true;
        if (length < 1)
            return;
        data_.resize(static_cast<typename buffer_type::size_type>(length));
        stream.read(&data_[0], length);
        std::getline(stream, string);
    }

    bool is_null() const noexcept
    {
        return is_null_;
    }

    std::string_view get() const noexcept
    {
        return {std::data(data_), std::size(data_)};
    }

    std::size_t size() const noexcept
    {
        return std::size(data_);
    }

    char const* data() const noexcept
    {
        return std::data(data_);
    }

private:
    using buffer_type = std::vector<char>;
    bool is_null_ = false;
    buffer_type data_;
};

class bulk_string final
{
public:
    bulk_string(std::istream &stream)
        : data_{stream}
    {
    }

    bool is_null() const noexcept
    {
        return data_.is_null();
    }

    std::string_view get() const noexcept
    {
        return data_.get();
    }

private:
    binary_data data_;
};

class null final
{
public:
    void get() const noexcept
    {
    }
};

class array final
{
public:
    using item_type = std::variant<
            simple_string,
            error_message,
            integer,
            bulk_string,
            array,
            null
        >;

    using items_type = std::vector<item_type>;

    array(std::istream &stream)
    {
        std::string string;
        std::getline(stream, string);
        auto count = std::stoll(string);
        if (count < 0)
            is_null_ = true;
        if (count < 1)
            return;
        items_.reserve(static_cast<typename items_type::size_type>(count));
        while (count--)
        {
            switch (get_mark(stream))
            {
            case detail::marker::simple_string :
                items_.emplace_back(simple_string{stream});
                break;
            case detail::marker::error_message :
                items_.emplace_back(error_message{stream});
                break;
            case detail::marker::integer :
                items_.emplace_back(integer{stream});
                break;
            case detail::marker::bulk_string :
                items_.emplace_back(bulk_string{stream});
                break;
            case detail::marker::array :
                items_.emplace_back(array{stream});
                break;
            default:
                throw std::invalid_argument{
                        "[rediscpp::core::protocol::resp::in::array] "
                        "Bad input format. Unsupported value type."
                    };
            }
        }
    }

    bool is_null() const noexcept
    {
        return is_null_;
    }

    std::size_t size() const noexcept
    {
        return std::size(items_);
    }

    items_type const& get() const noexcept
    {
        return items_;
    }

private:
    bool is_null_ = false;
    items_type items_;
};

}   // namespace serialization
}   // namespace resp
}   // namespace rediscpp::core::protocol

#endif  // !__REDISCPP_CORE_PROTOCOL_RESP_IN_H__