/*
 * Calculator v4.1.0 —— GTK4 图形界面（跨平台，可交叉编译到 Windows）
 *
 * 界面（现代深色主题，分组清晰）：
 *   - 顶部：大号显示屏（表达式输入）+ 结果标签
 *   - 中部：按功能分组的按钮：
 *       控制行(清除/常量/等号)
 *       函数行A(三角/开根/对数)
 *       函数行B(反三角/幂/杂项)
 *       数字与运算符
 *   - 底部：0/00/./exp/角度制切换
 *   - 支持键盘直接输入，Enter 计算
 */

#include <gtk/gtk.h>
#include "calculator.h"
#include "matrix.h"
#include "complex.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define GUI_VERSION "4.2.0"

/* —— 主题（东京之夜 / Tokyonight 风格）—— */
static const char *CSS =
    "window { background-color: #1a1b26; }\n"
    "grid { padding: 4px; }\n"
    "#display {\n"
    "  background-color: #24283b;\n"
    "  color: #c0caf5;\n"
    "  font-family: \"Consolas\", \"DejaVu Sans Mono\", monospace;\n"
    "  font-size: 22px;\n"
    "  padding: 12px 14px;\n"
    "  border-radius: 10px;\n"
    "  border: 1px solid #3b4261;\n"
    "  min-height: 40px;\n"
    "}\n"
    "#result {\n"
    "  color: #9ece6a;\n"
    "  font-size: 17px;\n"
    "  font-weight: 600;\n"
    "  padding: 4px 8px;\n"
    "  min-height: 22px;\n"
    "}\n"
    "button {\n"
    "  min-width: 56px;\n"
    "  min-height: 48px;\n"
    "  font-size: 16px;\n"
    "  font-weight: 600;\n"
    "  border-radius: 10px;\n"
    "  border: none;\n"
    "  background-color: #2f3549;\n"
    "  color: #c0caf5;\n"
    "  padding: 0;\n"
    "}\n"
    "button:hover { background-color: #3b4261; }\n"
    "button:active { background-color: #565f89; }\n"
    "button.digit { background-color: #24283b; }\n"
    "button.fn { background-color: #3b4261; color: #c0caf5; }\n"
    "button.op { background-color: #7aa2f7; color: #1a1b26; }\n"
    "button.equals { background-color: #9ece6a; color: #1a1b26; }\n"
    "button.clear { background-color: #f7768e; color: #1a1b26; }\n"
    "button.mode { background-color: #bb9af7; color: #1a1b26; }\n";

typedef struct {
    GtkWidget *entry;        /* 表达式输入框（显示屏） */
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

static void grid_add(GtkGrid *grid, const char *label, const char *css_class,
                     int col, int row, void (*cb)(GtkButton*, gpointer)) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    if (css_class != NULL) {
        gtk_widget_add_css_class(btn, css_class);
    }
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

    /* 载入主题 */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, CSS);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Calculator " GUI_VERSION);
    gtk_window_set_default_size(GTK_WINDOW(window), 440, 520);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    /* 显示屏 */
    g_state.entry = gtk_entry_new();
    gtk_widget_set_name(g_state.entry, "display");
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_state.entry), "输入表达式，如 sin(pi/2)+3");
    gtk_widget_set_hexpand(g_state.entry, TRUE);
    g_signal_connect(g_state.entry, "activate", G_CALLBACK(on_activate), NULL);
    gtk_box_append(GTK_BOX(box), g_state.entry);

    /* 结果标签 */
    g_state.result = gtk_label_new("");
    gtk_widget_set_name(g_state.result, "result");
    gtk_widget_set_hexpand(g_state.result, TRUE);
    gtk_label_set_xalign(GTK_LABEL(g_state.result), 0.0);
    gtk_label_set_selectable(GTK_LABEL(g_state.result), TRUE);
    gtk_box_append(GTK_BOX(box), g_state.result);

    /* 按钮网格（6 列） */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_vexpand(grid, TRUE);
    gtk_box_append(GTK_BOX(box), grid);

    /* 控制行：清除 / 常量 / 等号 */
    grid_add(GTK_GRID(grid), "C",   "clear", 0, 0, on_clear);
    grid_add(GTK_GRID(grid), "Del", "clear", 1, 0, on_backspace);
    grid_add(GTK_GRID(grid), "Ans", "digit", 2, 0, NULL);
    grid_add(GTK_GRID(grid), "pi",  "digit", 3, 0, NULL);
    grid_add(GTK_GRID(grid), "e",   "digit", 4, 0, NULL);
    grid_add(GTK_GRID(grid), "=",   "equals", 5, 0, on_equals);

    /* 函数行 A */
    grid_add(GTK_GRID(grid), "sin",  "fn", 0, 1, NULL);
    grid_add(GTK_GRID(grid), "cos",  "fn", 1, 1, NULL);
    grid_add(GTK_GRID(grid), "tan",  "fn", 2, 1, NULL);
    grid_add(GTK_GRID(grid), "sqrt", "fn", 3, 1, NULL);
    grid_add(GTK_GRID(grid), "ln",   "fn", 4, 1, NULL);
    grid_add(GTK_GRID(grid), "log",  "fn", 5, 1, NULL);

    /* 函数行 B */
    grid_add(GTK_GRID(grid), "asin", "fn", 0, 2, NULL);
    grid_add(GTK_GRID(grid), "acos", "fn", 1, 2, NULL);
    grid_add(GTK_GRID(grid), "atan", "fn", 2, 2, NULL);
    grid_add(GTK_GRID(grid), "pow",  "fn", 3, 2, NULL);
    grid_add(GTK_GRID(grid), "mod",  "fn", 4, 2, NULL);
    grid_add(GTK_GRID(grid), "gcd",  "fn", 5, 2, NULL);

    /* 数字 + 运算符 */
    grid_add(GTK_GRID(grid), "7", "digit", 0, 3, NULL);
    grid_add(GTK_GRID(grid), "8", "digit", 1, 3, NULL);
    grid_add(GTK_GRID(grid), "9", "digit", 2, 3, NULL);
    grid_add(GTK_GRID(grid), "/", "op", 3, 3, NULL);
    grid_add(GTK_GRID(grid), "*", "op", 4, 3, NULL);
    grid_add(GTK_GRID(grid), "-", "op", 5, 3, NULL);

    grid_add(GTK_GRID(grid), "4", "digit", 0, 4, NULL);
    grid_add(GTK_GRID(grid), "5", "digit", 1, 4, NULL);
    grid_add(GTK_GRID(grid), "6", "digit", 2, 4, NULL);
    grid_add(GTK_GRID(grid), "+", "op", 3, 4, NULL);
    grid_add(GTK_GRID(grid), "^", "op", 4, 4, NULL);
    grid_add(GTK_GRID(grid), "!", "fn", 5, 4, NULL);

    grid_add(GTK_GRID(grid), "1", "digit", 0, 5, NULL);
    grid_add(GTK_GRID(grid), "2", "digit", 1, 5, NULL);
    grid_add(GTK_GRID(grid), "3", "digit", 2, 5, NULL);
    grid_add(GTK_GRID(grid), "(", "op", 3, 5, NULL);
    grid_add(GTK_GRID(grid), ")", "op", 4, 5, NULL);
    grid_add(GTK_GRID(grid), ".", "digit", 5, 5, NULL);

    /* 底部：0 / 00 / exp / 角度制 / tau */
    grid_add(GTK_GRID(grid), "0",   "digit", 0, 6, NULL);
    grid_add(GTK_GRID(grid), "00",  "digit", 1, 6, NULL);
    grid_add(GTK_GRID(grid), "exp", "fn", 2, 6, NULL);
    grid_add(GTK_GRID(grid), "tau", "digit", 3, 6, NULL);
    g_state.mode_btn = gtk_button_new_with_label("RAD");
    gtk_widget_add_css_class(g_state.mode_btn, "mode");
    g_signal_connect(g_state.mode_btn, "clicked", G_CALLBACK(on_mode_toggle), NULL);
    gtk_widget_set_hexpand(g_state.mode_btn, TRUE);
    gtk_widget_set_vexpand(g_state.mode_btn, TRUE);
    gtk_grid_attach(GTK_GRID(grid), g_state.mode_btn, 4, 6, 1, 1);
    grid_add(GTK_GRID(grid), "phi", "digit", 5, 6, NULL);

    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    size_t n;
    const calc_function *f = calc_matrix_functions(&n);
    calc_register_functions(f, n);
    f = calc_complex_functions(&n);
    calc_register_functions(f, n);

    GtkApplication *app = gtk_application_new("org.teletubbix.calculator", G_APPLICATION_DEFAULT_FLAGS);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
