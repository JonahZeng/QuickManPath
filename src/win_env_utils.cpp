#include "win_env_utils.hpp"
#include <windows.h>
#include <sstream>

// 获取Windows系统标题栏高度
int getTitleBarHeight()
{
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
    {
        return ncm.iCaptionHeight;
    }
    return 30; // 默认值，如果获取失败
}

static std::vector<EnvPathItem_t> getEnvironmentVariable(const std::string &varName, HKEY hive)
{
    std::vector<EnvPathItem_t> result;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize = 0;
    char *buffer = nullptr;

    // Determine the correct registry path based on the hive
    const char *subKey;
    if (hive == HKEY_LOCAL_MACHINE)
    {
        subKey = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    }
    else if (hive == HKEY_CURRENT_USER)
    {
        subKey = "Environment";
    }
    else
    {
        return result; // Unsupported hive
    }

    if (RegOpenKeyExA(hive, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, varName.c_str(), NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
        {
            if (dwType == REG_EXPAND_SZ || dwType == REG_SZ)
            {
                buffer = new char[dwSize];
                if (RegQueryValueExA(hKey, varName.c_str(), NULL, &dwType, (LPBYTE)buffer, &dwSize) == ERROR_SUCCESS)
                {
                    std::string pathStr(buffer);
                    std::istringstream iss(pathStr);
                    std::string path;
                    while (getline(iss, path, ';'))
                    {
                        if (!path.empty())
                        {
                            result.push_back(EnvPathItem_t{path, true});
                        }
                    }
                }
                delete[] buffer;
            }
        }
        RegCloseKey(hKey);
    }
    return result;
}

std::vector<EnvPathItem_t> getSystemPath()
{
    return getEnvironmentVariable("Path", HKEY_LOCAL_MACHINE);
}

std::vector<EnvPathItem_t> getUserPath()
{
    return getEnvironmentVariable("Path", HKEY_CURRENT_USER);
}

bool setSystemPath(const std::vector<EnvPathItem_t>& systemPaths)
{
    std::string systemPathStr;
    for (const auto &item : systemPaths)
    {
        if (item.enabled)
        {
            if (!systemPathStr.empty())
            {
                systemPathStr += ";";
            }
            systemPathStr += item.path;
        }
    }

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                      0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                           (const BYTE *)systemPathStr.c_str(), (DWORD)(systemPathStr.length() + 1)) == ERROR_SUCCESS)
        {
            // 通知系统环境变量已更改
            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                               (LPARAM) "Environment", SMTO_BLOCK, 5000, NULL);
        }
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

bool setUserPath(const std::vector<EnvPathItem_t>& userPaths)
{
    std::string userPathStr;
    for (const auto &item : userPaths)
    {
        if (item.enabled)
        {
            if (!userPathStr.empty())
            {
                userPathStr += ";";
            }
            userPathStr += item.path;
        }
    }

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment",
                      0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
        if (RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                           (const BYTE *)userPathStr.c_str(), (DWORD)(userPathStr.length() + 1)) == ERROR_SUCCESS)
        {
            // 通知系统环境变量已更改
            SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                               (LPARAM) "Environment", SMTO_BLOCK, 5000, NULL);
        }
        RegCloseKey(hKey);
        return true;
    }
    return false;
}
