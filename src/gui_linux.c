/*
 * Calculator v3.1.0 —— Linux GTK3 图形界面
 *
 * 界面说明：
 *   - 上方是表达式输入框（可直接用键盘输入）
 *   - 下方是结果标签
 *   - 数字、符号、运算符和常用函数都有独立按钮，点击即可输入
 *   - Enter 计算，Esc 退出，窗口关闭按钮也可退出
 *   - “精度”按钮设置显示的小数位数
 *   - Ans 保存上一次计算结果
 */

#include "calculator.h"

#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define GUI_VERSION "3.1.0"
#define PRECISION_AUTO (-1)

typedef struct {
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *result_label;
    double ans;          /* 上一次计算结果，完整精度 */
    int has_ans;         /* 是否已有上一次结果 */
    int precision;       /* -1 表示自动格式，0~15 表示保留小数位 */
} AppState;

/* ------------------------------------------------------------------ */
/* 格式化与求值                                                         */
/* ------------------------------------------------------------------ */

static void format_result(double value, int precision, char *out, size_t size) {
    if (precision >= 0) {
        snprintf(out, size, "%.*f", precision, value);
    } else {
        snprintf(out, size, "%.15g", value);
    }
}

static void show_error(AppState *app, const char *detail) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "输入有误，请检查表达式");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                             "%s", detail);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* 执行一次计算；成功后更新 Ans 和结果标签，失败时弹窗提示且不退出 */
static void evaluate_expression(AppState *app) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(app->entry));

    if (text == NULL || *text == '\0') {
        gtk_label_set_text(GTK_LABEL(app->result_label), "请先输入表达式");
        return;
    }

    double result = 0.0;
    char error[512];
    if (calc_evaluate_with_ans(text, app->ans, app->has_ans,
                               &result, error, sizeof(error)) != 0) {
        show_error(app, error);
        gtk_label_set_text(GTK_LABEL(app->result_label), "表达式有误");
        return;
    }

    app->ans = result;
    app->has_ans = 1;

    char formatted[128];
    format_result(result, app->precision, formatted, sizeof(formatted));
    char label[160];
    snprintf(label, sizeof(label), "= %s", formatted);
    gtk_label_set_text(GTK_LABEL(app->result_label), label);
}

/* ------------------------------------------------------------------ */
/* 按钮回调                                                             */
/* ------------------------------------------------------------------ */

static void insert_at_cursor(GtkWidget *entry, const char *text) {
    GtkEditable *editable = GTK_EDITABLE(entry);
    gint pos = gtk_editable_get_position(editable);
    gtk_editable_insert_text(editable, text, -1, &pos);
    gtk_editable_set_position(editable, pos);
}

static void on_insert_button(GtkWidget *button, gpointer user_data) {
    (void)button;
    AppState *app = (AppState *)user_data;
    const char *text = (const char *)g_object_get_data(G_OBJECT(button), "token");
    insert_at_cursor(app->entry, text);
    gtk_widget_grab_focus(app->entry);
}

static void on_equals(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    evaluate_expression((AppState *)user_data);
}

static void on_clear(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppState *app = (AppState *)user_data;
    gtk_entry_set_text(GTK_ENTRY(app->entry), "");
    gtk_label_set_text(GTK_LABEL(app->result_label), "");
    gtk_widget_grab_focus(app->entry);
}

static void on_backspace(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppState *app = (AppState *)user_data;
    GtkEditable *editable = GTK_EDITABLE(app->entry);
    gint pos = gtk_editable_get_position(editable);
    if (pos > 0) {
        gtk_editable_delete_text(editable, pos - 1, pos);
        gtk_editable_set_position(editable, pos - 1);
    }
    gtk_widget_grab_focus(app->entry);
}

static void on_precision_dialog(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppState *app = (AppState *)user_data;

    enum { RESPONSE_AUTO = 1, RESPONSE_OK = 2 };

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "显示精度设置",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL,
        "自动", RESPONSE_AUTO,
        "确定", RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new("结果保留几位小数？(0 ~ 15，选择“自动”恢复默认格式)");
    GtkWidget *spin = gtk_spin_button_new_with_range(0.0, 15.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin),
                              app->precision >= 0 ? (double)app->precision : 2.0);

    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(content), spin, FALSE, FALSE, 8);
    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == RESPONSE_AUTO) {
        app->precision = PRECISION_AUTO;
    } else if (response == RESPONSE_OK) {
        app->precision = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    }
    gtk_widget_destroy(dialog);

    /* 如果已经有一个结果，按新精度重新显示 */
    if (app->has_ans) {
        char formatted[128];
        char label[160];
        format_result(app->ans, app->precision, formatted, sizeof(formatted));
        snprintf(label, sizeof(label), "= %s", formatted);
        gtk_label_set_text(GTK_LABEL(app->result_label), label);
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    (void)user_data;
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();   /* Esc 退出，和命令行版保持一致 */
        return TRUE;
    }
    return FALSE;
}

/* ------------------------------------------------------------------ */
/* 界面搭建                                                             */
/* ------------------------------------------------------------------ */

static GtkWidget *new_button(const char *label, const char *token,
                             GCallback callback, AppState *app) {
    GtkWidget *button = gtk_button_new_with_label(label);
    if (callback != NULL) {
        g_signal_connect(button, "clicked", callback, app);
    }
    if (token != NULL) {
        g_object_set_data_full(G_OBJECT(button), "token",
                               g_strdup(token), g_free);
    }
    gtk_widget_set_hexpand(button, TRUE);
    gtk_widget_set_vexpand(button, TRUE);
    return button;
}

static void add_button(GtkGrid *grid, const char *label, const char *token,
                       GCallback callback, AppState *app,
                       int left, int top, int width, int height) {
    GtkWidget *button = new_button(label, token, callback, app);
    gtk_grid_attach(grid, button, left, top, width, height);
}

static void build_ui(AppState *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);

    /* 表达式输入框 */
    app->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->entry),
                                   "输入表达式，例如 sin(pi/2)+5!；也可点击下方按钮输入");
    gtk_entry_set_width_chars(GTK_ENTRY(app->entry), 42);
    gtk_box_pack_start(GTK_BOX(vbox), app->entry, FALSE, FALSE, 4);
    g_signal_connect(app->entry, "activate", G_CALLBACK(on_equals), app);

    /* 结果标签 */
    app->result_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(app->result_label), 0.02);
    gtk_label_set_selectable(GTK_LABEL(app->result_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), app->result_label, FALSE, FALSE, 4);

    /* 按钮网格：6 列 */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 4);

    int r = 0;
    /* 第 0 行：三角函数和常用函数 */
    add_button(GTK_GRID(grid), "sin", "sin(", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), "cos", "cos(", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "tan", "tan(", G_CALLBACK(on_insert_button), app, 2, r, 1, 1);
    add_button(GTK_GRID(grid), "√", "sqrt(", G_CALLBACK(on_insert_button), app, 3, r, 1, 1);
    add_button(GTK_GRID(grid), "ln", "ln(", G_CALLBACK(on_insert_button), app, 4, r, 1, 1);
    add_button(GTK_GRID(grid), "log", "log(", G_CALLBACK(on_insert_button), app, 5, r, 1, 1);

    r++;
    add_button(GTK_GRID(grid), "7", "7", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), "8", "8", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "9", "9", G_CALLBACK(on_insert_button), app, 2, r, 1, 1);
    add_button(GTK_GRID(grid), "÷", "/", G_CALLBACK(on_insert_button), app, 3, r, 1, 1);
    add_button(GTK_GRID(grid), "C", NULL, G_CALLBACK(on_clear), app, 4, r, 2, 1);

    r++;
    add_button(GTK_GRID(grid), "4", "4", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), "5", "5", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "6", "6", G_CALLBACK(on_insert_button), app, 2, r, 1, 1);
    add_button(GTK_GRID(grid), "×", "*", G_CALLBACK(on_insert_button), app, 3, r, 1, 1);
    add_button(GTK_GRID(grid), "⌫", NULL, G_CALLBACK(on_backspace), app, 4, r, 2, 1);

    r++;
    add_button(GTK_GRID(grid), "1", "1", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), "2", "2", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "3", "3", G_CALLBACK(on_insert_button), app, 2, r, 1, 1);
    add_button(GTK_GRID(grid), "−", "-", G_CALLBACK(on_insert_button), app, 3, r, 1, 1);
    add_button(GTK_GRID(grid), "(", "(", G_CALLBACK(on_insert_button), app, 4, r, 1, 1);
    add_button(GTK_GRID(grid), ")", ")", G_CALLBACK(on_insert_button), app, 5, r, 1, 1);

    r++;
    add_button(GTK_GRID(grid), "0", "0", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), ".", ".", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "π", "π", G_CALLBACK(on_insert_button), app, 2, r, 1, 1);
    add_button(GTK_GRID(grid), "+", "+", G_CALLBACK(on_insert_button), app, 3, r, 1, 1);
    add_button(GTK_GRID(grid), "e", "e", G_CALLBACK(on_insert_button), app, 4, r, 1, 1);
    add_button(GTK_GRID(grid), "Ans", "Ans", G_CALLBACK(on_insert_button), app, 5, r, 1, 1);

    r++;
    add_button(GTK_GRID(grid), "x^y", "^", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), "n!", "!", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "log2", "log2(", G_CALLBACK(on_insert_button), app, 2, r, 1, 1);
    add_button(GTK_GRID(grid), "pow", "pow(", G_CALLBACK(on_insert_button), app, 3, r, 1, 1);
    add_button(GTK_GRID(grid), "=", NULL, G_CALLBACK(on_equals), app, 4, r, 2, 1);

    r++;
    add_button(GTK_GRID(grid), "exp", "exp(", G_CALLBACK(on_insert_button), app, 0, r, 1, 1);
    add_button(GTK_GRID(grid), "abs", "abs(", G_CALLBACK(on_insert_button), app, 1, r, 1, 1);
    add_button(GTK_GRID(grid), "精度", NULL, G_CALLBACK(on_precision_dialog), app, 2, r, 4, 1);

    gtk_widget_show_all(vbox);
    gtk_widget_grab_focus(app->entry);
}

/* ------------------------------------------------------------------ */
/* 程序入口                                                             */
/* ------------------------------------------------------------------ */

/* 无窗口自检：验证 GUI 二进制正确链接核心引擎，供 CI / 维护者使用 */
static int run_self_test(void) {
    double r = 0.0;
    char err[512];

    if (calc_evaluate_with_ans("e^2", 0.0, 0, &r, err, sizeof(err)) != 0) {
        fprintf(stderr, "self-test 失败: e^2 %s\n", err);
        return 1;
    }
    if (calc_evaluate_with_ans("sin(Ans)", r, 1, &r, err, sizeof(err)) != 0) {
        fprintf(stderr, "self-test 失败: sin(Ans) %s\n", err);
        return 1;
    }
    const double e = 2.71828182845904523536;
    if (fabs(r - sin(e * e)) > 1e-12) {
        fprintf(stderr, "self-test 数值不符: %g\n", r);
        return 1;
    }
    if (calc_evaluate_with_ans("1/0", 0.0, 0, &r, err, sizeof(err)) == 0) {
        fprintf(stderr, "self-test 失败: 1/0 应报错\n");
        return 1;
    }

    printf("Calculator GTK self-test passed (v%s)\n", GUI_VERSION);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--self-test") == 0) {
        return run_self_test();
    }

    gtk_init(&argc, &argv);

    AppState app;
    memset(&app, 0, sizeof(app));
    app.precision = PRECISION_AUTO;

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Calculator v3.1.0 (Linux GTK)");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 520, 520);
    gtk_container_set_border_width(GTK_CONTAINER(app.window), 10);

    g_signal_connect(app.window, "key-press-event", G_CALLBACK(on_key_press), &app);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    build_ui(&app);

    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
