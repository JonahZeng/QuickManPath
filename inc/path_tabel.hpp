#ifndef PATH_TABEL_H
#define PATH_TABEL_H
#include <vector>
#include <string>
#include <FL/Fl_Table_Row.H>

typedef struct EnvPathItem_s {
    std::string path;
    bool enabled;
} EnvPathItem_t;

class PathTable : public Fl_Table_Row
{
private:
    std::vector<EnvPathItem_t> envPaths;
    std::vector<uint8_t> delBtnClicked;
    int selectedRow; // 跟踪被选中的行，-1表示没有选中
    void (*focusCallback)(PathTable*); // 焦点变化回调函数
    
    // 用于定时器回调的数据结构
    struct ButtonData {
        int row;
        PathTable* table;
    };
    
    // 静态回调函数用于处理按钮点击动画
    static void resetButtonState(void *data);
    
public:
    PathTable(int X, int Y, int W, int H, const char *L = 0);
    void setFocusCallback(void (*callback)(PathTable*)); // 设置焦点回调
    void getPaths(std::vector<EnvPathItem_t> &outPathList);
    void setPaths(const std::vector<EnvPathItem_t> &pathList);
    void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) override;
    size_t getPathLength();
    void clearSelection(); // 清除选中状态
    
    // 添加handle方法以更好地控制事件处理
    int handle(int event) override;
};
#endif
