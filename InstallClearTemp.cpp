#include <windows.h>
#include <shlobj.h>
#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>
#include <limits>

std::string askYesNo(const std::string& question) {
    while (true) {
        std::cout << question << "Y(Да),N(Нет):";
        char c;
        std::cin >> c;
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        if (c == 'Y' || c == 'N') {
            return std::string(1, c);
        }
        std::cout << "Введите Y или N." << std::endl;
    }
}

bool createDirectoryRecursive(const std::string& path) {
    // Простейшая рекурсивная функция создания директорий
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        current.push_back(path[i]);
        if ((path[i] == '\\' || path[i] == '/') && current.size() > 1) {
            _mkdir(current.c_str());
        }
    }
    if (_mkdir(path.c_str()) == 0 || GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }
    return false;
}

bool setAutorun(const std::wstring& valueName, const std::wstring& exePath) {
    HKEY hKey;
    LPCWSTR subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    LONG res = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        subKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hKey,
        NULL
    );
    if (res != ERROR_SUCCESS) {
        return false;
    }

    res = RegSetValueExW(
        hKey,
        valueName.c_str(),
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(exePath.c_str()),
        static_cast<DWORD>((exePath.size() + 1) * sizeof(wchar_t))
    );
    RegCloseKey(hKey);
    return res == ERROR_SUCCESS;
}

std::wstring getInstallerDirectory() {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return L".";
    }
    std::wstring full(path);
    size_t pos = full.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L".";
    }
    return full.substr(0, pos);
}

bool copyFileSimple(const std::wstring& src, const std::wstring& dst) {
    if (!CopyFileW(src.c_str(), dst.c_str(), FALSE)) {
        return false;
    }
    return true;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    const std::string installPath = "C:\\Program Files\\ClearTemp";
    const std::string logsDir    = installPath + "\\logs";
    const std::string logsFile   = logsDir + "\\logs.txt";

    std::cout << "Я установщик ClearTemp вы даёте разрешение на установку в путь "
              << installPath << "?" << std::endl;
    std::cout.flush();

    std::string ansInstall = askYesNo("");
    if (ansInstall == "N") {
        return 0; // cmd закрывается
    }

    std::cout << std::endl;
    std::cout << "Разрешить ClearTemp запускаться из автозагрузки?" << std::endl;
    std::cout.flush();
    std::string ansAuto = askYesNo("");

    bool autorunSet = false;
    if (ansAuto == "Y") {
        std::wstring valueName = L"ClearTemp";
        std::wstring exePath   = L"C:\\Program Files\\ClearTemp\\Temp.exe";

        autorunSet = setAutorun(valueName, exePath);
    } else {
        std::cout << "WARNING: вам нужно будет запускать ClearTemp вручную по exe" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Установка...." << std::endl;

    std::cout << "Создание автозагрузки по пути: "
              << "Компьютер\\HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
              << std::endl;

    if (ansAuto == "Y") {
        if (autorunSet) {
            std::cout << "Автозагрузка создана!:" << std::endl;
            std::cout << "|Название: ClearTemp" << std::endl;
            std::cout << "|- Значение: C:\\Program Files\\ClearTemp\\Temp.exe" << std::endl;
            std::cout << "|-- Тип: Строковый параметр" << std::endl;
        } else {
            std::cout << "Не удалось создать запись автозагрузки." << std::endl;
        }
    } else {
        std::cout << "Автозагрузка не была создана по выбору пользователя." << std::endl;
    }

    std::cout << "Начинаю загружать основные файлы по пути: "
              << installPath << "..." << std::endl;

    createDirectoryRecursive(installPath);

    std::wstring installerDirW = getInstallerDirectory();
    std::wstring installDirW   = L"C:\\Program Files\\ClearTemp";

    // Копируем Temp.exe, InfoStatic.bat, Unistall.bat из папки установщика
    bool okTemp = copyFileSimple(installerDirW + L"\\Temp.exe",
                                 installDirW   + L"\\Temp.exe");
    bool okInfo = copyFileSimple(installerDirW + L"\\InfoStatic.bat",
                                 installDirW   + L"\\InfoStatic.bat");
    bool okUn   = copyFileSimple(installerDirW + L"\\Unistall.bat",
                                 installDirW   + L"\\Unistall.bat");

    if (okTemp) std::cout << "Успешная загрузка: Temp.exe" << std::endl;
    else        std::cout << "Ошибка при копировании Temp.exe" << std::endl;

    if (okInfo) std::cout << "Успешная загрузка: InfoStatic.bat" << std::endl;
    else        std::cout << "Ошибка при копировании InfoStatic.bat" << std::endl;

    if (okUn)   std::cout << "Успешная загрузка: Unistall.bat" << std::endl;
    else        std::cout << "Ошибка при копировании Unistall.bat" << std::endl;

    createDirectoryRecursive(logsDir);

    // Папка logs и файл logs.txt
    {
        std::cout << "Успешная загрузка папки: logs" << std::endl;
        std::ofstream f(logsFile);
        if (f) {
            f << "ClearTemp installation log\n";
            f << "Install path: " << installPath << "\n";
            f << "Autorun: " << (ansAuto == "Y" && autorunSet ? "enabled" : "disabled") << "\n";
            std::cout << "Успешное создание в папке logs: logs.txt" << std::endl;
        } else {
            std::cout << "Ошибка при создании logs.txt" << std::endl;
        }
    }

    std::cout << "Загрузка завершена" << std::endl;
    std::cout << "Можете проверить загрузку по пути: "
              << "C:\\Program Files\\ClearTemp\\logs\\logs.txt" << std::endl;
    std::cout << std::endl;
    std::cout << "Нажмите любую кнопку для выхода из cmd..." << std::endl;

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}