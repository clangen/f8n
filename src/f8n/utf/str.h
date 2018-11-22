#pragma once

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

namespace f8n { namespace str {

    template<typename... Args>
    static std::string format(const std::string& format, Args ... args) {
        /* https://stackoverflow.com/a/26221725 */
        size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; /* extra space for '\0' */
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); /* omit the '\0' */
    }

    static std::string lower(const std::string& in) {
        std::string out = in;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower) ;
        return out;
    }

    static inline std::string trim(const std::string& str) {
        std::string s(str);

        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));

        s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());

        return s;
    }

    static std::vector<std::string> split(
        const std::string& in, const std::string& delim)
    {
        std::vector<std::string> result;
        size_t last = 0, next = 0;
        while ((next = in.find(delim, last)) != std::string::npos) {
            result.push_back(std::move(trim(in.substr(last, next - last))));
            last = next + 1;
        }
        result.push_back(std::move(trim(in.substr(last))));
        return result;
    }

} }