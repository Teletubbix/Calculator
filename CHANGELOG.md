# 更新日志（Changelog）

本文件记录每个版本的变更。格式参考 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)。

## [6.0.7](https://github.com/Teletubbix/Calculator/compare/v6.0.6...v6.0.7) (2026-08-24)


### Bug Fixes

* 数值命令对非法数字输入提示用法并拒绝(integrate/nderiv/root/sum/product) ([78bd15b](https://github.com/Teletubbix/Calculator/commit/78bd15bd9eac68f4e15d2967afc6bd578289ea8d))

## [v6.0.6] - 2026-08-24
### 已添加
- GitHub Actions 自动发布工作流：打 `v*` tag 即自动构建并上传 Release（Linux 包 + Windows 自包含包）。

## [v6.0.5] - 2026-08-24
### 已添加
- 重写 README，与当前功能全面同步；新增 `docs/使用说明.md` 快速上手手册。

### 已修改
- 复用单个 CSS provider，主题切换不再堆积 provider；修复编译告警。

## [v6.0.4] - 2026-08-24
### 已修改
- Windows 图形界面改用 `-mwindows`（GUI 子系统），双击不再弹终端窗口。
- 主题名去掉「原神·」前缀（蒙德 风、至冬 冰…）。

## [v6.0.3] - 2026-08-24
### 已修改
- 默认窗口 440×560 → 420×540、按键 52→46、间距 8→6，初始完整显示。
- 屏蔽 Windows 启动的 `win32 session dbus binary not found` 无害警告。

（更早版本见 Release 历史。）
