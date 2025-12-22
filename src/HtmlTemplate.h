#pragma once
#include <fstream>
#include <sstream>
#include <string>

inline std::string loadHtmlTemplate(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline void replaceAll(
    std::string& str,
    const std::string& from,
    const std::string& to
) {
    size_t start = 0;
    while ((start = str.find(from, start)) != std::string::npos) {
        str.replace(start, from.length(), to);
        start += to.length();
    }
}
