#include <windows.h>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

std::wstring GetLogPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    fs::path exe = path;
    fs::path logDir = exe.parent_path() / L"logs";

    fs::create_directories(logDir);

    return (logDir / L"TempCleaner.log").wstring();
}

void Log(const std::wstring& text)
{
    std::wofstream file(GetLogPath().c_str(), std::ios::app);

    auto now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());

    file << L"[" << now << L"] " << text << L"\n";
}

std::wstring GetTempFolder()
{
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    return temp;
}

unsigned long long CleanTemp(const fs::path& temp)
{
    unsigned long long total = 0;

    std::error_code ec;

    for (auto& p : fs::recursive_directory_iterator(
             temp,
             fs::directory_options::skip_permission_denied,
             ec))
    {
        if (p.is_regular_file(ec))
        {
            auto size = p.file_size(ec);
            if (!ec)
                total += size;
        }
    }

    for (auto& entry : fs::directory_iterator(
             temp,
             fs::directory_options::skip_permission_denied,
             ec))
    {
        fs::remove_all(entry.path(), ec);
    }

    return total;
}

void ShowNotification(const std::wstring& gbStr)
{
    std::wstring cmd =
        L"powershell -NoProfile -ExecutionPolicy Bypass -Command \""
        L"[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] > $null;"
        L"$template = [Windows.UI.Notifications.ToastTemplateType]::ToastText02;"
        L"$xml = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent($template);"
        L"$texts = $xml.GetElementsByTagName('text');"
        L"$texts.Item(0).AppendChild($xml.CreateTextNode('ClearTemp')) > $null;"
        L"$texts.Item(1).AppendChild($xml.CreateTextNode('Deleted " + gbStr + L" GB')) > $null;"
        L"$toast = [Windows.UI.Notifications.ToastNotification]::new($xml);"
        L"$notifier = [Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('ClearTemp');"
        L"$notifier.Show($toast);\"";

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);

    CreateProcessW(
        NULL,
        cmd.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main()
{
    std::wstring tempPath = GetTempFolder();

    Log(L"TEMP path: " + tempPath);

    fs::path temp(tempPath);

    if (!fs::exists(temp))
    {
        Log(L"TEMP folder not found");
        return 0;
    }

    unsigned long long bytes = CleanTemp(temp);

    double gb = bytes / (1024.0 * 1024.0 * 1024.0);

    std::wstringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(2);
    ss << gb;

    std::wstring gbStr = ss.str();

    Log(L"Deleted GB: " + gbStr);

    ShowNotification(gbStr);

    return 0;
}
