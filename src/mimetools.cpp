#include "mimetools.h"
#include "hash_utils.h"
#include "time_utils.h"
#include "include/plugin.h"
#include "tinf.h"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace MimeTools {

// --- 辅助函数：UTF-16 <-> UTF-8 转换 ---

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// --- 辅助函数：EmEditor 选区交互 ---

std::wstring GetTargetText(HWND hwndView) {
    UINT_PTR nLen = Editor_GetSelTextW(hwndView, 0, NULL);
    if (nLen <= 1) {
        Editor_ExecCommand(hwndView, EEID_EDIT_SELECT_ALL);
        nLen = Editor_GetSelTextW(hwndView, 0, NULL);
        if (nLen <= 1) return L"";
    }
    std::vector<wchar_t> buffer(nLen);
    Editor_GetSelTextW(hwndView, nLen, buffer.data());
    return std::wstring(buffer.data());
}

void ReplaceTargetText(HWND hwndView, const std::wstring& newText) {
    Editor_InsertW(hwndView, newText.c_str());
}

// --- Base64 算法实现 ---

static const std::string B64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(B64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string base64_decode(const std::string& in) {
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[(unsigned char)B64_CHARS[i]] = i;
    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// --- URL 算法实现 ---

std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c; continue;
        }
        escaped << std::uppercase << '%' << std::setw(2) << (int)c;
    }
    return escaped.str();
}

std::string url_decode(const std::string& value) {
    std::string res;
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%') {
            if (i + 2 < value.length()) {
                int n;
                std::istringstream hex_stream(value.substr(i + 1, 2));
                if (hex_stream >> std::hex >> n) { res += (char)n; i += 2; }
                else res += '%';
            } else res += '%';
        } else if (value[i] == '+') res += ' ';
        else res += value[i];
    }
    return res;
}

// --- Quoted-Printable 算法实现 ---

std::string qp_encode(const std::string& in) {
    std::string out;
    int lineLen = 0;
    auto append = [&](const std::string& s) {
        if (lineLen + s.length() >= 75) {
            out += "=\r\n";
            lineLen = 0;
        }
        out += s;
        lineLen += (int)s.length();
    };

    for (unsigned char c : in) {
        if ((c >= 33 && c <= 60) || (c >= 62 && c <= 126)) {
            append(std::string(1, (char)c));
        } else if (c == '\r' || c == '\n' || c == '\t' || c == ' ') {
             append(std::string(1, (char)c));
             if (c == '\n') lineLen = 0;
        } else {
            char buf[4];
            sprintf(buf, "=%02X", (unsigned char)c);
            append(buf);
        }
    }
    return out;
}

std::string qp_decode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.length(); ++i) {
        if (in[i] == '=') {
            if (i + 2 < in.length()) {
                if (in[i+1] == '\r' && in[i+2] == '\n') {
                    i += 2;
                } else {
                    int val;
                    std::istringstream iss(in.substr(i + 1, 2));
                    if (iss >> std::hex >> val) {
                        out += (char)val;
                        i += 2;
                    } else out += '=';
                }
            } else out += '=';
        } else out += in[i];
    }
    return out;
}

// --- Hex 算法实现 ---

std::string hex_encode(const std::string& in) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (unsigned char c : in) oss << std::setw(2) << (int)c << " ";
    return oss.str();
}

std::string hex_decode(const std::string& in) {
    std::string clean;
    for (char c : in) if (!isspace((unsigned char)c)) clean += c;
    
    std::string res;
    for (size_t i = 0; i + 1 < clean.length(); i += 2) {
        try {
            res += (char)std::stoi(clean.substr(i, 2), nullptr, 16);
        } catch (...) {}
    }
    return res;
}

// --- SAML 算法实现 ---

std::string saml_decode(const std::string& in) {
    std::string stage1 = url_decode(in);
    std::string compressed = base64_decode(stage1);
    if (compressed.empty()) return in;

    static bool tinf_inited = false;
    if (!tinf_inited) { tinf_init(); tinf_inited = true; }

    unsigned int destLen = (unsigned int)compressed.size() * 10 + 4096;
    std::vector<unsigned char> dest(destLen);
    
    int res = tinf_uncompress(dest.data(), &destLen, (const unsigned char*)compressed.data());
    if (res == TINF_OK) {
        return std::string((char*)dest.data(), destLen);
    }
    return compressed;
}

// --- HTML 算法实现 ---

std::string html_encode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.length(); ++i) {
        unsigned char c = (unsigned char)in[i];
        if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '&') out += "&amp;";
        else if (c == '\"') out += "&quot;";
        else if (c == '\'') out += "&apos;";
        else if (c < 0x80) {
            out += (char)c;
        } else {
            // UTF-8 to Unicode
            uint32_t uni = 0;
            int len = 0;
            if ((c & 0xE0) == 0xC0) { uni = c & 0x1F; len = 1; }
            else if ((c & 0xF0) == 0xE0) { uni = c & 0x0F; len = 2; }
            else if ((c & 0xF8) == 0xF0) { uni = c & 0x07; len = 3; }
            
            bool valid = (len > 0 && i + len < in.length());
            if (valid) {
                for (int j = 0; j < len; ++j) {
                    unsigned char next = (unsigned char)in[++i];
                    if ((next & 0xC0) != 0x80) { valid = false; break; }
                    uni = (uni << 6) | (next & 0x3F);
                }
            }

            if (valid) {
                char buf[16];
                sprintf(buf, "&#x%X;", uni);
                out += buf;
            } else {
                out += (char)c; // fallback
            }
        }
    }
    return out;
}

std::string html_decode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.length(); ++i) {
        if (in[i] == '&') {
            size_t end = in.find(';', i);
            if (end != std::string::npos && end - i < 10) {
                std::string entity = in.substr(i + 1, end - i - 1);
                uint32_t uni = 0;
                bool found = false;

                if (entity == "lt") { out += '<'; found = true; }
                else if (entity == "gt") { out += '>'; found = true; }
                else if (entity == "amp") { out += '&'; found = true; }
                else if (entity == "quot") { out += '\"'; found = true; }
                else if (entity == "apos") { out += '\''; found = true; }
                else if (entity[0] == '#') {
                    try {
                        if (entity[1] == 'x' || entity[1] == 'X')
                            uni = std::stoul(entity.substr(2), nullptr, 16);
                        else
                            uni = std::stoul(entity.substr(1));
                        
                        // Unicode to UTF-8
                        if (uni > 0x10FFFF) {
                             // Out of range, do not decode
                        } else if (uni < 0x80) {
                            out += (char)uni;
                            found = true;
                        } else if (uni < 0x800) {
                            out += (char)(0xC0 | (uni >> 6));
                            out += (char)(0x80 | (uni & 0x3F));
                            found = true;
                        } else if (uni < 0x10000) {
                            out += (char)(0xE0 | (uni >> 12));
                            out += (char)(0x80 | ((uni >> 6) & 0x3F));
                            out += (char)(0x80 | (uni & 0x3F));
                            found = true;
                        } else {
                            out += (char)(0xF0 | (uni >> 18));
                            out += (char)(0x80 | ((uni >> 12) & 0x3F));
                            out += (char)(0x80 | ((uni >> 6) & 0x3F));
                            out += (char)(0x80 | (uni & 0x3F));
                            found = true;
                        }
                    } catch (...) {}
                }

                if (found) { i = end; continue; }
            }
        }
        out += in[i];
    }
    return out;
}

// --- 接口映射 (UI 桥接) ---

void EncodeHtml(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(html_encode(WideToUtf8(GetTargetText(hwndView)))));
}

void DecodeHtml(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(html_decode(WideToUtf8(GetTargetText(hwndView)))));
}

void EncodeBase64(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(base64_encode(WideToUtf8(GetTargetText(hwndView)))));
}

void DecodeBase64(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(base64_decode(WideToUtf8(GetTargetText(hwndView)))));
}

void EncodeUrl(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(url_encode(WideToUtf8(GetTargetText(hwndView)))));
}

void DecodeUrl(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(url_decode(WideToUtf8(GetTargetText(hwndView)))));
}

void DecodeSaml(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(saml_decode(WideToUtf8(GetTargetText(hwndView)))));
}

void EncodeHex(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(hex_encode(WideToUtf8(GetTargetText(hwndView)))));
}

void DecodeHex(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(hex_decode(WideToUtf8(GetTargetText(hwndView)))));
}

void EncodeQP(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(qp_encode(WideToUtf8(GetTargetText(hwndView)))));
}

void DecodeQP(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(qp_decode(WideToUtf8(GetTargetText(hwndView)))));
}

std::string md5_hash(const std::string& in) { return HashTools::md5(in); }
std::string sha1_hash(const std::string& in) { return HashTools::sha1(in); }
std::string sha256_hash(const std::string& in) { return HashTools::sha256(in); }
std::string sha512_hash(const std::string& in) { return HashTools::sha512(in); }

void HashMD5(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(HashTools::md5(WideToUtf8(GetTargetText(hwndView)))));
}

void HashSHA1(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(HashTools::sha1(WideToUtf8(GetTargetText(hwndView)))));
}

void HashSHA256(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(HashTools::sha256(WideToUtf8(GetTargetText(hwndView)))));
}

void HashSHA512(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(HashTools::sha512(WideToUtf8(GetTargetText(hwndView)))));
}

void ConvertTimestamp(HWND hwndView) {
    ReplaceTargetText(hwndView, Utf8ToWide(TimeTools::SmartConvert(WideToUtf8(GetTargetText(hwndView)))));
}

} // namespace MimeTools
