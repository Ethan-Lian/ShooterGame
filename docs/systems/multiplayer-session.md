# Multiplayer Session Lifecycle

## Scope and ownership

当前实现面向 Steam 好友邀请、单个本地玩家和 Listen Server。沿用 OnlineSubsystemSteam；NULL / IP PIE 的 Gameplay 验证不能证明 Steam 邀请链路通过。

| Owner | Owned state and responsibilities |
|---|---|
| `UMultiplayerSessionsSubsystem` | 一个 Create / Join / Destroy 操作、operation id、实际绑定的 OSS Interface 与 Delegate Handle；平台邀请回调；连接地址解析 |
| `ULobbyInviteSubsystem` | 一个好友刷新请求、其 Friends Interface、好友缓存、按稳定好友 ID 发出的邀请 |
| `UMultiplayerSessionFlowSubsystem` | 本地流程阶段、目标地图、待确认邀请、退出后的动作、Network / Travel failure 恢复 |
| `UMultiplayerSessionUIManagerSubsystem` | 现有 Widget 的创建、清理、输入模式与鼠标；不调用 OSS 或执行地图旅行 |

所有子系统由 GameInstance 持有并跨地图存在。Widget 不拥有在线请求；旧 Widget 消失不会取消或丢失请求结果。伤害、武器、ASC、比赛规则和玩家复制数据保持在 ShooterGame / Gameplay Framework 中。

Steam Lobby 是在线房间；UE Lobby 地图是等待场景。好友列表不代表已经进入房间的玩家，发送邀请成功不代表好友已加入。

## Request contract

- Session 服务方法和 Flow 命令返回的 `bool` 表示是否受理；受理的请求可能在函数返回前同步完成。最终结果由完成回调或 `OnStateChanged` 表达。
- 被拒绝的底层请求不广播完成。已受理请求只完成一次；在广播前释放自己的操作绑定。OSS 返回 `false` 后，仅当本次 operation id 仍有效时补发失败完成。
- 操作开始前记录所有权，清理时只解绑记录的 Interface / Handle。Deinitialize 解除拥有的 OSS、引擎、LocalPlayer 和 Ticker 绑定，之后不继续业务流程。
- 不提供通用请求队列或自定义 OSS 超时。忙碌期间的新操作拒绝；网络超时沿用引擎。已有 OSS 操作未完成时，失败恢复等待其完成后再 Destroy。
- `Idle` 表示流程可接受新请求；Host / Join 还要求没有游戏连接或残留 named session。已有连接的 PIE Listen Server 不是创建 Steam 房间的入口，应从本地 standalone 主菜单验证此链路。

## Host and start

```text
HostGame
  -> CreatingSession
  -> OSS create completion
  -> Traveling: ServerTravel(Lobby?listen)
  -> target map and local PlayerController ready
  -> Lobby

StartHostedGame (host only)
  -> validate configured gameplay map
  -> enable seamless travel
  -> Traveling: ServerTravel(GameplayMap)
  -> target map and local player ready
  -> InGame
```

Session 创建失败、部分创建残留和第一次 Lobby 旅行失败走清理流程。开始游戏的 `ServerTravel` 请求若被立即拒绝且未进入引擎失败恢复，恢复原来的 seamless 标记和 Lobby 阶段；现有面板重新可操作。旅行开始后发生的引擎失败统一清理并返回菜单。

地图加载完成按 GameInstance 和目标完整 package path 过滤，不把 Transition Map 当成目标地图。地图完成事件与 LocalPlayer 的 Controller 变更事件在下一帧核实，避免使用尚未完成设置的 Controller。

## Accepted invites and connection

```text
Disconnected / Idle
  -> accepted Steam invite
  -> JoiningSession
  -> OSS join completion + resolved address
  -> Traveling: ClientTravel(address)
  -> destination world and ServerConnection's actual local PlayerController ready
  -> Lobby or InGame

Already in a room / game
  -> store pending invite + OnInviteConfirmationRequested
  -> ConfirmPendingInvite
  -> leave current session and connection
  -> local menu ready
  -> join the stored invitation
```

首次加载客户端地图时，UE 会创建临时 PlayerController。Flow 等待 `ULocalPlayer::OnPlayerControllerChanged`，并验证真实 Controller 与 ServerConnection 的 PlayerController 一致，才结束连接阶段。地图加载和 OSS Join 回调都不能单独代表进入房间成功。

`DeclinePendingInvite` 保留当前房间。操作忙碌或清理失败时拒绝新的邀请。接受另一份邀请不会自动销毁现有房间。UIManager 收到 `OnInviteConfirmationRequested` 后显示 WBP 确认弹窗；取消保留当前房间，确认调用 Flow 的退出再加入链路。

## Leave and failure recovery

`LeaveSession` 与失败恢复使用同一条清理链路：停止受理新动作，Destroy 本地 named session，回到本地 Menu 地图关闭游戏连接，然后恢复 Idle。Destroy 失败也返回本地菜单，但保持 CleanupFailed，阻止再次创建 / 加入；再次调用 LeaveSession 重试清理。返回 Menu 本身失败不会自动循环旅行。

NetworkFailure 按 World / PendingNetGame 所属 GameInstance 和 Game / Pending NetDriver 过滤。房主收到某个客户端的 ConnectionLost、ConnectionTimeout、NetGuidMismatch 或 NetChecksumMismatch 时，保留自己的房间。实际连接失败和 TravelFailure 通过跨 World 的一次性 Core Ticker 延迟处理，让引擎默认断线处理先完成。

`OnStateChanged(State, Error)` 提供阶段和最近错误；新业务动作清空之前的错误。主菜单显示状态和 CleanupFailed 的清理重试；大厅向房主显示好友刷新、邀请、开始游戏，客户端显示等待房主提示；两端都能离开房间。

## Widget Blueprint UI

布局、颜色、字体、按钮状态样式和缩放全部在 Widget Blueprint 的 Designer 中维护，C++ Widget 仅承担状态订阅、数据呈现和命令调用，已移除好友行和邀请面板的运行时控件树构建。RootWidget 仍是 UIManager 内部的 Canvas layer 容器。

| Asset | C++ parent | Purpose |
|---|---|---|
| `/MultiplayerSession/Content/WBP_Menu` | `UMenu` | 原地图入口沿用此资产；创建房间、退出游戏、错误反馈与清理重试 |
| `/MultiplayerSession/UI/WBP_LobbyInvitePanel` | `ULobbyInvitePanelWidget` | 大厅等待界面、好友列表、刷新、开始和离开 |
| `/MultiplayerSession/UI/WBP_LobbyFriendRow` | `ULobbyFriendRowWidget` | 首字母标识、昵称、在线状态、邀请/已发送；离线好友不能点击邀请 |
| `/MultiplayerSession/UI/WBP_InviteConfirmation` | `UMultiplayerInviteConfirmationWidget` | 房内接受新邀请时，确认离开并加入或保留当前房间 |

`LobbyPanelClass` 和 `InviteConfirmationClass` 在 Project Settings → Plugins → Multiplayer Session 配置；好友行 class 在大厅 WBP 的 Class Defaults 配置。`BindWidget` 名称是 C++ / WBP 契约：重命名这些控件前应同时调整对应 C++ 属性。列表通过好友 ID 发邀请，刷新后可以重新发送，不将发送成功解释为玩家已经进房。

主菜单、大厅和确认弹窗使用 UIOnly 输入；开始 Gameplay 后清除大厅并恢复 GameOnly。确认弹窗默认焦点为取消，关闭后恢复下层界面输入。当前没有新增 Gameplay pause menu 或全局打开大厅的快捷键。

WBP 位于插件 `Content/`，遵循项目规则不进入 Git；代码仓库之外需要单独保留这些二进制资产。UI 使用 Slate 纯色、圆角和引擎字体，没有新增外部素材依赖。

## Configuration

Project Settings → Plugins → Multiplayer Session：`MenuMap`、`LobbyMap`、`GameplayMap`、`LobbyPanelClass`、`InviteConfirmationClass`。项目配置在 `Config/DefaultGame.ini`，目标地图和 UI 资产仍需随项目打包。`MenuSetup` 的显式 Lobby 路径可覆盖配置；省略路径时使用配置。客户端解析的地址属于 OSS，不能用本地地图配置替代。

地图配置使用实际 World asset 的完整路径。当前 `GameLevel` 是重定向资产，真实地图为 `GameLevel1`；GameplayMap 和 MapsToCook 使用后者，避免目标地图就绪判断一直等待重定向前的名字。

## Verification

2026-10-05：按当前 `.uproject` 的 UE 5.8.2 验证接口和实现，ShooterGameEditor / Win64 / Development 构建通过。通过运行中的 UE MCP 创建、编译并保存上述四个 WBP，在单进程 Standalone PIE 中点击验证：创建 → Lobby、离开 → Menu、再次创建，以及开始 → GameLevel1 → InGame。退出后旧大厅不再覆盖菜单，开始游戏后大厅清除并恢复 Gameplay 输入。验证结束后恢复编辑器原有的 Listen Server / 2 clients 设置；没有添加测试代码或 smoke-test 脚本。

这轮 PIE 使用本地 OSS；Steam 好友接口不可用时已确认界面显示失败提示与空列表。好友邀请、跨账号加入和房间切换确认的真实 Steam 链路尚未验证。引擎本轮 PIE 禁用了 Seamless Travel，因此上述地图切换验证的是非 Seamless 路径，不能代替以下多人验收。

开发者需要分别验证：

1. 两个 Steam 账号：创建 → 邀请 → 加入 → 开始游戏，客户端跟随进入 Gameplay。
2. 连续点击创建、开始和刷新好友：没有并行覆盖、重复完成或重复 Travel。
3. 无效 / 已满 / 已关闭房间、地址解析失败和连接超时：返回可恢复阶段，随后能重新加入。
4. 客户端离开：房主继续运行；房主离开或掉线：客户端清理并返回菜单。
5. 接受另一房间的邀请：未确认时当前房间保留；确认后先退出旧连接，再加入新房间。
6. Destroy 失败：CleanupFailed 阻止新请求，显式重试后恢复。
7. Seamless Travel 后验证 PlayerState / ASC / Inventory / Pawn 生命周期；本次联机改动不代替 Gameplay 回归。
