#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <sstream>
#include <iostream>

namespace fs = std::filesystem;

static unsigned long long collectSizeAndCleanTemp(fs::path tempPath) {
    unsigned long long totalBytes = 0;

    std::error_code ec;

    for (auto it = fs::recursive_directory_iterator(
             tempPath,
             fs::directory_options::skip_permission_denied,
             ec);
         it != fs::recursive_directory_iterator();
         ++it) {

        if (ec) {
            ec.clear();
            continue;
        }

        if (it->is_regular_file(ec)) {
            auto sz = it->file_size(ec);
            if (!ec) {
                totalBytes += static_cast<unsigned long long>(sz);
            } else {
                ec.clear();
            }
        }
    }

    fs::remove_all(tempPath, ec);
    ec.clear();
    fs::create_directories(tempPath, ec);

    return totalBytes;
}

int main() {
    wchar_t tempBuf[MAX_PATH];
    DWORD len = GetTempPathW(MAX_PATH, tempBuf);
    if (len == 0 || len > MAX_PATH) {
        return 0;
    }

    fs::path tempPath = fs::path(tempBuf);

    unsigned long long deletedBytes = collectSizeAndCleanTemp(tempPath);

    SHEmptyRecycleBinW(
        nullptr,
        nullptr,
        SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND
    );

    double gb = deletedBytes / (1024.0 * 1024.0 * 1024.0);

    std::wstringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(2);
    ss << gb;
    std::wstring gbStr = ss.str();

    // Показываем уведомление прямо из exe — всегда работает
    std::wstring msg = L"Было удалено " + gbStr + L" ГБ! Спасибо за то, что пользуетесь нами!";
    MessageBoxW(nullptr, msg.c_str(), L"Temp.exe сработал!", MB_OK | MB_ICONINFORMATION);

    // Дополнительно запускаем батник (если нужен для логов и т.д.)
    std::wstring batPath = L"C:\\Program Files\\ClearTemp\\InfoStatic.bat";
    std::wstring cmdParams = L"/c \"" + batPath + L"\" \"" + gbStr + L"\"";

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = cmdParams.c_str();
    sei.nShow = SW_HIDE;
    ShellExecuteExW(&sei);
    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 3000);
        CloseHandle(sei.hProcess);
    }

    return 0;
}

