# 🎨 Calculator 主题设计笔记（v6.0 主题拓展用）

> 搜集到的二次元/好看配色、渐变组合、设计范例。后续做"素材背景主题"时从这里取材。

## 一、灵感/范例来源（参考链接）
- **Anime 配色站**：https://colormagic.app/palette/6735b4b4dd671510623cd7bc （90s 动漫风）、https://uiuxshowcase.com/resources/anicolors-discover-create-anime-colors/（Anicolors 动漫配色）
- **彩色计算器 UI**（明暗主题切换）：https://github.com/CodewithSaket/3-D-Calculator
- **GTK 主题范例**：https://github.com/bjarneo/omarchy-gtk-theme
- **计算器 App 设计**（dribbble）：https://dribbble.com/shots/8648579-Calcul8or-Calculator-iOS-App、https://dribbble.com/shots/24532585-Daily-UI-Challenge-Calculator

## 二、精选二次元配色（可直接用于 CSS/渐变）
### 樱 Sakura 系
- 渐变：`#ff9a9e` → `#fecfef`；主色 `#e91e63`/`#d5006d`；文字 `#5a1f3a`
### 星夜 Starry（紫蓝星空）
- 渐变：`#0f0c29` → `#302b63` → `#24243e`；点缀 `#7c4dff`/`#00b4d8`；文字浅色
### 薄荷 Mint
- 渐变：`#43e97b` → `#38f9d7`；主色 `#059669`
### 黄昏 Sunset
- 渐变：`#fa709a` → `#fee140`；主色 `#e65100`
### 马卡龙 Pastel（软萌马卡龙）
- `#ffd3e0`、`#c7f2ff`、`#fff3b0`、`#ccffcc`、`#e5ccff`（五色软调，可互搭）

## 三、渐变背景组合（window 用 `linear-gradient(160deg, a, b)`）
| 风格 | 渐变 |
|---|---|
| 樱花 | `#ff9a9e → #fecfef` |
| 星夜 | `#0f0c29 → #24243e` |
| 天空 | `#48c6ef → #6f86d6` |
| 蜜桃 | `#f8b500 → #fceabb` |
| 紫罗兰 | `#a18cd1 → #fbc2eb` |

## 四、高对比原则（重点）
- **背景渐变 + 浅色/高亮按键**；数字键用白底深字；运算符/等号用醒目色+白字。
- **避免**深底+黑键（低对比），这正是初版被吐槽的"看不清"。
- 键位统一 6 列分组，主题只改配色/尺寸，布局稳定。

## 五、后续"素材背景主题"（需图片素材）
- 若用二次元立绘/背景图：需要你提供素材（或开放版权图），做成背景层 + 半透明面板。
- 待做：`Theme` 增加背景图片字段，CSS `background-image: url(...)`；面板加 `rgba` 背景保证文字可读。
