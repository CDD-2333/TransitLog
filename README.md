# TransitLog · 行程记录

> 本地单机的行程记录桌面应用（**无云同步**，账号为本地登录）。

- 技术栈：**Qt 6 + C++17 + SQLite**（CMake 构建）
- 主题：延续 MemoRise 的浅色纸感 Material 风格，并新增可切换深色主题
- 范围：本地账号、行程 CRUD（含进行中行程）、交通方式字典、行程标签、统计、主题、备份导出
- 架构：UI（View）→ ViewModel/AuthController → Repository（SQL 唯一出口）→ SQLite；SQL 不进入 UI 槽函数

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

## 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build
```

**参考工具链**：Qt **6.11.0 MinGW 64 位**（本机 `D:/Qt/qt/6.11.0/mingw_64`，与 MemoRise 一致）。

**Qt Creator 打开 `TransitLog.pro`**（qmake，与 MemoRise 工作流一致），选择 Qt 6.11.0 MinGW kit 即可构建。
也可用 `CMakeLists.txt` 构建（CI 走 CMake）。两种构建系统并存、源码相同。

每次 push 由 GitHub Actions 自动编译并跑冒烟测试（`.github/workflows/build.yml`）：
Ubuntu(GCC) + Windows(MinGW，镜像本地工具链)，版本均为 Qt 6.11.0。

## 测试

```bash
# 数据层冒烟（认证/行程 CRUD/空数据/备份）—— 无需显示环境
./build/TransitLogDataSmoke
# GUI 冒烟（主窗口渲染/卡片/统计抓图）—— 无显示器用 offscreen
QT_QPA_PLATFORM=offscreen ./build/TransitLogGuiSmoke
```

## 数据位置

本地 SQLite 数据库位于系统应用数据目录（Linux `~/.local/share/TransitLog/`、Windows `AppData/Roaming/CDD-2333/TransitLog/`），可在「设置 → 数据管理」导出备份。

## 目录结构

| 路径 | 说明 |
|---|---|
| `src/app/` | 数据库管理、主题 token、会话、认证控制器、PBKDF2 |
| `src/model/` | 实体、行程列表 Model、格式化工具 |
| `src/repo/` | 唯一写 SQL 的 Repository 层 |
| `src/ui/` | 主窗口、登录、编辑、统计、设置、卡片 delegate、图表 |
| `docs/` | 设计文档（DB schema / 线框 / 需求评审） |
