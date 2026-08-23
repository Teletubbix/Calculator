/*
 * Calculator v4.0.0 —— GTK4 图形界面（跨平台，可交叉编译到 Windows）
 *
 * 界面：
 *   - 顶部：表达式输入框 + 结果标签
 *   - 中部：整齐的计算按钮（数字、运算符、常用函数）
 *   - 底部一行：0 / 00 / . / DEG(RAD) 切换
 *   - 支持键盘直接输入，Enter 计算
 */

#include <gtk/gtk.h>
#include "calculator.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define GUI_VERSION "4.0.0"

typedef struct {
    GtkWidget *entry;        /* 表达式输入框 */
    GtkWidget *result;       /* 结果标签 */
    GtkWidget *mode_btn;     /* 角度制切换按钮 */
    double ans;              /* 上一次结果（完整精度） */
    int has_ans;             /* 是否已有结果 */
    CalcAngleMode mode;      /* 当前角度制 */
} AppState;

static AppState g_state;

static void insert_text(const char *text) {
    GtkEditable *ed = GTK_EDITABLE(g_state.entry);
    int pos = gtk_editable_get_position(ed);
    gtk_editable_insert_text(ed, text, -1, &pos);
    gtk_editable_set_position(ed, pos);
    gtk_widget_grab_focus(g_state.entry);
}

static void on_button_clicked(GtkButton *button, gpointer data) {
    (void)data;
    insert_text(gtk_button_get_label(button));
}

static void on_clear(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    gtk_editable_set_text(GTK_EDITABLE(g_state.entry), "");
    gtk_label_set_label(GTK_LABEL(g_state.result), "");
    gtk_widget_grab_focus(g_state.entry);
}

static void on_backspace(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    GtkEditable *ed = GTK_EDITABLE(g_state.entry);
    GtkEntryBuffer *buf = gtk_entry_get_buffer(GTK_ENTRY(g_state.entry));
    int len = (int)gtk_entry_buffer_get_length(buf);
    if (len > 0) {
        gtk_editable_delete_text(ed, len - 1, len);
    }
    gtk_widget_grab_focus(g_state.entry);
}

static void do_evaluate(void) {
    const char *expr = gtk_editable_get_text(GTK_EDITABLE(g_state.entry));
    double result = 0.0;
    char error[512] = {0};
    int rc = calc_evaluate_mode(expr, g_state.mode,
                                g_state.ans, g_state.has_ans,
                                &result, error, sizeof(error));
    if (rc != 0) {
        gtk_label_set_label(GTK_LABEL(g_state.result), error);
        return;
    }
    g_state.ans = result;
    g_state.has_ans = 1;
    char out[64];
    snprintf(out, sizeof(out), "= %.15g", result);
    gtk_label_set_label(GTK_LABEL(g_state.result), out);
}

static void on_equals(GtkButton *button, gpointer data) {
    (void)button; (void)data;
    do_evaluate();
}

static void on_activate(GtkEntry *entry, gpointer data) {
    (void)entry; (void)data;
    do_evaluate();
}

static void on_mode_toggle(GtkButton *button, gpointer data) {
    (void)data;
    g_state.mode = (g_state.mode == CALC_MODE_DEG) ? CALC_MODE_RAD : CALC_MODE_DEG;
    const char *txt = (g_state.mode == CALC_MODE_DEG) ? "DEG" : "RAD";
    gtk_button_set_label(button, txt);
    gtk_widget_grab_focus(g_state.entry);
}

static void grid_add(GtkGrid *grid, const char *label, int col, int row,
                     void (*cb)(GtkButton*, gpointer)) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    if (cb != NULL) {
        g_signal_connect(btn, "clicked", G_CALLBACK(cb), NULL);
    } else {
        g_signal_connect(btn, "clicked", G_CALLBACK(on_button_clicked), NULL);
    }
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_widget_set_vexpand(btn, TRUE);
    gtk_grid_attach(grid, btn, col, row, 1, 1);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)app; (void)user_data;
    memset(&g_state, 0, sizeof(g_state));
    g_state.mode = CALC_MODE_RAD;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Calculator " GUI_VERSION);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 460);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);

    /* 输入框 */
    g_state.entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_state.entry), "输入表达式，如 sin(pi/2)+3");
    gtk_widget_set_hexpand(g_state.entry, TRUE);
    g_signal_connect(g_state.entry, "activate", G_CALLBACK(on_activate), NULL);
    gtk_box_append(GTK_BOX(box), g_state.entry);

    /* 结果标签 */
    g_state.result = gtk_label_new("");
    gtk_widget_set_hexpand(g_state.result, TRUE);
    gtk_label_set_xalign(GTK_LABEL(g_state.result), 0.0);
    gtk_label_set_selectable(GTK_LABEL(g_state.result), TRUE);
    gtk_widget_set_margin_top(g_state.result, 2);
    gtk_box_append(GTK_BOX(box), g_state.result);

    /* 按钮网格（6 列） */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_widget_set_vexpand(grid, TRUE);
    gtk_box_append(GTK_BOX(box), grid);

    /* 第 0 行：函数 */
    grid_add(GTK_GRID(grid), "sin",  0, 0, NULL);
    grid_add(GTK_GRID(grid), "cos",  1, 0, NULL);
    grid_add(GTK_GRID(grid), "tan",  2, 0, NULL);
    grid_add(GTK_GRID(grid), "sqrt", 3, 0, NULL);
    grid_add(GTK_GRID(grid), "ln",   4, 0, NULL);
    grid_add(GTK_GRID(grid), "log",  5, 0, NULL);
    /* 第 1 行：反三角 / 幂 */
    grid_add(GTK_GRID(grid), "asin", 0, 1, NULL);
    grid_add(GTK_GRID(grid), "acos", 1, 1, NULL);
    grid_add(GTK_GRID(grid), "atan", 2, 1, NULL);
    grid_add(GTK_GRID(grid), "pow",  3, 1, NULL);
    grid_add(GTK_GRID(grid), "mod",  4, 1, NULL);
    grid_add(GTK_GRID(grid), "gcd",  5, 1, NULL);
    /* 第 2 行：控制 & 常量 */
    grid_add(GTK_GRID(grid), "C",    0, 2, on_clear);
    grid_add(GTK_GRID(grid), "Del",  1, 2, on_backspace);
    grid_add(GTK_GRID(grid), "Ans",  2, 2, NULL);
    grid_add(GTK_GRID(grid), "pi",   3, 2, NULL);
    grid_add(GTK_GRID(grid), "e",    4, 2, NULL);
    grid_add(GTK_GRID(grid), "=",    5, 2, on_equals);
    /* 第 3 行：7 8 9 */
    grid_add(GTK_GRID(grid), "7", 0, 3, NULL);
    grid_add(GTK_GRID(grid), "8", 1, 3, NULL);
    grid_add(GTK_GRID(grid), "9", 2, 3, NULL);
    grid_add(GTK_GRID(grid), "/", 3, 3, NULL);
    grid_add(GTK_GRID(grid), "*", 4, 3, NULL);
    grid_add(GTK_GRID(grid), "-", 5, 3, NULL);
    /* 第 4 行：4 5 6 */
    grid_add(GTK_GRID(grid), "4", 0, 4, NULL);
    grid_add(GTK_GRID(grid), "5", 1, 4, NULL);
    grid_add(GTK_GRID(grid), "6", 2, 4, NULL);
    grid_add(GTK_GRID(grid), "+", 3, 4, NULL);
    grid_add(GTK_GRID(grid), "^", 4, 4, NULL);
    grid_add(GTK_GRID(grid), "!", 5, 4, NULL);
    /* 第 5 行：1 2 3 */
    grid_add(GTK_GRID(grid), "1", 0, 5, NULL);
    grid_add(GTK_GRID(grid), "2", 1, 5, NULL);
    grid_add(GTK_GRID(grid), "3", 2, 5, NULL);
    grid_add(GTK_GRID(grid), "(", 3, 5, NULL);
    grid_add(GTK_GRID(grid), ")", 4, 5, NULL);
    grid_add(GTK_GRID(grid), ".", 5, 5, NULL);
    /* 第 6 行：0 与角度制切换 */
    grid_add(GTK_GRID(grid), "0", 0, 6, NULL);
    grid_add(GTK_GRID(grid), "00", 1, 6, NULL);
    grid_add(GTK_GRID(grid), "exp", 2, 6, NULL);
    grid_add(GTK_GRID(grid), "CLEAR", 3, 6, on_clear);
    g_state.mode_btn = gtk_button_new_with_label("RAD");
    g_signal_connect(g_state.mode_btn, "clicked", G_CALLBACK(on_mode_toggle), NULL);
    gtk_widget_set_hexpand(g_state.mode_btn, TRUE);
    gtk_widget_set_vexpand(g_state.mode_btn, TRUE);
    gtk_grid_attach(GTK_GRID(grid), g_state.mode_btn, 4, 6, 1, 1);
    grid_add(GTK_GRID(grid), "tau", 5, 6, NULL);

    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.teletubbix.calculator", G_APPLICATION_DEFAULT_FLAGS);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
