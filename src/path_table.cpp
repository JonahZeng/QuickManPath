#include "path_tabel.hpp"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/fl_ask.H>

PathTable::PathTable(int X, int Y, int W, int H, const char *L) : Fl_Table_Row(X, Y, W, H, L), selectedRow(-1), focusCallback(nullptr)
{
    col_header(1);
    col_resize(1);
    col_header_height(25);
    row_header(0);
    end();
}

void PathTable::setFocusCallback(void (*callback)(PathTable*))
{
    focusCallback = callback;
}

void PathTable::getPaths(std::vector<EnvPathItem_t> &outPathList)
{
    outPathList.clear();
    outPathList = envPaths; // copy
}

void PathTable::setPaths(const std::vector<EnvPathItem_t> &pathList)
{
    envPaths.clear();
    envPaths = pathList; // copy
    delBtnClicked.clear();
    delBtnClicked.resize(pathList.size(), 0);

    rows(static_cast<int>(envPaths.size()));
    redraw();
}

size_t PathTable::getPathLength()
{
    return envPaths.size();
}

void PathTable::clearSelection()
{
    if (selectedRow != -1)
    {
        selectedRow = -1;
        redraw(); // 重绘以恢复原始背景色
    }
}

// 静态回调函数实现 - 恢复按钮状态
void PathTable::resetButtonState(void *data)
{
    ButtonData *btnData = static_cast<ButtonData *>(data);
    if (btnData && btnData->table)
    {
        // 重置按钮点击状态
        if (btnData->row >= 0 && btnData->row < static_cast<int>(btnData->table->delBtnClicked.size()))
        {
            btnData->table->delBtnClicked[btnData->row] = 0;
            btnData->table->redraw(); // 重绘以更新按钮状态
        }
    }

    // 询问用户是否确定删除
    if (btnData && btnData->table && btnData->row >= 0 && btnData->row < static_cast<int>(btnData->table->envPaths.size()))
    {
        std::string pathToDelete = btnData->table->envPaths[btnData->row].path;
        std::string confirmMessage = "确定要删除路径 \"" + pathToDelete + "\" 吗？\n删除的路径将在下一次应用后失效";

        if (fl_ask("%s", confirmMessage.c_str()))
        {
            // 用户确认删除，执行删除操作
            btnData->table->envPaths.erase(btnData->table->envPaths.begin() + btnData->row);
            btnData->table->delBtnClicked.erase(btnData->table->delBtnClicked.begin() + btnData->row);
            btnData->table->rows(static_cast<int>(btnData->table->envPaths.size()));
            btnData->table->redraw();

            fl_message("路径已删除");
        }
    }

    // 释放内存
    delete btnData;
}

int PathTable::handle(int event)
{
    if (event == FL_PUSH)
    {
        // 获取鼠标点击位置
        int X = Fl::event_x();
        int Y = Fl::event_y();

        // 检查点击是否在表格范围内
        if (X >= this->x() && Y >= this->y() &&
            X <= this->x() + this->w() && Y <= this->y() + this->h())
        {

            // 转换为表格相对坐标
            int rel_x = X - this->x();
            int rel_y = Y - this->y();

            // 计算行号（跳过表头）
            int R = 0;
            int row_start = col_header_height();
            for (R = 0; R < rows(); R++)
            {
                int row_h = row_height(R);
                if (rel_y >= row_start && rel_y < row_start + row_h)
                    break;
                row_start += row_h;
            }

            // 计算列号（跳过表头）
            int C = 0;
            int col_start = 0;
            for (C = 0; C < cols(); C++)
            {
                int col_w = col_width(C);
                if (rel_x >= col_start && rel_x < col_start + col_w)
                    break;
                col_start += col_w;
            }

            // 如果点击的是有效的行（不是复选框或删除按钮），设置选中状态
            if (R >= 0 && R < static_cast<int>(envPaths.size()) && C != 2 && C != 3)
            {
                if (selectedRow != R)
                {
                    selectedRow = R;
                    take_focus(); // 获取焦点
                    redraw(); // 重绘以更新选中行的背景色
                    
                    // 通知MainWindow焦点变化
                    if (focusCallback)
                    {
                        focusCallback(this);
                    }
                }
                return 1; // 事件已处理
            }

            // 检查是否点击了复选框列（第2列）
            if (C == 2 && R >= 0 && R < static_cast<int>(envPaths.size()))
            {
                // 切换复选框状态
                envPaths[R].enabled = !envPaths[R].enabled;
                redraw(); // 重绘以更新复选框状态
                return 1; // 事件已处理
            }

            // 检查是否点击了删除按钮列（第3列）
            if (C == 3 && R >= 0 && R < static_cast<int>(envPaths.size()))
            {
                // 设置按钮点击状态并设置定时器恢复
                delBtnClicked[R] = 1;
                redraw();

                // 创建包含行号和this指针的结构用于回调
                PathTable::ButtonData *data = new PathTable::ButtonData{R, this};

                // 设置定时器，200毫秒后恢复按钮状态
                Fl::add_timeout(0.2, resetButtonState, data);

                return 1; // 事件已处理
            }
        }
    }
    else if (event == FL_UNFOCUS)
    {
        // 失去焦点时清除选中状态
        if (selectedRow != -1)
        {
            selectedRow = -1;
            redraw(); // 重绘以恢复原始背景色
        }
    }

    return Fl_Table_Row::handle(event);
}

void PathTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H)
{
    switch (context)
    {
    case CONTEXT_STARTPAGE:
        return;

    case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        {
            fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, col_header_color());
            fl_color(FL_BLACK);
            if (C == 0)
            {
                fl_draw("序号", X, Y, W, H, FL_ALIGN_CENTER);
            }
            else if (C == 1)
            {
                fl_draw("路径", X, Y, W, H, FL_ALIGN_CENTER);
            }
            else if (C == 2)
            {
                fl_draw("开启", X, Y, W, H, FL_ALIGN_CENTER);
            }
        }
        fl_pop_clip();
        return;

    case CONTEXT_CELL:
        fl_push_clip(X, Y, W, H);
        {
            // 背景色 - 如果是选中的行，使用Windows蓝色，否则使用默认颜色
            if (R == selectedRow)
            {
                // Windows蓝色高亮色 (类似系统选中颜色)
                fl_color(fl_rgb_color(204, 232, 255));
            }
            else
            {
                fl_color((R % 2 == 0) ? FL_WHITE : FL_BACKGROUND2_COLOR);
            }
            fl_rectf(X, Y, W, H);

            // 文本和复选框
            fl_color(FL_BLACK);
            if (C == 0)
            {
                fl_draw(std::to_string(R + 1).c_str(), X, Y, W, H, FL_ALIGN_CENTER);
            }
            else if (C == 1 && R < envPaths.size())
            {
                fl_draw(envPaths[R].path.c_str(), X + 2, Y, W - 4, H, FL_ALIGN_LEFT);
            }
            else if (C == 2 && R < envPaths.size())
            {
                // 绘制复选框
                int checkbox_size = H - 4; // 复选框大小略小于单元格高度
                if (checkbox_size > W - 4)
                    checkbox_size = W - 4;

                int checkbox_x = X + (W - checkbox_size) / 2;
                int checkbox_y = Y + (H - checkbox_size) / 2;

                // 绘制复选框边框
                fl_draw_box(FL_DOWN_BOX, checkbox_x, checkbox_y, checkbox_size, checkbox_size, FL_WHITE);

                // 如果选中，绘制勾选标记
                if (envPaths[R].enabled)
                {
                    fl_color(FL_BLACK);
                    fl_line(checkbox_x + 2, checkbox_y + checkbox_size / 2,
                            checkbox_x + checkbox_size / 2, checkbox_y + checkbox_size - 2);
                    fl_line(checkbox_x + checkbox_size / 2, checkbox_y + checkbox_size - 2,
                            checkbox_x + checkbox_size - 2, checkbox_y + 2);
                }
            }
            else if (C == 3 && R < envPaths.size())
            {
                if (delBtnClicked[R] == 0)
                {
                    fl_color(FL_BACKGROUND_COLOR);
                    fl_rectf(X + 2, Y + 2, W - 4, H - 4);
                    fl_color(FL_BLACK);
                    fl_rect(X + 2, Y + 2, W - 4, H - 4);
                    fl_draw("删除", X, Y, W, H, FL_ALIGN_CENTER);
                }
                else
                {
                    // 只绘制点击状态（灰色边框），定时器会负责恢复
                    fl_color(FL_BACKGROUND_COLOR);
                    fl_rectf(X + 2, Y + 2, W - 4, H - 4);
                    fl_color(FL_GRAY);
                    fl_rect(X + 2, Y + 2, W - 4, H - 4);
                    fl_draw("删除", X, Y, W, H, FL_ALIGN_CENTER);
                }
            }

            // 边框
            fl_color(FL_LIGHT2);
            fl_rect(X, Y, W, H);
        }
        fl_pop_clip();
        return;

    default:
        return;
    }
}
