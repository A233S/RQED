#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string base64_encode(const std::vector<unsigned char>& data) {
    std::string encoded;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (const auto& byte : data) {
        char_array_3[i++] = byte;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                encoded += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = 0;

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++)
            encoded += base64_chars[char_array_4[j]];

        while (i++ < 2)
            encoded += '=';
    }

    return encoded;
}

std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    std::vector<unsigned char> ret;
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];

    while (in_ < in_len) {
        char c = encoded_string[in_];
        if (c == '=') break;
        size_t pos = base64_chars.find(c);
        if (pos == std::string::npos) {
            ++in_;
            continue;
        }
        char_array_4[i++] = c;
        ++in_;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }

    return ret;
}

std::vector<unsigned char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("无法打开文件: " + filename);

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw std::runtime_error("读取文件失败: " + filename);

    return buffer;
}

std::string read_file_as_string(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("无法打开文件: " + filename);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void apply_xor(std::vector<unsigned char>& data, const std::string& ps) {
    if (ps.empty()) return;
    size_t ps_len = ps.size();
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= static_cast<unsigned char>(ps[i % ps_len]);
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    std::string filepath, ps;
    bool encode = false, decode = false;

    // 解析参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-file" && i + 1 < argc) {
            filepath = argv[++i];
        }
        else if (arg == "-en") {
            encode = true;
        }
        else if (arg == "-de") {
            decode = true;
        }
        else if (arg == "-ps" && i + 1 < argc) {
            ps = argv[++i];
        }
        else {
            std::cerr << "参数错误: " << arg << std::endl;
            return 1;
        }
    }

    try {
        if (filepath.empty()) throw std::runtime_error("必须指定文件路径!!!   用`-file xxx`输入文件,`-en`代表编码文件,`-de`代码解码文件,`-ps xxx`代表字符串偏移量（相当于编码和解码密码）");
        if (encode == decode) throw std::runtime_error("必须选择-en或-de");

        if (encode) {
            auto data = read_file(filepath);
            if (!ps.empty()) apply_xor(data, ps);
            std::cout << base64_encode(data);
        }
        else {
            std::string encoded = read_file_as_string(filepath);
            auto data = base64_decode(encoded);
            if (!ps.empty()) apply_xor(data, ps);
            std::cout.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}