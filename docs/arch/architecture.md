# ShooterGame 架构

UE 5.8，基于 GAS 的 Listen Server 射击项目。Steam 好友组房，进入据点争夺，结算后由房主重开。

## 模块

| 模块 | 职责 |
| --- | --- |
| `ShooterGame` | 角色、GAS 战斗、武器、比赛规则和 HUD |
| `MultiplayerSession` 插件 | 在线房间、好友邀请、地图旅行和房间 UI |

插件不处理伤害和比赛规则；Gameplay 不接 OSS 回调。

## 主流程

```text
建房 → 大厅 → 比赛 → 结算 → 同地图重开
               ↓
         开火 → 伤害 → 死亡 → 重生
```

## 系统入口

| 关注点 | 文档 |
| --- | --- |
| 玩家状态和当前 Pawn 怎么分开，为什么这样分 | [所有权决策](../adr/0001-pawn-lifecycle-ownership.md) |
| ASC 什么时候绑定，旧 Pawn 怎么清理 | [Pawn 生命周期](../systems/pawn-lifecycle.md) |
| 弹药归谁管，命中和伤害怎么走 | [武器与战斗](../systems/weapon-combat.md) |
| 什么时候开局，出生点和重生怎么安排 | [比赛生命周期](../systems/match-lifecycle.md) |
| 独占时间怎么算，争夺和死亡怎么结账 | [据点计分](../systems/control-point-scoring.md) |
| 结果怎么冻结，客户端晚收到结果怎么办 | [比赛结算](../systems/match-results.md) |
| 重开保留什么，重建什么 | [比赛重开](../systems/match-restart.md) |
| 邀请、关闭加入、旅行失败怎么处理 | [联机流程](../systems/multiplayer-session.md) |

Steam 跨设备、弱网和 Dedicated Server 还没验证。

## 文档维护

事实只写一处，其他位置引用。ADR 记决策，System 记行为。
ADR 正文不超过 20 行，System 正文不超过 60 行。没有实际内容的字段不留。
改代码时，在对应 System 的“踩坑/变更”补一行。日志和构建细节放 commit message 或 PR。
