# Match Results and UI Foundation

## Status

Issue #3 第三步。2026-10-14，已核对开发者提交 `fbdcf48`：下表三个核心入口均已补齐，包含结果生成、结束清理调用与结算 UI 切换。静态确认不等同于全部多人运行场景通过。

| 开发者入口 | 契约 |
| --- | --- |
| `AShooterGameMode::BuildMatchResults` | 从 GameState 的 PlayerArray 收集 ShooterPlayerState，排除 inactive 与 spectator-only 玩家；保存 PlayerId、昵称和整数 Score，标记所有最高分玩家为胜者，按积分降序展示。并列行使用 PlayerId 稳定排序，但不改变胜负。返回值的 bIsFinalized 为 true，即使没有参赛者。 |
| `AShooterGameMode::EndMatch` | 非 InProgress 立即返回；先结清截止前的据点时间，再生成快照；调用 Super 切换阶段；停止 Gameplay；最后发布结果。重复调用不能重新结算或覆盖结果。 |
| `UShooterMatchWidget::RefreshResultsDisplay` | WaitingPostMatch 隐藏 MatchPanel、显示 ResultsWidget，并让 ShooterHUD 切换本地输入/准星/血量显示。结果未就绪时 ShowPendingResults，就绪后 ShowResults。保持外层 MatchWidget 可见并继续轮询。 |

## Ownership and Order

```text
GameMode.EndMatch
  -> StopControlPointScoring(GetScoringServerTime())
  -> BuildMatchResults                         [developer]
  -> Super::EndMatch: WaitingPostMatch
  -> StopMatchGameplay
       -> clear owned respawn timers
       -> clear ASC ability input + cancel Pawn-scoped abilities
       -> clear combat stance/reload and sprint input
       -> stop and disable CharacterMovement
  -> GameState.SetMatchResults

MatchWidget.NativeTick / initial refresh
  -> RefreshResultsDisplay                    [developer]
       -> select gameplay/pending/final UI
       -> ShooterHUD.SetMatchEndedPresentation
       -> MatchResultsWidget.ShowResults
            -> reuse unchanged rows, create changed rows once
```

- `FShooterMatchResults` 是当前比赛的结果快照，复制在 GameState；行里只有值，不引用 PlayerState。比赛中仍只有原生 Score 是积分真相，结果不参与后续计分。
- MatchState 与结果可能分开到达。`bIsFinalized` 区分等待同步与合法空结果。UI 同时比较收到的行和本地 PlayerId，避免每帧重建；若复制字段或本地 PlayerState 稍后补齐，会刷新列表和本地胜负提示。
- `SetMatchResults` 仅允许服务器发布一次 finalized 快照。`ResetMatchResults` 在开局调用。
- `IsMatchGameplayAllowed` 同时要求 InProgress、有效截止时间以及剩余时间大于零。现有 Combat 的每发射击检查、换弹开始/完成，以及 Sprint 激活与重生请求/回调均接入此条件，拒绝截止后延迟到达的动作。
- GameMode 用 weak Controller → TimerHandle map 持有重生 Timer，替换请求、Logout、EndPlay 和结束清理都显式取消；不再使用无法集中清理的局部 Handle。
- 服务器结束清理不改变 `State.Dead`、不销毁装备，也不重置积分或控制时间余数。下一局恢复属于第四步。
- ShooterPlayerController 清理本地持有输入。ShooterHUD 的 ignore-input 请求只在阶段变化时增减一次；EndPlay 平衡自己拥有的请求，不覆盖下一 World 的输入模式。

## WBP Assets

| Asset | C++ parent / required configuration |
| --- | --- |
| `/Game/ShooterGameContent/HUD/WBP_MatchStatus` | `UShooterMatchWidget`；保留四个原 TextBlock，并绑定 LeadingScoreText、MatchPanel 和 ResultsWidget。ResultsWidget 是下面结算 WBP 的实例，铺满 Canvas，ZOrder 10，默认 Collapsed。 |
| `/Game/ShooterGameContent/HUD/WBP_MatchResults` | `UShooterMatchResultsWidget`；绑定 ResultsTitleText、ResultsSubtitleText、ResultsRows、ResultsStatusText、HostStatusText、PlayAgainButton；ResultRowWidgetClass 指向结果行 WBP。 |
| `/Game/ShooterGameContent/HUD/WBP_MatchResultRow` | `UShooterMatchResultRowWidget`；绑定 ResultRowBackground、RankText、PlayerNameText、ResultScoreText、WinnerText。 |

布局保存在 Designer；C++ 只填数据与切换显示。结果行序号表示展示顺序，不用于打破并列。胜者显示金色 WINNER，本地玩家行使用青色底，长昵称使用省略号。

第四步已接入现有重开按钮的 C++ 绑定与房主显示，核心重开和加载等待判断留给开发者补齐，见 [重开基础框架](match-restart.md)。关闭加入、外围出生与 3 秒重生仍待后续完成。资产不进入 Git，需要与代码分开保留。

## Verification

2026-10-13，UE 5.8.2 Editor / Game Development 构建均通过，日志为 `Saved/MatchResultsScaffold/Build-Editor.log` 与 `Build-Game.log`。三个 WBP 接入新父类后编译、保存成功；Editor 关闭后重新启动，三个资产重新加载、编译均成功，必需控件未绑定数均为 0，结果行 class 配置保留正确。证据为同目录的 `assets-connected.json`、`assets-fresh-load.json` 与 `Editor-reloaded.log`。这只验证框架与资产接入，不代表三个 TODO 或多人 Gameplay 已完成。

三个开发者函数补齐后，由开发者验证：唯一胜者、最高分并列、全员 0 分；最后一整秒计入且余数不进位；结束时持续开火/换弹/等待重生全部停止；各客户端结果一致；结算后玩家离开不删除已保存结果；结果晚到仍能显示；重复 EndMatch 不重复发布。

不新增测试代码或 smoke-test scripts。当前框架和参考实现不等同于完成多人运行验证。
