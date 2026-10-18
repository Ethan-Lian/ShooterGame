# Pawn 生命周期

所有权划分见 [ADR-0001](../adr/0001-pawn-lifecycle-ownership.md)。

## 初始化

`BeginPlay`、`OnRep_PlayerState`、Controller 变更都进入 PawnExtension 的同一套检查。

```text
Spawned → PlayerStateReady → ASCReady → ComponentBindingsReady → GameplayReady
```

1. 等 PlayerState 和 ASC 可用；绑定对象变了，先解绑旧关系。
2. 解绑旧 Avatar，再用当前 Pawn 初始化 ActorInfo。Startup Ability / Effect 只由服务器授予一次。
3. Health、Movement 绑定 ASC，订阅 `State.Dead`。
4. 进入 GameplayReady，按当前 Tag 恢复存活或死亡表现，再刷新装备。

重复初始化不重复绑定。Controller 变更会刷新 ActorInfo；Controller 清空则解绑。

## 解绑

`UnPossessed`、Controller 清空、`EndPlay` 都进入 `UninitializePawn`，允许重复调用。

- 只有 `ASC->GetAvatarActor() == Pawn` 时，才取消激活中的 Ability、清输入和 GameplayCue。
- 清理当前 Pawn 的 Combat、Equipment、Movement、Health 和 Death Tag 订阅。
- 清 Avatar 前再次确认归属：Owner 有效时 `SetAvatarActor(nullptr)`，否则 `ClearActorInfo()`。
- 清除本地绑定，回到 Spawned。当前所有激活 Ability 都按 Pawn 生命周期处理。

## 死亡与重生

```text
Health <= 0 → GameplayEvent.Death → GA_Death（ServerOnly）
  → 应用 State.Dead → 广播 Message.Player.Death
```

`State.Dead` 是死亡状态，输入、碰撞和 Montage 跟随它更新。消息之后的重生流程见 [比赛生命周期](match-lifecycle.md)，装备处理见 [武器与战斗](weapon-combat.md)。

## 代码入口

- [ShooterPawnExtensionComponent.cpp](../../Source/ShooterGame/Private/Components/ShooterPawnExtensionComponent.cpp)：初始化、绑定和解绑。
- [PlayerCharacter.cpp](../../Source/ShooterGame/Private/Character/PlayerCharacter.cpp)：引擎回调和存活/死亡表现。
- [ShooterPlayerState.cpp](../../Source/ShooterGame/Private/PlayerState/ShooterPlayerState.cpp)：ASC 初始化和重生状态重置。
- [GA_Death.cpp](../../Source/ShooterGame/Private/AbilitySystem/Abilities/GA_Death.cpp)：死亡事件和消息。

## 踩坑/变更

- 旧 Pawn 销毁时，ASC 可能已经绑定新 Pawn；清理前必须检查 Avatar 归属。
- PlayerState 和 Controller 的复制顺序不固定，不能靠某一个回调一次性完成初始化。
