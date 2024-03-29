#ifndef WHALEROUTE_STRINGCONVERTER_H
#define WHALEROUTE_STRINGCONVERTER_H

#include <optional>
#include <sstream>
#include <string>

namespace whaleroute::config {

template<typename T>
struct StringConverter {
    static std::optional<T> fromString(const std::string& data)
    {
        if constexpr (std::is_convertible_v<T, std::string>) {
            return data;
        }
        else {
            auto value = T{};
            auto stream = std::stringstream{data};
            stream >> value;

            if (stream.bad() || stream.fail() || !stream.eof())
                return {};
            return value;
        }
    }
};

} // namespace whaleroute::config

#endif // WHALEROUTE_STRINGCONVERTER_H