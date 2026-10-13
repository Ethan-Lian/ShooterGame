# Control Point Scoring Framework

## Status and scope

Issue #3 第二步，2026-10-13。据点候选集合、比赛生命周期、计分 Timer、状态复制和 HUD 读取已接入，三个核心函数已由开发者补齐。开发者确认计分测试通过，随后报告据点内无法射击造成伤害、据点外正常；已据此补充检测体积与武器查询的碰撞隔离，修复后的联机运行仍待复测。

## Developer-owned functions

三个由开发者实现的函数及其契约如下：

| 顺序 | 函数 | 输入 / 输出契约 |
| --- | --- | --- |
| 1 | `AShooterControlPoint::EvaluateOccupancy` | 服务器调用。`GetOverlappingCharacters` 只提供 Capsule 几何候选；筛选当前、被 Possess、GameplayReady 且无 `State.Dead` 的 Pawn，按 PlayerState 去重。零人返回 Empty，多人返回 Contested，这两种状态 SolePlayer 为 null；唯一玩家返回 Held 与对应 PlayerState。 |
| 2 | `AShooterPlayerState::AddControlTime` | 服务器传入正的有效独占秒数，已限制在比赛截止时间以内。结合 `ControlTimeRemainder`，通过原生 `SetScore` 增加整秒对应的整数分，余数保持在 `[0, 1)`；分数改变时刷新复制。一次调用可能累计超过一秒，不得按调用次数加分或向上取整。 |
| 3 | `AShooterGameMode::ChangeScoringPlayer` | `ServerTime` 已裁剪到截止时间。先调用 `SettleControlPointScore` 给旧 ScoringPlayer 结账，再更新 ScoringPlayer 和 LastScoringServerTime。null 表示暂停；不清除个人余数。即使新旧玩家相同也需结算，因为正常的加分 Timer 同样走此入口。 |

第三个函数之后，框架自动发布 GameState 快照并安排下一次 Timer，不需要在这些函数内重复做这些工作，也不需要另加积分副本、客户端加分 RPC 或新的 Tick。

## Ownership

- `AShooterControlPoint` 保存服务器侧 Capsule 候选的 weak set；Overlap、死亡、Logout、Actor EndPlay 清理候选。Mesh 与武器不进入集合。它只提供占用判断，不写积分。
- GameMode 持有唯一据点的 weak reference、当前计时玩家、上次结算服务器时间和唯一的计分 Timer。地图只允许一个据点，重复注册会报错。
- PlayerState 复用原生复制的 Score，保存服务器侧 double 余数。`ResetMatchScore` 在开局调用；现有 `ResetCombatStateForRespawn` 不清除比赛积分。
- GameState 复制 `FShooterControlPointState`，其中包含 Empty/Held/Contested 与独占者 PlayerState。计时执行数据不复制。
- 现有 Match Widget 继续每帧读取快照，新增据点状态与本地玩家积分，不引入另一套 Delegate 生命周期。

## Call flow

```text
point BeginPlay -> collect existing Capsule overlaps -> RegisterControlPoint
match start -> Super spawn -> set deadline -> reset player scores -> RefreshControlPointScoring

Capsule begin/end overlap / Pawn EndPlay / death / Logout
  -> candidate membership changes
  -> RefreshControlPointScoring
       -> clamp server time to match deadline
       -> EvaluateOccupancy                         [developer 1]
       -> ChangeScoringPlayer                      [developer 3]
            -> SettleControlPointScore
                 -> AddControlTime                 [developer 2]
       -> publish GameState snapshot
       -> schedule one-shot Timer for next whole second or match deadline

score Timer -> RefreshControlPointScoring
RestartPlayer -> Super spawn + possession -> RefreshControlPointScoring
EndMatch -> settle through deadline -> clear Timer and scoring player -> Super
GameMode EndPlay -> clear Timer and weak references
```

Spawn overlap 可能早于 Possess/ASC 绑定，因此 `RestartPlayer` 完成后再次刷新；据点 BeginPlay 补读已有重叠，开局再读一次。死亡先移出候选再走原有重生请求，重复的 EndOverlap/死亡通知不会重新加入候选。

`SettleControlPointScore` 根据此前保存的计时玩家结算，不能因为该玩家此刻已死亡而抹掉死亡之前的独占时长。计分 Timer 只负责唤醒，实际秒数来自服务器时钟差值。`EndMatch` 在父类改变阶段前结清末段时间；截止后到达的占用事件也会停止计分。

## Editor setup

- 将原生 `ShooterControlPoint` 从 Place Actors 拖入 `/Game/ShooterGameContent/Maps/GameLevel` 的中央大岛，只放一个。本次框架不替开发者决定地图中的坐标，也不自动改动地图。
- `CaptureVolume` 原生 half extent 为 `(400, 400, 150)` cm；在 Details 调整 Box Extent、Actor 位置和 Label 朝向。Capsule 与体积发生重叠即成为几何候选，不要求角色完全进入。
- 原生青色体积线框与 `CONTROL POINT` TextRender 是基础标记，可在后续 Blueprint/地图美术中调整，不决定计分。
- `CaptureVolume` 使用 `ControlPoint` Object Channel（`ECC_GameTraceChannel1`），由 `Config/DefaultEngine.ini` 声明；保持 QueryOnly，响应仅 Pawn 为 Overlap。通道默认响应为 Overlap，让现有 Pawn Collision Profile 也响应此通道；不能改为默认 Ignore 而不同时配置 Pawn。
- 武器的相机和枪口射线按 Object Type 查询 WorldStatic/WorldDynamic/Pawn，因此检测体积不能使用这些类型。单纯把 Visibility/Camera 响应设为 Ignore 不能排除 Object Type 查询。修改通道配置后重启 Editor；已放置实例的 CaptureVolume Object Type 应显示为 ControlPoint，若曾手工覆盖则重置该属性。
- `/Game/ShooterGameContent/HUD/WBP_MatchStatus` 继续使用 `UShooterMatchWidget`。必需的 TextBlock 为 `MatchPhaseText`、`MatchTimeText`、`ControlPointText`、`PlayerScoreText`。后两项加入现有 `MatchRows`，不更改原来的倒计时控件。
- 地图和 WBP 仍按项目规则排除在 Git 之外，需要单独保留。

## Validation boundary

2026-10-13，UE 5.8.2 Editor / Game Development 构建均通过，日志为 `Saved/ControlPointScaffold/Build-Editor.log` 与 `Build-Game.log`。HUD 资产首次更新时出现旧资产缺少新绑定及新控件 GUID 的编译诊断；完成保存后，在独立进程重新加载、编译、保存通过，`Assets-HUD-reload.log` 为 0 errors。多人占用、积分与截止正确性尚未运行验证，要在三个 TODO 实现并放置据点后，由开发者验证。

开发者随后补齐三个函数并确认计分测试成功，具体人数与覆盖场景未说明，不从该结果推断全部验收通过。碰撞隔离修复后需要复测双方在据点内、从外向内、从内向外及射线穿过据点的伤害，同时检查独占加分、争夺暂停与死亡后立即转为独占。

碰撞隔离修复的 UE 5.8.2 Editor / Game Development 构建均通过，日志为 `Build-Editor-collision-fix.log` 与 `Build-Game-collision-fix.log`（均在 `Saved/ControlPointScaffold/`）。独立 Editor 进程只读加载已保存的 GameLevel，确认唯一据点实例的 Object Type 为 `ECC_CONTROL_POINT`（值 14，对应 `ECC_GameTraceChannel1`），据点对 Pawn、角色 Capsule 对 ControlPoint 均为 Overlap；未改动或保存地图。证据为 `Inspect-collision-fix-final.log`，0 errors。上述构建与配置检查不等同修复后的多人射击回归通过。

其他重点场景：空闲/独占/三人争夺；0.6 秒加暂停加 0.4 秒；不同玩家余数独立；据点内死亡和退出；新 Pawn 初始化后被正确识别；时间截止前最后不足一秒不进位、截止后不加分。不会为此添加测试代码或 smoke-test scripts。

本步不实现最终胜者列表、重开、关闭新玩家加入、加载等待、外围出生点、3 秒重生或比赛结束时停止战斗。这些仍遵循 match-lifecycle 文档记录的后续范围。
