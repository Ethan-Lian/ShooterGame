# 比赛结算

## 结束顺序

`EndMatch` 只在 InProgress 执行一次。

```text
结清据点时间 → BuildMatchResults → Super::EndMatch（WaitingPostMatch）
  → StopMatchGameplay → GameState.SetMatchResults
```

停止 Gameplay 时清重生 Timer、ASC 输入和激活 Ability，结束换弹与冲刺，停止并禁用移动。
动作截止检查和 Timer 归属见 [比赛生命周期](match-lifecycle.md)，计分规则见 [据点计分](control-point-scoring.md)。

## 结果

- 从 PlayerArray 收集参赛者，排除 inactive 和 spectator-only。
- `FShooterMatchResults` 保存 PlayerId、昵称、整数分和胜者标记，只保存值，不引用 PlayerState。
- 最高分全部获胜，包括全员零分；按分数降序展示，同分按 PlayerId 排序，不打破并列。
- 结果生成后冻结，玩家随后离开不删行。比赛中的积分仍读 Score，结果快照不参与计分。
- 服务器只发布一次 finalized 结果；空列表也能 finalized，新一局重置。

## UI

- WaitingPostMatch 或 LeavingMap 时隐藏 MatchPanel、显示 ResultsWidget；外层 MatchWidget 保持可见，继续刷新。
- MatchState 和结果分开到达：未 finalized 时显示等待，结果就绪后显示列表。
- 列表数据或本地 PlayerId 变化才刷新，避免每帧重建；本地 PlayerState 晚到也会更新胜负提示。
- ShooterHUD 切换输入、准星和血量显示；EndPlay 只撤销自己添加的输入屏蔽。
- 布局在 `WBP_MatchResults` 和 `WBP_MatchResultRow`，按钮行为见 [比赛重开](match-restart.md)。

## 代码入口

- [ShooterGameMode.cpp](../../Source/ShooterGame/Private/GameMode/ShooterGameMode.cpp)：EndMatch、BuildMatchResults、StopMatchGameplay。
- [ShooterMatchResultTypes.h](../../Source/ShooterGame/Public/Gameplay/ShooterMatchResultTypes.h)：结果快照。
- [ShooterMatchResultsWidget.cpp](../../Source/ShooterGame/Private/HUD/ShooterMatchResultsWidget.cpp)：结果列表和本地提示。
- [ShooterHUD.cpp](../../Source/ShooterGame/Private/HUD/ShooterHUD.cpp)：结束表现和输入清理。

## 踩坑/变更

- 不能用空结果判断“还没同步”，合法空结果与等待状态靠 bIsFinalized 区分。
- 同分排序只影响展示顺序，不能据此只选一个胜者。
