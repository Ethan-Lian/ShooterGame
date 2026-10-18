# 据点计分

地图放一个 `AShooterControlPoint`。服务器统计独占时间，整秒加分，争夺时暂停。

## 数据

| 位置 | 保存内容 |
| --- | --- |
| ControlPoint | Capsule 重叠候选的 weak set |
| GameMode | 当前计时玩家、上次结算时间、一个计分 Timer |
| PlayerState | 原生 Score、个人不足一秒的 ControlTimeRemainder |
| GameState | Empty / Held / Contested 和独占者快照 |

`EvaluateOccupancy` 只认当前、已 Possess、GameplayReady 且存活的 Pawn，按 PlayerState 去重。零人 Empty，一人 Held，多人 Contested。

## 结算

```text
Overlap / 死亡 / Logout / Pawn EndPlay / 重生完成 / 计分 Timer
  → RefreshControlPointScoring
  → 裁剪服务器时间到比赛截止时间
  → EvaluateOccupancy → ChangeScoringPlayer
  → 先给旧玩家结账，再切换计时玩家
  → 发布快照，安排下一整秒或截止时间的 Timer
```

- Timer 只唤醒，实际加分按服务器时间差计算，不按回调次数加分。
- `AddControlTime` 把新增时长与个人余数相加，向下取整增加 Score，余数留给下次。
- 暂停、死亡、重生不清个人余数；新一局统一重置。比赛截止时不足一秒不进位。
- 旧玩家刚死亡或离开，也要结清此前的独占时间；当前状态不能抹掉已发生的占用。
- 据点 BeginPlay 补读已有重叠；RestartPlayer 完成后再刷新，避免 Spawn Overlap 早于 Possess。
- 比赛结束前结清最后一段时间并清 Timer，结束顺序见 [比赛结算](match-results.md)。

## 碰撞

CaptureVolume 为 QueryOnly，只与 Pawn Overlap，且只收角色 Capsule。
使用独立 `ControlPoint` Object Channel（ECC_GameTraceChannel1），配置见 [DefaultEngine.ini](../../Config/DefaultEngine.ini)。武器查询见 [武器与战斗](weapon-combat.md)。

## 代码入口

- [ShooterControlPoint.cpp](../../Source/ShooterGame/Private/Gameplay/ShooterControlPoint.cpp)：候选维护和占用判断。
- [ShooterGameMode.cpp](../../Source/ShooterGame/Private/GameMode/ShooterGameMode.cpp)：计时和切换结算对象。
- [ShooterPlayerState.cpp](../../Source/ShooterGame/Private/PlayerState/ShooterPlayerState.cpp)：整秒积分与余数。

## 踩坑/变更

- 检测体积曾挡住武器 Object Type 射线；改为独立通道，单改 Visibility / Camera 为 Ignore 无效，联机射击仍待复测。
- ControlPoint 通道默认响应保留 Overlap；若改为 Ignore，必须同时配置 Pawn 的响应。
