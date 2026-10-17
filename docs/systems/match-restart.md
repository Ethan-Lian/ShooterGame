# Match Restart Foundation

## Status and developer-owned functions

Issue #3 第四步。基础框架于 2026-10-14 接入；重开与加载等待两个核心函数已由开发者补齐，2026-10-15 开发者反馈测试完成。未提供独立测试日志或设备、人数与 Steam 拓扑，不据此推断全部联机验收通过。

| 函数 | 实现契约 |
| --- | --- |
| `AShooterGameMode::TryRestartMatch` | 请求者必须是当前 World 中具有 Authority 的本地玩家 Controller；当前阶段必须为 WaitingPostMatch，GameState 结果必须 finalized。调用 RestartGame 后以阶段是否进入 LeavingMap 返回是否开始旅行。不得提前清除当前积分、结果或恢复旧 Pawn。 |
| `AShooterGameMode::ReadyToStartMatch_Implementation` | 仅 WaitingToStart、一至四名 human players、NumTravellingPlayers 为零、实际参赛 Controller 数量一致且正式 PlayerStart 足够时返回 true。父类 StartMatch 负责出生，不以 Pawn readiness 作为开局前置条件。 |

第五步的每局出生点分配位于父类生成 Pawn 之前，见 [比赛生命周期](match-lifecycle.md)。

## Ownership and flow

```text
ResultsWidget: host click
  -> disable button and display loading before calling outward
  -> ShooterPlayerController.RequestPlayAgain
       -> require authority + local player controller
       -> ShooterGameMode.TryRestartMatch             [developer]
            -> validate host / phase / finalized results
            -> ShooterGameMode.RestartGame
                 -> bUseSeamlessTravel = true on this instance
                 -> Super::RestartGame
                      -> ServerTravel("?Restart", relative)
                      -> LeavingMap

new gameplay World
  -> UE seamless-player initialization and HUD replacement
  -> WaitingToStart
  -> ReadyToStartMatch                                 [developer]
  -> StartMatch
  -> existing HandleMatchHasStarted: spawn, deadline, reset results/score, scoring
```

- 当前范围为 Listen Server 与单个本地玩家。房主的本地 PlayerController 同时在服务器上，因此直接调用 GameMode；未新增只为房主操作服务的 Server RPC。服务器上的远端 Controller 也有 Authority，不能仅用 Authority 判定房主。
- Widget 只表达意图。PlayerController 拒绝客户端本地请求，GameMode 再校验身份和阶段。`TryRestartMatch` 的 true 只表示进入 LeavingMap，不表示下一 World 已加载或新一局已开始。
- 正常旅行立即切换 LeavingMap，后续请求不再满足 WaitingPostMatch。UI 同时禁用按钮；不新增另一份复制的 restarting 状态或通用比赛管理子系统。
- 请求未受理时保留旧结果，并恢复符合当前条件的按钮；Widget 显示失败提示。旅行过程的 Network/Travel failure 仍由已有 Session Flow 的恢复链路处理。
- ResultsWidget 继续由 MatchWidget 的既有轮询刷新。按钮状态刷新位于结果行缓存的 early return 之前，不能因为结果行未变化而忽略阶段变化。
- WaitingPostMatch 与 LeavingMap 都保持结果界面和 UIOnly；LeavingMap 显示 loading 并禁用重开，避免旅行前重新显示准星和恢复游戏输入。
- 按钮 OnClicked 在 NativeConstruct 中 AddUniqueDynamic，在 NativeDestruct 中解除绑定。按钮只向本地房主显示，结果同步完成后才启用。

## Reset and travel lifecycle

UE 5.8.2 的 `AGameMode::RestartGame` 使用同地图 ServerTravel。项目 override 在调用父类前设置当前实例的 bUseSeamlessTravel，避免 Blueprint class defaults 选择 hard travel。

GameInstance/在线 Session 保留，未重新创建房间。默认不额外保留 Pawn、武器、据点、GameMode 或 GameState。当前 AGameMode seamless-player 路径会重建 PlayerState，并复制昵称、PlayerId 与 UniqueId 等资料；项目没有复制 ASC、Inventory 或 ControlTimeRemainder，因此新 PlayerState 的战斗状态按已有 startup 链路初始化。死亡重生时仍保留同一个 PlayerState，这是不同的生命周期。

新局已有 `HandleMatchHasStarted` 明确重置 Score、ControlTimeRemainder 和结果，再刷新据点。旧 GameMode 的结束/EndPlay 清理 Timer；旧据点解除 Overlap 与 Pawn EndPlay 订阅。未增加在旧 World 手动复活所有 Pawn 的流程。

同类 PlayerController 可以跨旅行保留，其 BeginPlay 不保证再次调用。旧 HUD 的 EndPlay 只平衡自己拥有的 ignore-input 请求并移除 Widget；新 HUD 的 BeginPlay 显式设 GameOnly、隐藏鼠标，且必须在 Widget 构造前执行，让已经结束的比赛仍能立即切回结果输入。未使用 ResetIgnoreMoveInput/ResetIgnoreLookInput 清空其他所有者的请求。

ReadyToStartMatch 使用 UE 的 NumPlayers / NumTravellingPlayers。引擎在 seamless-player 初始化和离开时更新这些计数，并在旅行开始时清空远端连接的 ClientWorldPackageName，不能把同地图名称视为客户端已经完成本次旅行。此门槛表示引擎确认地图已加载，不承诺所有客户端首帧已渲染或资产已完全流送。

## Assets and remaining scope

继续使用现有 `/Game/ShooterGameContent/HUD/WBP_MatchResults.PlayAgainButton` 与 HostStatusText，无新增 BindWidget 名称或 Blueprint 布局要求。C++ 负责按钮绑定与运行时启用，不需要另在 WBP 中添加重开逻辑。BP_ShooterGameMode 不应覆写 ReadyToStartMatch 绕过 C++ 判断。

第五步已接入每局独占出生点分配与 3 秒重生；外围位置由地图中的正式 PlayerStart 布置决定。关闭新玩家加入仍未实现，Issue #3 尚未全部完成。Dedicated Server、房主迁移、断线重连与跨局持久化不在本步范围内。

## Verification

2026-10-14，基础框架通过 UE 5.8.2 Editor/Game Development 构建，日志为 `Saved/MatchRestartScaffold/Build-Editor.log` 与 `Build-Game.log`。使用引擎 CompileAllBlueprints commandlet 定向加载、编译 BP_ShooterGameMode、BP_ShooterHUD、WBP_MatchStatus、WBP_MatchResults 和 WBP_MatchResultRow，5 个资产均成功，Blueprint 编译 0 errors / 0 warnings / 0 failed loads；全局唯一 warning 为引擎 MCP 插件的 EULA 提示。证据为同目录 `Blueprint-Compile.log`，未保存或修改这些二进制资产。

上述构建是基础框架阶段的记录；开发者后续补齐核心函数并反馈测试完成。新增出生分配仍需分别运行验证。未新增测试代码或 smoke-test scripts。

开发者填好两个函数后验证：

- 首次大厅进入和重开都等待原成员加载；加载期间无计分。
- 主机有按钮，客户端只有等待提示；结果未 finalized 时不能重开。
- 连续点击只开始一次旅行；连续至少两次重开后，比分、余数和结果清零，无重复 UI 或旧 Timer。
- 上一局有人死亡、持续开火、正在换弹或等待重生，下一局仍正常出生、持枪和操作。
- 鼠标隐藏、视角/移动恢复；退出一名客户端不让剩余玩家永久等待。
- 多进程/跨设备 Listen Server 与实际 Steam 房间分别验证；PIE 默认可能回退为 hard travel，不把该路径的成功当成 Seamless Travel 通过。
