# TransitLog 需求评审与更优实现建议（v0.2）

> 2026-08-03 确认的变更：**多用户 / 云同步 / 云数据库全部砍掉**，改为**本地登录**。
> 本文档同步更新到该口径。

---

## 1. ✅ 范围收窄：不做云，做本地登录（已确认）

**原问题**：本地 Qt 桌面应用没有"云"，"多用户 + 云同步"意味着账号体系 + 同步服务 + 同步协议三个中型项目，会吞掉全部精力。

**已确认结论**：
- P0 **不做云数据库、不做多用户、不做同步**；
- 账号 = **本地登录**：`user` 表存用户名 + 加盐密码哈希，仅本机认证，相当于一个"本地门禁 + 多本帐"。
- **收益**：工程量骤降，专注把行程记录本身做扎实（数据、交互、主题）。

**代价（要说清楚）**：换机 / 重装 = 数据丢失。所以设置页提供**导出备份 .db**（P0 已纳入），这是"不联网"前提下最实在的数据保险。

**未来**：若做云，表里的 `updated_at` / `is_deleted` 已预留，同步策略已定为 **LWW + 墓碑**（见 `database-design.md` §4）。

## 2. ⚠️ "复用 MemoRise 的 QSS token 体系" —— 实际上不存在，需要新建

**现状核查**（已读 MemoRise 源码）：
- 颜色是硬编码十六进制，散布在 `mainwindow.cpp` 的 `lightStyleSheet()`、`settingsdialog.cpp`、`main.cpp` 等各处；
- **没有任何深色主题**（`settingsdialog` 只有背景图/透明度），也没有 token 抽象层。

所以"复用"不成立。正确做法是 **TransitLog 里先建一套真正的 token 体系**：

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

## 3. ✔️ "先给表结构，确认后再写代码" —— 已确认，避免过度设计

- P0 共 4 张表：`user` / `trip_record` / `transport_mode` / `trip_tag`。已确认（见 `database-design.md`）。
- 标签 P0 做**单标签**（`tag_id` 一列），多标签 junction 表留 P1。

## 4. ✔️ "MVVM / Model-View 分离，SQL 不写进 UI 槽函数" —— 完全同意

建议的层级（Qt 原生能力即可，不引入重量级 MVVM 框架）：

```
UI 层        MainWindow / Dialog / Widget   ← 只调 ViewModel 方法，不碰 QSqlQuery
ViewModel    每个页面一个（如 TripListVM / TripEditVM）← 持有业务状态，发信号
Repository    TripRepository / UserRepository   ← 唯一的 SQL 操作点
DB           QSqlDatabase 连接管理 + schema 迁移（版本化）
```

- 列表用自定义 `QAbstractListModel`，天然契合"不要在 UI 里写 SQL"。

## 5. ⚠️ "每个模块提交前自检：能编译、空数据不崩溃、中文编码正确" —— 可执行化

1. **能编译**：加 CI（GitHub Actions，Windows MSVC + Linux），每 push 编译一次。
2. **空/异常数据**：三层防御：
   - Repository 层：`SELECT` 空结果返回默认对象而非 `nullptr`；
   - Model 层：`rowCount()` 为 0 时 View 显示空态页；
   - 数值字段（`distance_m`/`cost_fen` 为 NULL）在 ViewModel 用 `hasValue` 标志，UI 显示 `--`，绝不 `toDouble` 出 NaN。
3. **中文编码**：源码 UTF-8（Qt Creator 默认）；字符串用 `QString`；SQLite `PRAGMA encoding='UTF-8'`；读写文件统一 UTF-8；**禁止** `QString::fromLocal8Bit` 依赖平台。

## 6. ✔️ LWW —— 已确认作为未来同步策略（当前不用）

- 已定 **LWW（最后写入覆盖）**，简单够用；若将来按字段合并再改 op-log。
- P0 无同步，此项仅保留在 `database-design.md` §4 作将来依据。

## 7. ⚠️ 密码安全 —— 本地也绝不存明文

- 哈希用 **PBKDF2**（`QCryptographicHash` + 盐迭代，或 OpenSSL）或 **Argon2**。
- 本地登录无网络，省去 HTTPS 顾虑，但仍要加盐哈希 + 登录失败限次。

## 8. ✔️ 金额 / 里程 —— 别用 float（已在表设计落实）

- `cost_fen`（分）、`distance_m`（米）存整数；UI 显示层 `÷100`、`÷1000`。
- 聚合（SUM/AVG）不会累积浮点误差。

## 9. ℹ️ 其他提醒（已确认项）

- **时区**：存 UTC，显示本地（已落实）。
- **进行中行程**（`end_time` 空）→ **确认做**。UI 显示"进行中"徽标；列表排序按 `start_time`。
- **软删除**：删除 = `is_deleted=1`，需**二次确认**，避免误删。
- **备份**：设置页"导出备份 .db"替代"同步"作为数据保险。
- **修改密码 / 退出登录**：本地登录的基本账户操作，纳入设置页。

---

## 结论：P0 范围（最终）

| 模块 | 内容 | 复杂度 |
|---|---|---|
| 本地账号 | 注册/登录/退出/修改密码（本地 + 加盐哈希） | 低 |
| 行程 CRUD | 列表/新增/编辑/删除(软删)/详情 + **进行中行程** | 中 |
| 交通方式字典 | 预置种子数据 + 只读引用 | 低 |
| 行程标签 | `trip_tag` 字典 + 单标签选择 | 低 |
| 统计 | 里程/费用/行程数 + 简单图 | 中 |
| 主题 | ThemeManager 浅色纸感 + 深色 | 中 |
| 数据管理 | 导出备份 .db / 清空本地数据 | 低 |

**明确不做（P1+）**：云同步/云数据库、群组共享、附件图片、多标签、离线地图、跨平台打包发布。
