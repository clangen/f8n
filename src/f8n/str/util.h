#pragma once

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <iomanip>

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

    static std::string join(
        const std::vector<std::string>& items, const std::string& separator)
    {
        std::string result;
        bool first = true;
        for (auto& s : items) {
            result += s;
            if (first) {
                result += separator;
                first = false;
            }
        }
        return result;
    }

    static inline void replace(
        std::string& input, const std::string& find, const std::string& replace)
    {
        size_t pos = input.find(find);
        while (pos != std::string::npos) {
            input.replace(pos, find.size(), replace);
            pos = input.find(find, pos + replace.size());
        }
    }

    static inline size_t copy(const std::string& src, char* dst, size_t size) {
        size_t len = src.size() + 1; /* space for the null terminator */
        if (dst) {
            size_t copied = src.copy(dst, size - 1);
            dst[copied] = '\0';
            return copied + 1;
        }
        return len;
    }

    static std::string make(const double value, const int precision = 2) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << value;
        return out.str();
    }

} }