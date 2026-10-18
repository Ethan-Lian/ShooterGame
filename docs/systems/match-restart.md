# 比赛重开

## 流程

```text
PlayAgainButton → PlayerController.RequestPlayAgain → GameMode.TryRestartMatch
  → RestartGame → Seamless ServerTravel("?Restart") → LeavingMap
  → 新地图等待开局
```

- 只接受当前 World 的服务器本地房主 Controller，且必须 WaitingPostMatch、结果已 finalized。
- Authority 不能单独判断房主，服务器上的远端 Controller 也有 Authority，还要检查 IsLocalPlayerController。
- RestartGame 强制 bUseSeamlessTravel；进入 LeavingMap 后拒绝重复请求，UI 同时禁用按钮。
- TryRestartMatch 返回 true 只表示进入 LeavingMap，不代表新地图加载完成。
- 新一局的加载门槛、出生和清零顺序见 [比赛生命周期](match-lifecycle.md)。

## 对象生命周期

| 对象 | 重开时 |
| --- | --- |
| GameInstance / Session | 保留，不重建房间 |
| PlayerController | 可保留，不能依赖 BeginPlay 再执行 |
| PlayerState | 替换；引擎先 Reset，再 CopyProperties，项目不额外复制跨局状态 |
| ASC / Inventory | 不复制到新 PlayerState，重新初始化 |
| GameMode / GameState / Pawn / 武器 | 随新 World 重建 |
| HUD / Widget | 替换，旧实例清理自己的输入与引用 |

旧 GameMode 在 EndPlay 清 Timer，不在旧 World 手动复活 Pawn。加入许可见 [联机流程](multiplayer-session.md)。

## 代码入口

- [ShooterPlayerController.cpp](../../Source/ShooterGame/Private/Controller/ShooterPlayerController.cpp)：重开请求和本地输入清理。
- [ShooterGameMode.cpp](../../Source/ShooterGame/Private/GameMode/ShooterGameMode.cpp)：房主校验和 RestartGame。
- [ShooterMatchResultsWidget.cpp](../../Source/ShooterGame/Private/HUD/ShooterMatchResultsWidget.cpp)：按钮显示与可用状态。

## 踩坑/变更

- 跨旅行可能保留 Controller；旧 HUD 清理不能覆盖新 World 的输入模式。
- 不在重开请求里提前清分，结算界面继续保留上一局结果。
- CopyProperties 包含 Score，但无缝旅行处理会先 Reset 清零；不能据此认为上一局积分会保留。
