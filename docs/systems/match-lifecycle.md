# Match Lifecycle

## Scope

Issue #3 第一小步：比赛阶段、180 秒计时、最小 HUD 显示。`AShooterGameMode : AGameMode` 与 `AShooterGameState : AGameState` 成对使用，沿用 UE MatchState。

第一步原本使用引擎默认自动开局条件。第四步已由开发者补齐加载等待判断。第五步增加每局独占出生点分配与 3 秒重生：至少一人、最多四人、原成员加载完成且正式 PlayerStart 数量足够时才开局。关闭新玩家加入仍待完成。

第二步已接入 [据点计分基础框架](control-point-scoring.md)：开局重置个人积分，死亡/退出/重生完成刷新据点，结束前结清截止前的独占时间并清理计分 Timer。三个核心函数已由开发者补齐，并确认计分测试通过。

第三步的 [结算与 UI 框架](match-results.md) 增加结果快照复制、动作截止检查、可清理的重生 Timer 和结果 WBP 接口。开发者在 `fbdcf48` 补齐 EndMatch 编排、胜者计算与 UI 状态选择。

第四步已接入 [重开流程](match-restart.md)：按钮请求、同地图 Seamless Travel 配置、旧界面清理和新 HUD 输入恢复。重开校验与加载等待条件已由开发者补齐，2026-10-15 开发者反馈测试完成；测试设备、人数与 Steam 拓扑未单独说明。

## Ownership and Flow

```text
AGameMode checks ReadyToStartMatch
  -> StartMatch / InProgress
  -> ShooterGameMode.HandleMatchHasStarted
       -> shuffle map-authored PlayerStarts and assign one unique Controller.StartSpot each
       -> Super: normal spawn and match-start work
       -> GameState.SetMatchEndServerTime(ServerTime + MatchDuration)
  -> AGameMode.Tick checks ReadyToEndMatch
       -> deadline reached
       -> EndMatch / WaitingPostMatch
  -> AGameMode forwards MatchState to GameState
  -> GameState replicates phase and end time to clients
  -> local HUD reads the current snapshot and server time
```

- GameMode 只存在于服务器，拥有比赛规则和 `MatchDuration` 配置。GameState 保存唯一的结束时间和基类已有的阶段复制状态。PlayerState 继续拥有 ASC/Inventory，本小步没有增加积分字段。
- `MatchEndServerTime` 使用 `UPROPERTY(Replicated)` 与 `GetLifetimeReplicatedProps` / `DOREPLIFETIME` 注册。覆盖复制列表时必须调用 `Super`，保留引擎 MatchState 和服务器时间等复制属性。
- 剩余时间为 `Max(0, EndTime - GetServerWorldTimeSeconds())`。客户端使用引擎同步后的服务器时间估计值，不决定比赛胜负或结束；不能把它理解为所有机器严格同时或没有网络误差。
- 阶段和结束时间没有复制到达顺序保证。`HasMatchEndTime` 区分尚未收到时间与真正的零秒，避免刚收到 `InProgress` 就显示比赛结束。

## Round Spawn Assignment

2026-10-15，已实现每局一次的服务器出生点分配。地图可提供四个或更多外围 PlayerStart；两名玩家随机使用其中两个不同点，四名玩家使用四个不同点。本局死亡后继续使用各自的 StartSpot，重开后重新抽签，允许再次抽到上一局的位置。

- `CollectRoundSpawnData` 按引用返回当前非 spectator-only、非 inactive 玩家 Controller 与地图正式 PlayerStart；排除 PIE 的临时 Play From Here 点。不复制这些临时数组，不另存玩家到出生点的映射表。
- `ReadyToStartMatch` 仅验证 WaitingToStart、NumPlayers 为 1–4、NumTravellingPlayers 为零、实际参赛 Controller 数量一致、出生点数量不少于人数。条件不满足时保持等待，不在验证函数里随机或分配。
- `AssignRoundSpawnPoints` 在 `HandleMatchHasStarted` 的父类调用前执行 Fisher-Yates shuffle，然后覆盖每名玩家的 `Controller.StartSpot`。父类随后通过默认 FindPlayerStart 和 RestartPlayer 生成 Pawn。
- 现有死亡重生仍由 GameMode-owned Timer 驱动，原生 RespawnDelay 改为 3 秒。积分和独占时间余数不因死亡清零，武器与 ASC 沿用现有生命周期；不增加出生保护或安全距离评分。
- 默认 FindPlayerStart 在 StartSpot 有效时复用它，因此本局不会因其他点变空而换点。Actor 的实际生成碰撞处理仍由引擎承担，固定点身份不承诺生成坐标完全不偏移。
- Config/DefaultGame.ini 的 GameSession.MaxPlayers 设为 4；现有在线 Session 默认也是四个公共位置，包含房主。正式游戏从现有大厅进入，本步尚未关闭中途加入，未新增新玩家的局内分配流程。

Editor 中把正式出生点全部布置在外围，并确保角色 Capsule 能正常生成。本功能不根据岛屿几何推断外围位置，不修改 PlayerStart 坐标。出生点不足时不会开局；可在 ReadyToStartMatch 中查看实际人数与候选点数。

验证从现有大厅进入：两人和四人各自分配不同点，连续死亡始终回到个人点，约 3 秒恢复，重开后再次分配。观察 `Round spawn: <name> -> <PlayerStart>` 日志；初始分配每局每名玩家一条。直接从 GameLevel 启动多个 PIE 窗口可能逐个登录并让首名玩家提前开局，不能代替原房间成员共同旅行的路径。

本步保留地图和 Blueprint 二进制资产在 Git 之外；构建与配置证据放在 Saved/RoundSpawns，不新增测试代码或 smoke-test scripts。

2026-10-15，UE 5.8.2 Editor/Game Development 构建均通过，日志为 `Saved/RoundSpawns/Build-Editor.log` 与 `Build-Game.log`。BP_ShooterGameMode 编译、保存及独立新进程重新加载编译通过，确认实际 RespawnDelay 为 3 秒；记录为 `assets-configured.json`、`Assets-configure.log`、`Assets-fresh-load-complete.log`，原资产备份在同目录 `Before/`。全局唯一 warning 为引擎 MCP 插件的 EULA 提示。

只读加载当前 GameLevel 确认有两个正式 PlayerStart（Actor 名称 PlayerStart_0 / PlayerStart_1），未改动或保存地图。当前布局可验证双人局；三至四人局需要开发者先补足外围出生点，否则按规则保持 WaitingToStart。以上是构建、配置和资产检查，新的多人分配、死亡固定点与重开随机结果仍待开发者运行验证。

## HUD and Assets

- `UShooterMatchWidget` 在 `NativeConstruct` 立即刷新，在 `NativeTick` 读取当前 GameState；不保存可能随旅行失效的 GameState 缓存，不额外绑定 Delegate。
- WBP Designer 拥有布局，C++ 只更新文字。`BindWidget` 控件名为 `MatchPhaseText`、`MatchTimeText`、`ControlPointText`、`PlayerScoreText`，重命名时需要同时修改 C++ 契约。
- HUD 只为本地 Controller 创建 Widget，独立于准星与血量 Widget 的配置；在 `EndPlay` 移除并释放自身持有的实例。
- Widget 资产：`/Game/ShooterGameContent/HUD/WBP_MatchStatus`，父类 `UShooterMatchWidget`。
- `/Game/ShooterGameContent/HUD/BP_ShooterHUD.MatchWidgetClass` 指向该 WBP。
- `/Game/ShooterGameContent/GameMode/BP_ShooterGameMode` 使用 `AShooterGameState`，`MatchDuration = 180`，`bDelayedStart = false`，HUDClass 指向现有 `BP_ShooterHUD`。
- 资产按项目规则不进入 Git；代码之外需要单独保留 WBP 与两个 Blueprint 的 class defaults 修改。

## Validation

2026-10-13，UE 5.8.2 Editor / Game Development 构建均通过，日志为 `Saved/MatchSkeleton/Build-Editor.log` 与 `Build-Game.log`。WBP、HUD 和 GameMode Blueprint 编译、保存后，在新进程中重新加载通过，0 errors；唯一 warning 来自引擎 ModelContextProtocol 插件的 EULA 提示，非项目 Blueprint 错误。证据为 `Assets-fresh-load.log` 与 `assets-authored.json`。

多人 Gameplay 尚待开发者运行验证：主机和客户端显示比赛阶段与倒计时，默认 180 秒后都显示 `Match ended / 00:00`。可临时把 Blueprint MatchDuration 调成 10 秒检查阶段转换，完成后恢复 180 秒。构建和资产加载结果不代表多人运行或完整大厅旅行已通过。
