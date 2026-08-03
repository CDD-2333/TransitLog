# TransitLog 需求评审与更优实现建议

> 针对原需求逐条评审：哪里不合理、更好的做法是什么。原则：**P0 最小可行，别把"多用户+云同步"做成吞掉全部精力的黑箱。**

---

## 1. ⚠️ "多用户 / 云同步"是需求里最大的风险项 —— 必须先定后端方案

**问题**：本地 Qt 桌面应用本身没有"云"。`多用户` + `云同步` 意味着需要：
- 一个**账号体系**（注册/登录/密码存储）
- 一台**同步服务端**（接收、存储、下发行程数据）
- 一套**同步协议**（增量、冲突解决、墓碑）

这是三个独立的中型项目，如果语义不清，会无限膨胀。

**建议（二选一）**：
- **A. 极简自建同步服务（推荐，课程项目可控）**：一个很小的 HTTP 服务（Python/Node + SQLite 或直接 MySQL），只提供 5 个接口：`注册/登录/拉取/上传/心跳`。协议自己定，最可控。
- **B. 第三方 BaaS（最快）**：Supabase / Firebase。Qt 侧只写 REST 调用，账号和同步全托管。代价是引入网络依赖和第三方账号。

**更关键的澄清**：`多用户`是指
- (a) 每人各自独立的行程数据（P0，容易，权限=按 user_id 隔离），还是
- (b) 多人**共享/协作**行程（群组、授权、可见性，P1+，复杂度爆炸）？

**我的结论**：P0 只做 (a)，权限就一行 SQL 的事（`WHERE user_id = ?`）。共享行程列入 P1，且明确告诉用户 P0 不做。

## 2. ⚠️ "复用 MemoRise 的 QSS token 体系" —— 实际上不存在，需要新建

**现状核查**（已读 MemoRise 源码）：
- 颜色是硬编码十六进制，散布在 `mainwindow.cpp` 的 `lightStyleSheet()`、`settingsdialog.cpp`、`main.cpp` 等各处；
- **没有任何深色主题**（`settingsdialog` 只有背景图/透明度），也没有 token 抽象层。

所以"复用"不成立。正确做法是 **TransitLog 里先建一套真正的 token 体系**，作为比 MemoRise 更进一步的地方：

```
ThemeManager
 ├─ Palette:  light / dark 两套 token
 │     --bg, --surface, --primary, --primary-hover,
 │     --danger, --text-primary, --text-secondary,
 │     --border, --accent-* ...
 └─ buildQSS(theme) -> QString   // 只从 token 拼装，不散写颜色
```

- 浅色纸感沿用 MemoRise 的调色（`#f3f4f6` 底 / `#fff` 卡 / 绿 `#4A7C6F`）；
- 深色是**新增**的一套 token，不是简单反色（要处理对比度和字体色）。
- 切换用 `setStyleSheet(ThemeManager::instance().buildQSS())` 全局重刷，一处生效。

## 3. ✔️ "先给表结构，确认后再写代码" —— 好，但注意别过度设计

- 方向正确，避免返工。
- 反建议：**P0 只要 3 张表**（`user` / `trip_record` / `transport_mode`），最多加 `device_sync_state`。同步状态表、标签、附件、共享都是 P1，现在设计了也写不完。
- 表结构设计文档已给出：`docs/database-design.md`。

## 4. ✔️ "MVVM / Model-View 分离，SQL 不写进 UI 槽函数" —— 完全同意，明确架构分层

建议的层级（Qt 原生能力，不需要引入重量级 MVVM 框架）：

```
UI 层        MainWindow / Dialog / Widget   ← 只调 ViewModel 方法，不碰 QSqlQuery
ViewModel    每个页面一个（如 TripListVM / TripEditVM）← 持有业务状态，发信号
Repository    TripRepository / UserRepository   ← 唯一的 SQL 操作点
Service       SyncService（拉/推/冲突合并）、AuthService
DB            QSqlDatabase 连接管理 + schema 迁移（版本化）
```

- 列表用 `QAbstractListModel`（自定义 model 包 `trip_record`），天然契合"不要在 UI 里写 SQL"。
- 同步逻辑放 `SyncService`，与 UI 完全解耦，可单测。

## 5. ⚠️ "每个模块提交前自检：能编译、空数据不崩溃、中文编码正确" —— 好，但要可执行化

把"自检"变成**可重复的清单**而不是口头承诺：

1. **能编译**：加 CI（GitHub Actions，Windows MSVC + Linux），每 push 编译一次——比手工强。
2. **空/异常数据**：约定"数据库永远返回合法行" + 三层防御：
   - Repository 层：`SELECT` 空结果返回默认对象而非 `nullptr`；
   - Model 层：`rowCount()` 为 0 时 View 显示空态页；
   - 数值字段（`distance_m`/`cost_fen` 为 NULL）在 ViewModel 用 `hasValue` 标志，UI 显示 `--`，绝不 `toDouble` 出 NaN。
3. **中文编码**：源码文件存 **UTF-8**（Qt Creator 默认）；字符串用 `QString`；SQLite `PRAGMA encoding='UTF-8'`；读写文件统一 `QTextCodec`/`QStringConverter` UTF-8；**禁止** `QString::fromLocal8Bit` 依赖平台。

## 6. ⚠️ 同步冲突策略 —— LWW 有坑，课程项目够用但要讲清楚

- **LWW（按 updated_at 后写覆盖）**：实现最简单，但"两端同时改同一行程"会整行覆盖，丢一端编辑。
- **op-log 同步**：每端记录操作日志，按字段合并，不丢数据；但实现复杂度高一个量级。
- **建议**：P0 用 LWW + 冲突时以 `updated_at` 大者胜（客户端时钟与服务器时钟有偏差风险，最好服务端盖时间戳）。在设置页明示"最后写入覆盖"，并加**同步前自动备份**（本地 `backup_YYYYMMDD.db`）兜底。

## 7. ⚠️ 密码安全 —— 别存明文

- 哈希用 **PBKDF2**（Qt 无内置，用 OpenSSL 或 QCryptographicHash + 盐迭代）或 **Argon2**。
- 传输必须 HTTPS，绝不可裸 HTTP 传密码。
- 课程项目可接受简化，但要在 README 里写明"生产环境需换用专业服务"。

## 8. ✔️ 金额 / 里程 —— 别用 float（已在表设计落实）

- `cost_fen`（分）、`distance_m`（米）存整数；UI 显示层 `÷100`、`÷1000`。
- 聚合（SUM/AVG）不会累积浮点误差，也不会出现 0.30000000000000004。

## 9. ℹ️ 其他提醒

- **时区**：存 UTC，显示本地（已落实）。
- **"进行中行程"**：`end_time` 为空的行程是否要做（开始即记录、到达结束）？P0 建议**不做**，降低状态机复杂度。
- **软删除**：删除=墓碑同步，UI 里"删除"只是 `is_deleted=1`，需要**二次确认**，避免误删后云端也被清掉。
- **同步失败**：必须可重试、可离线继续记录；同步按钮要有"上次同步时间"反馈（已在设置页线框体现）。

---

## 结论：建议的 P0 范围

| 模块 | 内容 | 复杂度 |
|---|---|---|
| 账号 | 注册/登录/退出/改密（本地+服务端） | 中 |
| 行程 CRUD | 列表/新增/编辑/删除(软删)/详情 | 中 |
| 交通方式字典 | 预置种子数据 + 只读引用 | 低 |
| 统计 | 里程/费用/行程数 + 简单图 | 中 |
| 同步 | 全量首拉 + LWW 增量 + 手动/自动 | 中高 |
| 主题 | ThemeManager 浅色纸感 + 深色 | 中 |

**明确不做（P1+）**：群组共享、附件图片、标签、离线地图、跨平台打包发布。

请逐条确认后，我再进入工程初始化与编码。
