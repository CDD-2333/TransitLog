# TransitLog · 行程记录

> 多用户 / 云同步的行程记录桌面应用。当前处于**设计阶段**，尚未编码。

- 技术栈：**Qt 6 + C++17 + SQLite**（离线优先）＋ HTTP 同步服务
- 主题：延续 MemoRise 的浅色纸感 Material 风格，并新增可切换深色主题

## 当前进度（2026-08-03）

| 阶段 | 状态 |
|---|---|
| 需求评审 | ✅ 见 `docs/requirements-review.md` |
| 数据库设计 | ⏳ 待确认 `docs/database-design.md`（ER 图 + 建表 SQL） |
| P0 界面线框 | ✅ 见 `docs/ui-wireframe.md` |
| 工程初始化 / 编码 | ⏳ 待确认后开始 |

## 设计文档

- [数据库设计（ER + SQL）](./docs/database-design.md) ｜ [ER 图](./docs/er-diagram.svg)
- [P0 界面线框](./docs/ui-wireframe.md)
- [需求评审与更优实现建议](./docs/requirements-review.md)

## 构建（待工程初始化后生效）

用 Qt Creator 打开 `TransitLog.pro`，配置 Qt 6 的 MSVC / MinGW kit 后构建。
