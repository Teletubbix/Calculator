/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */
/*
 * Calculator v5.0.0 —— GTK4 图形界面（跨平台，可交叉编译）
 * 主题系统：多套高对比、二次元风格主题（樱/海/薰衣草/薄荷/黄昏）。
 * 主题与窗口分辨率、键位大小、字体、配色深度绑定（系统工程）。
 * 布局统一为有序的 6 列分组（控制行/函数两行/数字运算符/底部）。
 */

#include <gtk/gtk.h>
#include "calculator.h"
#include "matrix.h"
#include "complex.h"
#include "db.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define GUI_VERSION "5.0.0"

/* —— 原神主题（渐变背景，数字/函数=浅底深字，运算符/等号=深底白字，保证高对比）—— */
typedef struct {
    const char *name;
    const char *win_grad1, *win_grad2;
    const char *display_bg, *display_fg, *result_fg, *accent;
    const char *btn_bg, *btn_fg;          /* 基础(未分类) —— 浅底深字 */
    const char *digit_bg, *digit_fg;      /* 数字 —— 白底深字 */
    const char *fn_bg, *fn_fg;            /* 函数 —— 浅色底深字 */
    const char *op_bg, *op_fg;            /* 运算符 —— 深底白字 */
    const char *equals_bg, *equals_fg;    /* 等号 —— 深底白字 */
    const char *clear_bg, *clear_fg;      /* 清除 —— 中深底白字 */
    const char *mode_bg, *mode_fg;        /* 模式 —— 深底白字 */
    int win_w, win_h, key_w, key_h, font, radius;
} Theme;

static const Theme THEMES[] = {
    /* 蒙德 · 风（青绿） */
    { "原神·蒙德 风", "#5795a8", "#cde6ec", "#eef6f8", "#1f3b40", "#2b8397", "#8ab9c9",
      "#ffffff", "#1f3b40", "#ffffff", "#1f3b40", "#d9edf0", "#17343a",
      "#1f6b7d", "#ffffff", "#125563", "#ffffff", "#e07a5f", "#ffffff",
      "#2b8397", "#ffffff", 440, 560, 62, 52, 16, 14 },
    /* 璃月 · 岩（金） */
    { "原神·璃月 岩", "#c9a96a", "#efe0c0", "#fbf5e8", "#3a2c14", "#b8860b", "#d8bd8a",
      "#ffffff", "#3a2c14", "#ffffff", "#3a2c14", "#f0e6cc", "#3a2c14",
      "#8a6d3b", "#ffffff", "#6b5025", "#ffffff", "#b5651f", "#ffffff",
      "#96743d", "#ffffff", 440, 560, 62, 52, 16, 14 },
    /* 稻妻 · 雷（紫） */
    { "原神·稻妻 雷", "#6a5a9e", "#c3b3ea", "#f4f0fb", "#241a3d", "#7c3aed", "#b9a6e8",
      "#ffffff", "#241a3d", "#ffffff", "#241a3d", "#e5dbf5", "#241a3d",
      "#5e35b1", "#ffffff", "#4527a0", "#ffffff", "#8e3ae8", "#ffffff",
      "#6a1db9", "#ffffff", 440, 560, 62, 52, 16, 14 },
    /* 须弥 · 草（绿） */
    { "原神·须弥 草", "#4a9e6b", "#c2e6cf", "#eefaf2", "#123726", "#1f9d55", "#9fd4b4",
      "#ffffff", "#123726", "#ffffff", "#123726", "#dff2e5", "#123726",
      "#15803d", "#ffffff", "#0f6a33", "#ffffff", "#65a30d", "#ffffff",
      "#0e7a3d", "#ffffff", 440, 560, 62, 52, 16, 14 },
    /* 枫丹 · 水（蓝） */
    { "原神·枫丹 水", "#4a90c9", "#bcd8ec", "#eaf4fb", "#12283c", "#1e90d6", "#a8cbe6",
      "#ffffff", "#12283c", "#ffffff", "#12283c", "#dcecf7", "#12283c",
      "#1565c0", "#ffffff", "#0d47a1", "#ffffff", "#e5731f", "#ffffff",
      "#1a73b8", "#ffffff", 440, 560, 62, 52, 16, 14 },
};
#define NTHEMES ((int)(sizeof THEMES / sizeof THEMES[0]))

typedef struct {
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *result;
    GtkWidget *mode_btn;
    GtkWidget *theme_btn;
    int theme_idx;
    CalcComplex ans;
    int has_ans;
    CalcAngleMode mode;
} AppState;

static AppState g_state;

/* 根据主题生成 CSS 并应用（分辨率/键位/配色绑定主题） */
static void apply_theme(const Theme *t) {
    char css[2400];
    snprintf(css, sizeof(css),
        "window { background-image: linear-gradient(160deg, %s, %s); }\n"
        "#display { background-color: %s; color: %s; font-family: \"Consolas\",\"DejaVu Sans Mono\",monospace;"
        "  font-size: %dpx; padding: 12px 14px; border-radius: %dpx; border: 1px solid %s; min-height: 40px; }\n"
        "#result { color: %s; font-size: 15px; font-weight: 600; padding: 4px 8px; min-height: 20px; }\n"
        "button { min-width: %dpx; min-height: %dpx; font-size: %dpx; font-weight: 600;"
        "  border-radius: %dpx; border: none; background-color: %s; color: %s; padding: 0; }\n"
        "button:hover { filter: brightness(1.06); }\n"
        "button:active { filter: brightness(0.90); }\n"
        "button.digit { background-color: %s; color: %s; }\n"
        "button.fn { background-color: %s; color: %s; }\n"
        "button.op { background-color: %s; color: %s; }\n"
        "button.equals { background-color: %s; color: %s; }\n"
        "button.clear { background-color: %s; color: %s; }\n"
        "button.mode { background-color: %s; color: %s; }\n",
        t->win_grad1, t->win_grad2,
        t->display_bg, t->display_fg, t->font + 6, t->radius, t->accent,
        t->result_fg, t->key_w, t->key_h, t->font, t->radius,
        t->btn_bg, t->btn_fg, t->digit_bg, t->digit_fg, t->fn_bg, t->fn_fg,
        t->op_bg, t->op_fg, t->equals_bg, t->equals_fg, t->clear_bg, t->clear_fg,
        t->mode_bg, t->mode_fg);
    GtkCssProvider *p = gtk_css_provider_new();
    gtk_css_provider_load_from_string(p, css);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(p),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(p);
    gtk_window_set_default_size(GTK_WINDOW(g_state.window), t->win_w, t->win_h);
}

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
    if (len > 0) gtk_editable_delete_text(ed, len - 1, len);
    gtk_widget_grab_focus(g_state.entry);
}

static void do_evaluate(void) {
    const char *expr = gtk_editable_get_text(GTK_EDITABLE(g_state.entry));
    CalcComplex result = {0, 0};
    char error[512] = {0};
    int rc = calc_evaluate_complex(expr, g_state.mode, g_state.ans, g_state.has_ans,
                                   &result, error, sizeof(error));
    if (rc != 0) { gtk_label_set_label(GTK_LABEL(g_state.result), error); return; }
    g_state.ans = result;
    g_state.has_ans = 1;
    double tol = 1e-12 * (1.0 + fabs(result.re) + fabs(result.im));
    char out[160];
    if (fabs(result.im) < tol) snprintf(out, sizeof(out), "= %.15g", result.re);
    else snprintf(out, sizeof(out), "= %.15g %s %.15gj", result.re,
                  (result.im < 0.0 ? "-" : "+"), fabs(result.im));
    gtk_label_set_label(GTK_LABEL(g_state.result), out);
}

static void on_equals(GtkButton *button, gpointer data) { (void)button; (void)data; do_evaluate(); }
static void on_activate(GtkEntry *entry, gpointer data) { (void)entry; (void)data; do_evaluate(); }

static void on_mode_toggle(GtkButton *button, gpointer data) {
    (void)data;
    g_state.mode = (g_state.mode == CALC_MODE_DEG) ? CALC_MODE_RAD : CALC_MODE_DEG;
    gtk_button_set_label(button, (g_state.mode == CALC_MODE_DEG) ? "DEG" : "RAD");
    gtk_widget_grab_focus(g_state.entry);
}

static void on_theme_switch(GtkButton *button, gpointer data) {
    (void)data;
    g_state.theme_idx = (g_state.theme_idx + 1) % NTHEMES;
    apply_theme(&THEMES[g_state.theme_idx]);
    gtk_button_set_label(button, THEMES[g_state.theme_idx].name);
    gtk_widget_grab_focus(g_state.entry);
}

static void grid_add(GtkGrid *grid, const char *label, const char *css_class,
                     int col, int row, void (*cb)(GtkButton*, gpointer)) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    if (css_class != NULL) gtk_widget_add_css_class(btn, css_class);
    if (cb != NULL) g_signal_connect(btn, "clicked", G_CALLBACK(cb), NULL);
    else g_signal_connect(btn, "clicked", G_CALLBACK(on_button_clicked), NULL);
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_widget_set_vexpand(btn, TRUE);
    gtk_grid_attach(grid, btn, col, row, 1, 1);
}

static void activate(GtkApplication *app, gpointer user_data) {
    (void)app; (void)user_data;
    memset(&g_state, 0, sizeof(g_state));
    g_state.mode = CALC_MODE_RAD;
    g_state.theme_idx = 0;

    GtkWidget *window = gtk_application_window_new(app);
    g_state.window = window;
    gtk_window_set_title(GTK_WINDOW(window), "Calculator " GUI_VERSION);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    g_state.entry = gtk_entry_new();
    gtk_widget_set_name(g_state.entry, "display");
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_state.entry), "输入表达式，如 sin(pi/2)+3");
    gtk_widget_set_hexpand(g_state.entry, TRUE);
    g_signal_connect(g_state.entry, "activate", G_CALLBACK(on_activate), NULL);
    gtk_box_append(GTK_BOX(box), g_state.entry);

    g_state.result = gtk_label_new("");
    gtk_widget_set_name(g_state.result, "result");
    gtk_widget_set_hexpand(g_state.result, TRUE);
    gtk_label_set_xalign(GTK_LABEL(g_state.result), 0.0);
    gtk_label_set_selectable(GTK_LABEL(g_state.result), TRUE);
    gtk_box_append(GTK_BOX(box), g_state.result);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_vexpand(grid, TRUE);
    gtk_box_append(GTK_BOX(box), grid);

    /* 可控、有序的 6 列分组布局 */
    grid_add(GTK_GRID(grid), "C", "clear", 0, 0, on_clear);
    grid_add(GTK_GRID(grid), "Del", "clear", 1, 0, on_backspace);
    grid_add(GTK_GRID(grid), "Ans", "digit", 2, 0, NULL);
    grid_add(GTK_GRID(grid), "pi", "digit", 3, 0, NULL);
    grid_add(GTK_GRID(grid), "e", "digit", 4, 0, NULL);
    grid_add(GTK_GRID(grid), "=", "equals", 5, 0, on_equals);

    grid_add(GTK_GRID(grid), "sin", "fn", 0, 1, NULL);
    grid_add(GTK_GRID(grid), "cos", "fn", 1, 1, NULL);
    grid_add(GTK_GRID(grid), "tan", "fn", 2, 1, NULL);
    grid_add(GTK_GRID(grid), "sqrt", "fn", 3, 1, NULL);
    grid_add(GTK_GRID(grid), "ln", "fn", 4, 1, NULL);
    grid_add(GTK_GRID(grid), "log", "fn", 5, 1, NULL);

    grid_add(GTK_GRID(grid), "asin", "fn", 0, 2, NULL);
    grid_add(GTK_GRID(grid), "acos", "fn", 1, 2, NULL);
    grid_add(GTK_GRID(grid), "atan", "fn", 2, 2, NULL);
    grid_add(GTK_GRID(grid), "pow", "fn", 3, 2, NULL);
    grid_add(GTK_GRID(grid), "mod", "fn", 4, 2, NULL);
    grid_add(GTK_GRID(grid), "gcd", "fn", 5, 2, NULL);

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

    grid_add(GTK_GRID(grid), "0", "digit", 0, 6, NULL);
    grid_add(GTK_GRID(grid), "00", "digit", 1, 6, NULL);
    grid_add(GTK_GRID(grid), "exp", "fn", 2, 6, NULL);
    grid_add(GTK_GRID(grid), "tau", "digit", 3, 6, NULL);
    g_state.mode_btn = gtk_button_new_with_label("RAD");
    gtk_widget_add_css_class(g_state.mode_btn, "mode");
    g_signal_connect(g_state.mode_btn, "clicked", G_CALLBACK(on_mode_toggle), NULL);
    gtk_widget_set_hexpand(g_state.mode_btn, TRUE);
    gtk_widget_set_vexpand(g_state.mode_btn, TRUE);
    gtk_grid_attach(GTK_GRID(grid), g_state.mode_btn, 4, 6, 1, 1);

    g_state.theme_btn = gtk_button_new_with_label(THEMES[0].name);
    gtk_widget_add_css_class(g_state.theme_btn, "mode");
    g_signal_connect(g_state.theme_btn, "clicked", G_CALLBACK(on_theme_switch), NULL);
    gtk_widget_set_hexpand(g_state.theme_btn, TRUE);
    gtk_widget_set_vexpand(g_state.theme_btn, TRUE);
    gtk_grid_attach(GTK_GRID(grid), g_state.theme_btn, 5, 6, 1, 1);

    gtk_window_set_child(GTK_WINDOW(window), box);
    apply_theme(&THEMES[0]);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    size_t n;
    const calc_function *f = calc_matrix_functions(&n);
    calc_register_functions(f, n);
    f = calc_complex_functions(&n);
    calc_register_functions(f, n);
    f = calc_db_functions(&n);
    calc_register_functions(f, n);

    GtkApplication *app = gtk_application_new("org.teletubbix.calculator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
