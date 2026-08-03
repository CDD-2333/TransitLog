# TransitLog · 行程记录

> 本地单机的行程记录桌面应用（**无云同步**，账号为本地登录）。

- 技术栈：**Qt 6 + C++17 + SQLite**
- 主题：延续 MemoRise 的浅色纸感 Material 风格，并新增可切换深色主题
- 范围：本地账号、行程 CRUD（含进行中行程）、交通方式字典、行程标签、统计、主题、备份导出

## 当前进度（2026-08-03）

| 阶段 | 状态 |
|---|---|
| 需求评审 | ✅ 已确认范围（无云）`docs/requirements-review.md` |
| 数据库设计 | ✅ 已确认 `docs/database-design.md`（ER 图 + 建表 SQL） |
| P0 界面线框 | ✅ 见 `docs/ui-wireframe.md` |
| 工程初始化 / 编码 | 🔨 进行中 |

## 设计文档

- [数据库设计（ER + SQL）](./docs/database-design.md) ｜ [ER 图](./docs/er-diagram.svg)
- [P0 界面线框](./docs/ui-wireframe.md)
- [需求评审与更优实现建议](./docs/requirements-review.md)

## 构建（待工程初始化后生效）

用 Qt Creator 打开 `TransitLog.pro`，配置 Qt 6 的 MSVC / MinGW kit 后构建。
