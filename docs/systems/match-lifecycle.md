# 比赛生命周期

## 阶段与计时

GameMode 决定规则，GameState 复制阶段和截止时间，沿用 UE MatchState。

```text
WaitingToStart → InProgress → WaitingPostMatch → LeavingMap
```

- `ReadyToStartMatch` 要求处于 WaitingToStart、1–4 人、`NumTravellingPlayers == 0`，实际参赛 Controller 数与 NumPlayers 一致，正式 PlayerStart 不少于人数。
- `HandleMatchHasStarted` 先分配出生点，再调用父类出生流程；随后设置截止时间，清零积分、据点余数和结果，启动计分。
- 默认一局 180 秒，截止时间为服务器当前时间加 MatchDuration。到期后 `ReadyToEndMatch` 触发 [结算](match-results.md)。
- 剩余时间为 `Max(0, MatchEndServerTime - GetServerWorldTimeSeconds())`，客户端只显示，不决定结束。
- `IsMatchGameplayAllowed` 要求 InProgress、截止时间有效且未到期；开火、换弹、Sprint 和重生都走这个条件。

开始前关闭加入见 [联机流程](multiplayer-session.md)，下一局的对象重建见 [比赛重开](match-restart.md)。

## 出生与重生

- 每局开始前收集非 spectator-only、非 inactive 玩家和正式 PlayerStart，排除 PIE 的 Play From Here 点。
- Fisher-Yates 打乱出生点，每人分配不同的 `Controller.StartSpot`；本局死亡复用，重开重新抽。
- 死亡后等 3 秒，UnPossess 并销毁旧 Pawn，重置战斗状态，再 RestartPlayer。身体绑定见 [Pawn 生命周期](pawn-lifecycle.md)。
- GameMode 按 weak Controller 保存重生 Timer；替换请求、Logout、EndPlay 和比赛结束时清理，回调再检查动作许可与当前 Pawn。
- 地图出生点不足就继续等待；固定出生点不保证生成碰撞调整后的坐标完全相同。

## HUD

`UShooterMatchWidget` 初始化时刷新，此后每帧读取当前 GameState，不缓存跨 World 引用。
阶段和截止时间可能分开到达，`HasMatchEndTime` 为 false 时显示同步中。

布局在 `WBP_MatchStatus`，C++ 更新数据；结算界面见 [比赛结算](match-results.md)。

## 代码入口

- [ShooterGameMode.cpp](../../Source/ShooterGame/Private/GameMode/ShooterGameMode.cpp)：开局条件、出生分配、计时和重生。
- [ShooterGameState.cpp](../../Source/ShooterGame/Private/GameState/ShooterGameState.cpp)：复制状态和动作许可。
- [ShooterMatchWidget.cpp](../../Source/ShooterGame/Private/HUD/ShooterMatchWidget.cpp)：比赛 HUD。

## 踩坑/变更

- 开局条件只检查，不在 `ReadyToStartMatch` 里随机或分配出生点。
- 无缝旅行会清空远端连接的 ClientWorldPackageName，加载等待用引擎旅行人数，不用地图名。
- 从游戏地图直接启动多窗口 PIE 可能逐个登录、提前开局；原成员一起进局走大厅旅行。
