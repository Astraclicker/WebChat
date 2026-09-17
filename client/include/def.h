#pragma once
#include <string>

enum class msgType {
    text,
    loginRequested,
    createUserRequested
};

struct msg {
    std::string data;
    msgType msgType;
};

inline std::string gbkToUtf8(const std::string &gbk, const UINT codePage = CP_ACP) {
    if (gbk.empty()) {
        return {};
    }
    //GKB -> UTF-16
    const int wlen = ::MultiByteToWideChar(
        codePage, 0, gbk.data(), static_cast<int>(gbk.size()), nullptr, 0);
    if (wlen <= 0) {
        return {};
    }
    std::wstring w(static_cast<std::size_t>(wlen), L'\0');
    ::MultiByteToWideChar(
        codePage, 0, gbk.data(), static_cast<int>(gbk.size()), w.data(), wlen);

    //UTF-16 -> UTF-8
    const int u8len = ::WideCharToMultiByte(
        CP_UTF8, 0, w.data(), wlen, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) {
        return {};
    }
    std::string u8(static_cast<std::size_t>(u8len), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, w.data(), wlen, u8.data(), u8len, nullptr, nullptr);
    return u8;
}
