#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Pack.H>
#include <FL/fl_ask.H>
#include <FL/fl_image.H>
#include <FL/x.H>
#include <windows.h>
#include <filesystem>
#include <map>
#include <fstream>
#include "path_tabel.hpp"
#include "win_env_utils.hpp"
#include "nlohmann/json.hpp"

constexpr int groupH = 350;
constexpr int tabelH = 300;
constexpr int labelH = 25;
constexpr int labelWholeH = 40;
constexpr int labelLeftMargin = 10;
constexpr int labelTopMargin = 10;
constexpr int tabelLeftMargin = 10;
constexpr int buttonWholeH = 40;
constexpr int buttonH = 25;
constexpr int buttonW = 90;
constexpr int fixedCellW = 60;

class MainWindow : public Fl_Window
{
public:
    PathTable *systemPathTable;
    PathTable *userPathTable;
private:
    Fl_Button *refreshButton;
    Fl_Button *applyButton;
    Fl_Button *newUserButton;
    Fl_Button *newSystemButton;
    Fl_Box *systemLabel;
    Fl_Box *userLabel;
    Fl_Pack *mainPack;
    Fl_Group *systemGroup;
    Fl_Group *userGroup;
    Fl_Group *buttonGroup;

    static void refreshCallback(Fl_Widget *w, void *data)
    {
        MainWindow *win = (MainWindow *)data;
        win->refreshPaths();
    }

    static void applyCallback(Fl_Widget *w, void *data)
    {
        MainWindow *win = (MainWindow *)data;
        win->applyPaths();
    }

    static void newUserCallback(Fl_Widget *w, void *data)
    {
        MainWindow *win = (MainWindow *)data;
        const char *newPath = fl_input("请输入新的用户环境变量路径:", "");
        if (newPath && strlen(newPath) > 0)
        {
            // 获取当前用户路径
            std::vector<EnvPathItem_t> userPaths;
            win->userPathTable->getPaths(userPaths);

            // 检查路径是否已存在
            for(auto &item : userPaths)
            {
                if (item.path == newPath)
                {
                    fl_alert("该路径已存在于用户环境变量中！");
                    return;
                }
            }

            userPaths.push_back(EnvPathItem_t{newPath, true}); // 默认启用
            win->userPathTable->setPaths(userPaths);

        }
    }

    static void newSystemCallback(Fl_Widget *w, void *data)
    {
        MainWindow *win = (MainWindow *)data;
        const char *newPath = fl_input("请输入新的系统环境变量路径:", "");
        if (newPath && strlen(newPath) > 0)
        {
            // 获取当前系统路径
            std::vector<EnvPathItem_t> systemPaths;
            win->systemPathTable->getPaths(systemPaths);

            // 检查路径是否已存在
            for(auto &item : systemPaths)
            {
                if (item.path == newPath)
                {
                    fl_alert("该路径已存在于系统环境变量中！");
                    return;
                }
            }
            systemPaths.push_back(EnvPathItem_t{newPath, true}); // 默认启用
            win->systemPathTable->setPaths(systemPaths);
        }
    }

    static void closeCallback(Fl_Widget *w, void *data)
    {
        MainWindow *win = static_cast<MainWindow *>(data);
        win->hide(); // 隐藏窗口
        delete win;  // 手动销毁窗口对象，触发析构函数
        win = nullptr;
    }

    // 处理PathTable焦点切换
    void handleTableFocus(PathTable* focusedTable)
    {
        if (focusedTable == systemPathTable)
        {
            userPathTable->clearSelection();
        }
        else if (focusedTable == userPathTable)
        {
            systemPathTable->clearSelection();
        }
    }

    void initPaths()
    {
        namespace fs = std::filesystem;
        using json = nlohmann::json;

        // 获取用户目录
        char *userProfile = nullptr;
        size_t len = 0;
        if (_dupenv_s(&userProfile, &len, "USERPROFILE") != 0 || userProfile == nullptr)
        {
            fl_alert("无法获取用户目录！");
            return;
        }

        // 使用 userProfile
        fs::path appDataPath = fs::path(userProfile) / "AppData/Local/QuickManPath";

        // 释放分配的内存
        free(userProfile);

        fs::path jsonFilePath = appDataPath / "pathVars.json";

        // 如果目录不存在，则创建
        if (!fs::exists(appDataPath))
        {
            fs::create_directories(appDataPath);
        }

        // 如果文件不存在，则创建并写入默认内容
        if (!fs::exists(jsonFilePath))
        {
            json defaultJson = {
                {"systemPaths", std::map<std::string, bool>()},
                {"userPaths", std::map<std::string, bool>()}
            };
            std::ofstream outFile(jsonFilePath);
            outFile << defaultJson.dump(4); // 格式化输出
            outFile.close();
        }

        // 读取 JSON 文件
        std::ifstream inFile(jsonFilePath);
        json pathData;
        inFile >> pathData;
        inFile.close();

        // 从 JSON 数据中加载路径
        auto preSystemPaths = pathData.value("systemPaths", std::map<std::string, bool>());
        auto preUserPaths = pathData.value("userPaths", std::map<std::string, bool>());

        // load local path
        auto locSystemPaths = getSystemPath();
        auto locUserPaths = getUserPath();

        // 合并 preSystemPaths 和 locSystemPaths
        for (const auto &envItem : locSystemPaths)
        {
            preSystemPaths[envItem.path] = envItem.enabled;
        }

        // 合并 preUserPaths 和 locUserPaths
        for (const auto &envItem : locUserPaths)
        {
            preUserPaths[envItem.path] = envItem.enabled;
        }

        std::vector<EnvPathItem_t> systemPathVec;
        for (const auto &pair : preSystemPaths)
        {
            systemPathVec.push_back(EnvPathItem_t{pair.first, pair.second});
        }
        systemPathTable->setPaths(systemPathVec);
        std::vector<EnvPathItem_t> userPathVec;
        for (const auto &pair : preUserPaths)
        {
            userPathVec.push_back(EnvPathItem_t{pair.first, pair.second});
        }
        userPathTable->setPaths(userPathVec);
    }

    void refreshPaths()
    {
        // load local path
        auto locSystemPaths = getSystemPath();
        auto locUserPaths = getUserPath();

        std::vector<EnvPathItem_t> curSystemPaths;
        systemPathTable->getPaths(curSystemPaths);
        std::vector<EnvPathItem_t> curUserPaths;
        userPathTable->getPaths(curUserPaths);

        // 合并 preSystemPaths 和 locSystemPaths
        for (const auto &envItem : locSystemPaths)
        {
            bool found = false;
            for(auto &item : curSystemPaths)
            {
                if (item.path == envItem.path)
                {
                    item.enabled = envItem.enabled;
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                curSystemPaths.push_back(envItem);
            }
        }

        // 合并 preUserPaths 和 locUserPaths
        for (const auto &envItem : locUserPaths)
        {
            bool found = false;
            for(auto &item : curUserPaths)
            {
                if (item.path == envItem.path)
                {
                    item.enabled = envItem.enabled;
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                curUserPaths.push_back(envItem);
            }
        }

        systemPathTable->setPaths(curSystemPaths);
        userPathTable->setPaths(curUserPaths);
        fl_message("刷新成功！");
    }

    void applyPaths()
    {
        //1. 获取当前path及其状态，如果EnvPathItem_t.enable，则应用，否则不应用，
        //2. 注意当前系统的path，如果其被修改为disable，或者不在path里面，删除
        std::vector<EnvPathItem_t> curSystemPaths;
        systemPathTable->getPaths(curSystemPaths);
        std::vector<EnvPathItem_t> curUserPaths;
        userPathTable->getPaths(curUserPaths);

        bool res0 = setSystemPath(curSystemPaths);
        bool res1 = setUserPath(curUserPaths);

        if(res0 && res1)
        {
            fl_message("路径应用成功！");
        }
        else
        {
            fl_message("路径应用失败！");
        }
    }

public:
    MainWindow(int W, int H, int titleBarH, const char *L = 0) : Fl_Window(W, H, L)
    {
        // 设置窗口为双缓冲模式以减少闪烁
        // set_output();
        
        // 设置窗口关闭回调
        callback(closeCallback, this);

        // 设置窗口图标将在窗口显示后进行

        // 创建主布局容器 - 垂直排列
        mainPack = new Fl_Pack(0, 0, W, H);
        mainPack->type(Fl_Pack::VERTICAL);
        mainPack->spacing(0); // 组间无间距
        mainPack->begin();

        // 用户Path组
        userGroup = new Fl_Group(0, 0, W, groupH);
        userGroup->box(FL_FLAT_BOX);
        userGroup->begin();

        userLabel = new Fl_Box(labelLeftMargin, labelTopMargin, W - labelLeftMargin * 2, labelH, "用户环境变量 Path:");
        userLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

        userPathTable = new PathTable(tabelLeftMargin, labelWholeH, W - tabelLeftMargin * 2, tabelH);
        userPathTable->cols(4);
        userPathTable->col_width(0, fixedCellW);
        userPathTable->col_width(1, W - fixedCellW * 3 - tabelLeftMargin * 2);
        userPathTable->col_width(2, fixedCellW);
        userPathTable->col_width(3, fixedCellW);
        
        // 设置焦点回调
        userPathTable->setFocusCallback([](PathTable* table) {
            MainWindow* win = static_cast<MainWindow*>(table->parent());
            if (win) {
                win->handleTableFocus(table);
            }
        });

        userGroup->end();
        userGroup->resizable(userPathTable);

        // 系统Path组
        systemGroup = new Fl_Group(0, 0, W, groupH);
        systemGroup->box(FL_FLAT_BOX);
        systemGroup->begin();
        
        systemLabel = new Fl_Box(labelLeftMargin, labelTopMargin, W - labelLeftMargin * 2, labelH, "系统环境变量 Path:");
        systemLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        
        systemPathTable = new PathTable(tabelLeftMargin, labelWholeH, W - tabelLeftMargin * 2, tabelH);
        systemPathTable->cols(4);
        systemPathTable->col_width(0, fixedCellW);
        systemPathTable->col_width(1, W - fixedCellW * 3 - tabelLeftMargin * 2);
        systemPathTable->col_width(2, fixedCellW);
        systemPathTable->col_width(3, fixedCellW);
        
        // 设置焦点回调
        systemPathTable->setFocusCallback([](PathTable* table) {
            MainWindow* win = static_cast<MainWindow*>(table->parent());
            if (win) {
                win->handleTableFocus(table);
            }
        });
        
        systemGroup->end();
        systemGroup->resizable(systemPathTable);

        // 按钮组 - 放在底部，固定高度
        buttonGroup = new Fl_Group(0, 0, W, buttonWholeH);
        buttonGroup->box(FL_FLAT_BOX);
        buttonGroup->begin();
        
        // 计算4个按钮的布局
        int buttonSpacing = 10; // 按钮间距
        int totalButtonWidth = buttonW * 4 + buttonSpacing * 3; // 4个按钮的总宽度
        int startX = (W - totalButtonWidth) / 2; // 起始X坐标，使按钮居中
        int buttonY = (buttonWholeH - buttonH) / 2; // Y坐标，垂直居中
        
        // 创建4个按钮
        newUserButton = new Fl_Button(startX, buttonY, buttonW, buttonH, "新建用户变量");
        newUserButton->callback(newUserCallback, this);
        
        newSystemButton = new Fl_Button(startX + buttonW + buttonSpacing, buttonY, buttonW, buttonH, "新建系统变量");
        newSystemButton->callback(newSystemCallback, this);
        
        refreshButton = new Fl_Button(startX + (buttonW + buttonSpacing) * 2, buttonY, buttonW, buttonH, "刷新");
        refreshButton->callback(refreshCallback, this);

        applyButton = new Fl_Button(startX + (buttonW + buttonSpacing) * 3, buttonY, buttonW, buttonH, "应用");
        applyButton->callback(applyCallback, this);
        
        buttonGroup->end();
        buttonGroup->resizable(0); // 按钮组不可调整大小，保持固定高度

        mainPack->end();

        // 设置窗口可调整大小
        resizable(mainPack);
        // 计算最小宽度：4个按钮 + 3个间距 + 左右边距
        int minWindowWidth = buttonW * 4 + 10 * 3 + 20; // 20是左右边距
        size_range(minWindowWidth, groupH * 2 + buttonWholeH + titleBarH);

        // 初始加载数据
        initPaths();
    }

    ~MainWindow()
    {
        namespace fs = std::filesystem;
        using json = nlohmann::json;

        // 获取用户目录
        char *userProfile = nullptr;
        size_t len = 0;
        if (_dupenv_s(&userProfile, &len, "USERPROFILE") != 0 || userProfile == nullptr)
        {
            fl_alert("无法获取用户目录！");
            return;
        }

        // 使用 userProfile
        fs::path appDataPath = fs::path(userProfile) / "AppData/Local/QuickManPath";

        // 释放分配的内存
        free(userProfile);

        fs::path jsonFilePath = appDataPath / "pathVars.json";

        // 获取当前路径数据
        std::vector<EnvPathItem_t> systemPaths;
        systemPathTable->getPaths(systemPaths);

        std::vector<EnvPathItem_t> userPaths;
        userPathTable->getPaths(userPaths);

        // 转换为map格式以便JSON序列化
        std::map<std::string, bool> systemPathsMap;
        std::map<std::string, bool> userPathsMap;
        
        for (const auto& item : systemPaths) {
            systemPathsMap[item.path] = item.enabled;
        }
        
        for (const auto& item : userPaths) {
            userPathsMap[item.path] = item.enabled;
        }

        // 写入 JSON 文件
        json pathData = {
            {"systemPaths", systemPathsMap},
            {"userPaths", userPathsMap}
        };
        std::ofstream outFile(jsonFilePath);
        if (outFile.is_open())
        {
            outFile << pathData.dump(4); // 格式化输出
            outFile.close();
        }
        else
        {
            fl_alert("无法写入路径数据到 JSON 文件！");
        }
    }

    void resize(int X, int Y, int W, int H) override
    {
        Fl_Window::resize(X, Y, W, H);
        

        int availableHeight = H - buttonWholeH;

        size_t total_path_len = systemPathTable->getPathLength() + userPathTable->getPathLength() + 2; // 2 is table header

        int systemHeight = static_cast<int>((availableHeight - 2 * labelWholeH) * (systemPathTable->getPathLength() + 1) / total_path_len + labelWholeH);
        int userHeight = static_cast<int>((availableHeight - 2 * labelWholeH) * (userPathTable->getPathLength() + 1) / total_path_len + labelWholeH);

        userGroup->resize(0, 0, W, userHeight);
        systemGroup->resize(0, userHeight, W, systemHeight);
        buttonGroup->resize(0, availableHeight, W, buttonWholeH);

        // 调整 userPathTable 的宽度
        if (userPathTable)
        {
            int scroll_bar_width = userPathTable->scrollbar_size();
            if(scroll_bar_width == 0)
            {
                scroll_bar_width = Fl::scrollbar_size();
            }
            // userPathTable->resize(10, 40, W - 20, userHeight - 50); // 保持边距一致
            userPathTable->col_width(0, fixedCellW);
            userPathTable->col_width(1, W - fixedCellW * 3 - tabelLeftMargin * 2 - scroll_bar_width);
            userPathTable->col_width(2, fixedCellW);
            userPathTable->col_width(3, fixedCellW);
        }
        if (systemPathTable)
        {
            int scroll_bar_width = systemPathTable->scrollbar_size();
            if(scroll_bar_width == 0)
            {
                scroll_bar_width = Fl::scrollbar_size();
            }
            // systemPathTable->resize(10, userHeight + 40, W - 20, systemHeight - 50); // 保持边距一致
            systemPathTable->col_width(0, fixedCellW);
            systemPathTable->col_width(1, W - fixedCellW * 3 - tabelLeftMargin * 2 - scroll_bar_width);
            systemPathTable->col_width(2, fixedCellW);
            systemPathTable->col_width(3, fixedCellW);
        }

        if (newUserButton && newSystemButton && refreshButton && applyButton && buttonGroup)
        {
            int buttonSpacing = 10; // 按钮间距
            int totalButtonWidth = buttonW * 4 + buttonSpacing * 3; // 4个按钮的总宽度
            int startX = (buttonGroup->w() - totalButtonWidth) / 2; // 起始X坐标，使按钮居中
            int buttonY = (buttonWholeH - buttonH) / 2; // Y坐标，垂直居中
            
            // 更新所有4个按钮的位置
            newUserButton->position(startX, availableHeight + buttonY);
            newSystemButton->position(startX + buttonW + buttonSpacing, availableHeight + buttonY);
            refreshButton->position(startX + (buttonW + buttonSpacing) * 2, availableHeight + buttonY);
            applyButton->position(startX + (buttonW + buttonSpacing) * 3, availableHeight + buttonY);
        }
    }
};

int main(int argc, char **argv)
{
    int titleBarHeight = getTitleBarHeight();
    // 计算合适的初始窗口宽度以容纳4个按钮
    int initialWidth = max(700, buttonW * 4 + 10 * 3 + 40); // 4个按钮 + 3个间距 + 额外边距
    MainWindow *window = new MainWindow(initialWidth, 740, titleBarHeight, "QuickManPath");
    window->show(argc, argv);
    
    // 设置窗口图标
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), "QuickPathManIcon");
    if (hIcon && window->shown())
    {
        // 获取窗口句柄并设置图标
        HWND hwnd = fl_xid(window);
        if (hwnd)
        {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
    }
    
    return Fl::run();
}
