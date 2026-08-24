/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */
/*
 * Calculator v6.0.3 —— GTK4 图形界面（跨平台，可交叉编译）
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
#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GUI_VERSION "6.0.3"

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
    const char *bg_img;                    /* 可选背景图（绝对/相对路径，存在才叠加） */
} Theme;

static const Theme THEMES[] = {
    /* 蒙德 · 风（青绿）—— 全部符号统一近黑，浅色底保证高可读 */
    { "原神·蒙德 风", "#5795a8", "#cde6ec", "#eef6f8", "#1f3b40", "#2b8397", "#8ab9c9",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#d9edf0", "#111111",
      "#eaf2f5", "#111111", "#dceaef", "#111111", "#eaf2f5", "#111111",
      "#e5f0f3", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/mondstadt.png" },
    /* 璃月 · 岩（金） */
    { "原神·璃月 岩", "#c9a96a", "#efe0c0", "#fbf5e8", "#3a2c14", "#b8860b", "#d8bd8a",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#f0e6cc", "#111111",
      "#f8f3ea", "#111111", "#f3ebdc", "#111111", "#f8f3ea", "#111111",
      "#f6f0e5", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/liyue.png" },
    /* 稻妻 · 雷（紫） */
    { "原神·稻妻 雷", "#6a5a9e", "#c3b3ea", "#f4f0fb", "#241a3d", "#7c3aed", "#b9a6e8",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#e5dbf5", "#111111",
      "#f2effb", "#111111", "#eae4f8", "#111111", "#f2effb", "#111111",
      "#f0ebfa", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/inazuma.png" },
    /* 须弥 · 草（绿） */
    { "原神·须弥 草", "#4a9e6b", "#c2e6cf", "#eefaf2", "#123726", "#1f9d55", "#9fd4b4",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#dff2e5", "#111111",
      "#eef7f2", "#111111", "#e2f2e8", "#111111", "#eef7f2", "#111111",
      "#eaf6ee", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/sumeru.png" },
    /* 枫丹 · 水（蓝） */
    { "原神·枫丹 水", "#4a90c9", "#bcd8ec", "#eaf4fb", "#12283c", "#1e90d6", "#a8cbe6",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#dcecf7", "#111111",
      "#eff6fb", "#111111", "#e5eff8", "#111111", "#eff6fb", "#111111",
      "#ecf4fa", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/fontaine.png" },
    /* 纳塔 · 火（橙红，v6.0 新增） */
    { "原神·纳塔 火", "#c65b3a", "#f3d0a8", "#fdf3e4", "#3a1d10", "#d9541e", "#f0b48a",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#f8e0c8", "#111111",
      "#fcf2ea", "#111111", "#fae8dc", "#111111", "#fcf2ea", "#111111",
      "#fceee5", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/natlan.png" },
    /* 至冬 · 冰（冰蓝白，v6.0 新增） */
    { "原神·至冬 冰", "#5b7ba3", "#dbe7f5", "#f2f7fc", "#14273c", "#2f7fb8", "#b3cee6",
      "#f2f4f6", "#111111", "#ffffff", "#111111", "#e2eef9", "#111111",
      "#f1f6fb", "#111111", "#e8f0f8", "#111111", "#f1f6fb", "#111111",
      "#eef4fa", "#111111", 420, 540, 60, 46, 16, 14, "themes/img/snezhnaya.png" },
};
#define NTHEMES ((int)(sizeof THEMES / sizeof THEMES[0]))

typedef struct {
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *result;
    GtkWidget *mode_btn;
    GtkWidget *theme_btn;
    GtkWidget *polar_btn;
    GtkWidget *nota_btn;
    int theme_idx;
    int polar;        /* 0=直角坐标(a+bj)，1=极坐标(r∠θ) 显示 */
    int notation;     /* 0=自动 1=科学计数 2=工程计数 */
    CalcComplex ans;
    int has_ans;
    CalcAngleMode mode;
} AppState;

static AppState g_state;

static char g_exe_dir[1024];

/* 计算可执行文件所在目录（跨平台），供解析主题背景图使用。 */
static void resolve_exe_dir(void) {
    char tmp[1024];
#ifdef _WIN32
    DWORD n = GetModuleFileNameA(NULL, tmp, (DWORD)sizeof(tmp));
    if (n == 0 || n >= sizeof(tmp)) { g_exe_dir[0] = '\0'; return; }
#else
    ssize_t n = readlink("/proc/self/exe", tmp, sizeof(tmp) - 1);
    if (n <= 0) { g_exe_dir[0] = '\0'; return; }
    tmp[n] = '\0';
#endif
    char *slash = strrchr(tmp, '/');
    char *bslash = strrchr(tmp, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;
    if (slash) *slash = '\0';
    snprintf(g_exe_dir, sizeof(g_exe_dir), "%s", tmp);
}

/* 解析主题背景图：若 rel 指定的文件存在，返回 1 并把绝对/相对路径写入 out。
 * 依次尝试：相对 cwd、可执行文件同级、可执行文件/themes/img/<basename>。 */
static int asset_path(const char *rel, char *out, size_t sz) {
    char cand[1024];
    if (rel == NULL || rel[0] == '\0') { out[0] = '\0'; return 0; }
    if (g_file_test(rel, G_FILE_TEST_IS_REGULAR)) { snprintf(out, sz, "%s", rel); return 1; }
    const char *base = strrchr(rel, '/');
    if (!base) base = strrchr(rel, '\\');
    base = base ? base + 1 : rel;
    if (g_exe_dir[0]) {
        snprintf(cand, sizeof(cand), "%s/%s", g_exe_dir, rel);
        if (g_file_test(cand, G_FILE_TEST_IS_REGULAR)) { snprintf(out, sz, "%s", cand); return 1; }
        snprintf(cand, sizeof(cand), "%s/themes/img/%s", g_exe_dir, base);
        if (g_file_test(cand, G_FILE_TEST_IS_REGULAR)) { snprintf(out, sz, "%s", cand); return 1; }
    }
    snprintf(out, sz, "%s", rel);
    return 0;
}

/* 根据主题生成 CSS 并应用（分辨率/键位/配色绑定主题）。
 * v6.0：若主题指定了存在的背景图，则叠加在渐变之上。
 * v6.0.1：把 Genshin 徽标作为背景暗纹，融入窗口背景。 */
static void apply_theme(const Theme *t) {
    char css[3200];
    char winrule[1200];
    char logorule[1400] = "";
    char img[1024];
    if (t->bg_img != NULL && asset_path(t->bg_img, img, sizeof(img))) {
        snprintf(winrule, sizeof(winrule),
                 "window { background-image: url(\"%s\"), linear-gradient(160deg, %s, %s);"
                 " background-size: cover, auto; background-position: center, center; }\n",
                 img, t->win_grad1, t->win_grad2);
    } else {
        snprintf(winrule, sizeof(winrule),
                 "window { background-image: linear-gradient(160deg, %s, %s); }\n",
                 t->win_grad1, t->win_grad2);
    }
    /* 大号、半透明的原作徽标层，铺在按键网格之下（被按键遮住也没关系） */
    {
        char logopath[1024];
        if (asset_path("assets/logo.png", logopath, sizeof(logopath))) {
            snprintf(logorule, sizeof(logorule),
                ".calc-bg { background-image: url(\"%s\");"
                " background-size: 150%% auto; background-position: center 70px;"
                " background-repeat: no-repeat; opacity: 0.34; }\n",
                logopath);
        }
    }
    snprintf(css, sizeof(css),
        "%s"
        "%s"
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
        winrule, logorule,
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

/* 数值格式化：auto / 科学计数(e) / 工程计数(k M G m µ n …) */
static void fmt_number(double v, int notation, char *out, size_t sz) {
    if (!isfinite(v)) { snprintf(out, sz, "%.15g", v); return; }
    if (notation == 1) { snprintf(out, sz, "%.15e", v); return; }   /* 科学计数 */
    if (notation == 2) {                                             /* 工程计数 */
        if (v == 0) { snprintf(out, sz, "0"); return; }
        double a = fabs(v);
        int e = (int)floor(log10(a));
        int e3 = ((e % 3) + 3) % 3;      /* 归一化到 [0,3) 的余数 */
        int expo = e - e3;
        double mant = v / pow(10.0, expo);
        static const char *const pre[] = { "", "k", "M", "G", "T", "P", "E" };
        static const char *const neg[] = { "", "m", "\xc2\xb5", "n", "p", "f", "a" };
        const char *suffix;
        if (expo >= 0) { suffix = (expo/3 < (int)(sizeof(pre)/sizeof(pre[0]))) ? pre[expo/3] : "e"; }
        else           { suffix = (-expo/3 < (int)(sizeof(neg)/sizeof(neg[0]))) ? neg[(-expo)/3] : "e"; }
        if (strcmp(suffix,"e")==0) { snprintf(out, sz, "%.15e", v); return; }
        snprintf(out, sz, "%.12g%s", mant, suffix);
        return;
    }
    snprintf(out, sz, "%.15g", v);       /* 自动 */
}

/* 组装结果显示字符串，遵循 polar / notation / mode 状态 */
static void show_result(void) {
    CalcComplex z = g_state.ans;
    char out[192];
    double tol = 1e-12 * (1.0 + fabs(z.re) + fabs(z.im));

    if (g_state.polar) {
        double r = hypot(z.re, z.im);
        double th = atan2(z.im, z.re);
        double th_disp = th;
        const char *unit;
        if (g_state.mode == CALC_MODE_DEG)     { th_disp = th * 180.0 / M_PI; unit = "\xc2\xb0"; }
        else if (g_state.mode == CALC_MODE_GRAD){ th_disp = th * 200.0 / M_PI; unit = "g"; }
        else unit = "rad";
        char rs[64], ts[64];
        fmt_number(r, g_state.notation, rs, sizeof(rs));
        fmt_number(th_disp, g_state.notation, ts, sizeof(ts));
        snprintf(out, sizeof(out), "= %s \xe2\x88\xa0 %s%s", rs, ts, unit);
    } else {
        char re[64], im[64];
        fmt_number(z.re, g_state.notation, re, sizeof(re));
        if (fabs(z.im) < tol) {
            snprintf(out, sizeof(out), "= %s", re);
        } else {
            fmt_number(fabs(z.im), g_state.notation, im, sizeof(im));
            snprintf(out, sizeof(out), "= %s %s %sj", re, (z.im < 0.0 ? "-" : "+"), im);
        }
    }
    gtk_label_set_label(GTK_LABEL(g_state.result), out);
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
    show_result();
}

static void on_equals(GtkButton *button, gpointer data) { (void)button; (void)data; do_evaluate(); }
static void on_activate(GtkEntry *entry, gpointer data) { (void)entry; (void)data; do_evaluate(); }

static void on_mode_toggle(GtkButton *button, gpointer data) {
    (void)data;
    g_state.mode = (g_state.mode == CALC_MODE_DEG) ? CALC_MODE_RAD : CALC_MODE_DEG;
    gtk_button_set_label(button, (g_state.mode == CALC_MODE_DEG) ? "DEG" : "RAD");
    if (g_state.has_ans) show_result();
    gtk_widget_grab_focus(g_state.entry);
}

static void on_polar_toggle(GtkButton *button, gpointer data) {
    (void)data;
    g_state.polar = !g_state.polar;
    gtk_button_set_label(button, g_state.polar ? "R∠θ" : "a+bj");
    if (g_state.has_ans) show_result();
    gtk_widget_grab_focus(g_state.entry);
}

static void on_nota_toggle(GtkButton *button, gpointer data) {
    (void)data;
    g_state.notation = (g_state.notation + 1) % 3;
    const char *lbl = (g_state.notation == 0) ? "标准" :
                      (g_state.notation == 1) ? "科学" : "工程";
    gtk_button_set_label(button, lbl);
    if (g_state.has_ans) show_result();
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
    resolve_exe_dir();

    GtkWidget *window = gtk_application_window_new(app);
    g_state.window = window;
    gtk_window_set_title(GTK_WINDOW(window), "Calculator " GUI_VERSION);

    /* 用 GtkOverlay 把大号、半透明的原神徽标铺在按键层之下，融入窗口背景 */
    GtkWidget *overlay = gtk_overlay_new();
    GtkWidget *bgbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(bgbox, TRUE);
    gtk_widget_set_vexpand(bgbox, TRUE);
    gtk_widget_add_css_class(bgbox, "calc-bg");
    gtk_overlay_set_child(GTK_OVERLAY(overlay), bgbox);

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
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
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

    /* 显示格式切换行（v6.0）：极坐标/直角坐标 + 记数法 */
    g_state.polar_btn = gtk_button_new_with_label("a+bj");
    gtk_widget_add_css_class(g_state.polar_btn, "mode");
    g_signal_connect(g_state.polar_btn, "clicked", G_CALLBACK(on_polar_toggle), NULL);
    gtk_widget_set_hexpand(g_state.polar_btn, TRUE);
    gtk_widget_set_vexpand(g_state.polar_btn, TRUE);
    gtk_grid_attach(GTK_GRID(grid), g_state.polar_btn, 0, 7, 3, 1);

    g_state.nota_btn = gtk_button_new_with_label("标准");
    gtk_widget_add_css_class(g_state.nota_btn, "mode");
    g_signal_connect(g_state.nota_btn, "clicked", G_CALLBACK(on_nota_toggle), NULL);
    gtk_widget_set_hexpand(g_state.nota_btn, TRUE);
    gtk_widget_set_vexpand(g_state.nota_btn, TRUE);
    gtk_grid_attach(GTK_GRID(grid), g_state.nota_btn, 3, 7, 3, 1);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), box);
    gtk_window_set_child(GTK_WINDOW(window), overlay);
    apply_theme(&THEMES[0]);
    gtk_window_present(GTK_WINDOW(window));
}

/* 过滤 Windows 上无害的 dbus 警告，其余日志交给默认处理器 */
static void quiet_gio_log(const gchar *domain, GLogLevelFlags level,
                          const gchar *message, gpointer user_data) {
    (void)user_data;
    if (domain && message && strstr(message, "win32 session dbus binary not found")) {
        return;
    }
    g_log_default_handler(domain, level, message, user_data);
}

int main(int argc, char **argv) {
    /* 抑制 Windows 上无害的 "win32 session dbus binary not found" 警告 */
    g_log_set_handler("GLib-GIO",
                      G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE,
                      (GLogFunc)quiet_gio_log, NULL);

    size_t n;
    const calc_function *f = calc_matrix_functions(&n);
    calc_register_functions(f, n);
    f = calc_complex_functions(&n);
    calc_register_functions(f, n);
    f = calc_db_functions(&n);
    calc_register_functions(f, n);

    /* NON_UNIQUE：避免 Windows 无 dbus 时报 “win32 session dbus binary not found” */
    GtkApplication *app = gtk_application_new("org.teletubbix.calculator",
                                              G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
