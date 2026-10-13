# Match Lifecycle

## Scope

Issue #3 第一小步：比赛阶段、180 秒计时、最小 HUD 显示。`AShooterGameMode : AGameMode` 与 `AShooterGameState : AGameState` 成对使用，沿用 UE MatchState。

当前仍按引擎默认条件自动开局，`WaitingToStart` 可能很短。等待全体原有玩家加载、关闭新玩家加入、外围出生、3 秒重生、结束停止战斗/重生、据点积分、胜者结果与房主重开属于后续工作。比赛阶段结束不等于现有 GAS 与重生逻辑自动停止。

## Ownership and Flow

```text
AGameMode checks ReadyToStartMatch
  -> StartMatch / InProgress
  -> ShooterGameMode.HandleMatchHasStarted
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

## HUD and Assets

- `UShooterMatchWidget` 在 `NativeConstruct` 立即刷新，在 `NativeTick` 读取当前 GameState；不保存可能随旅行失效的 GameState 缓存，不额外绑定 Delegate。
- WBP Designer 拥有布局，C++ 只更新文字。`BindWidget` 控件名为 `MatchPhaseText` 和 `MatchTimeText`，重命名时需要同时修改 C++ 契约。
- HUD 只为本地 Controller 创建 Widget，独立于准星与血量 Widget 的配置；在 `EndPlay` 移除并释放自身持有的实例。
- Widget 资产：`/Game/ShooterGameContent/HUD/WBP_MatchStatus`，父类 `UShooterMatchWidget`。
- `/Game/ShooterGameContent/HUD/BP_ShooterHUD.MatchWidgetClass` 指向该 WBP。
- `/Game/ShooterGameContent/GameMode/BP_ShooterGameMode` 使用 `AShooterGameState`，`MatchDuration = 180`，`bDelayedStart = false`，HUDClass 指向现有 `BP_ShooterHUD`。
- 资产按项目规则不进入 Git；代码之外需要单独保留 WBP 与两个 Blueprint 的 class defaults 修改。

## Validation

2026-10-13，UE 5.8.2 Editor / Game Development 构建均通过，日志为 `Saved/MatchSkeleton/Build-Editor.log` 与 `Build-Game.log`。WBP、HUD 和 GameMode Blueprint 编译、保存后，在新进程中重新加载通过，0 errors；唯一 warning 来自引擎 ModelContextProtocol 插件的 EULA 提示，非项目 Blueprint 错误。证据为 `Assets-fresh-load.log` 与 `assets-authored.json`。

多人 Gameplay 尚待开发者运行验证：主机和客户端显示比赛阶段与倒计时，默认 180 秒后都显示 `Match ended / 00:00`。可临时把 Blueprint MatchDuration 调成 10 秒检查阶段转换，完成后恢复 180 秒。构建和资产加载结果不代表多人运行或完整大厅旅行已通过。
