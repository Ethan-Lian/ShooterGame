# ADR-0001: PlayerState owns ASC, PawnExtension coordinates Pawn lifecycle

## Status

Accepted — 2026-08-10

## Context

Pawn 的 ASC ActorInfo、Health、Movement、Death Tag 和 Equipment 绑定原本主要由 `APlayerCharacter` 编排。Possession、复制回调、UnPossess 和重生顺序变化时，容易出现重复 Delegate、stale Avatar，或者旧 Pawn 清理已经属于新 Pawn 的状态。

项目需要一个统一生命周期入口，但不需要完整复制 Lyra InitState 框架。与此同时，ASC、AttributeSet 和 Inventory 必须跨当前 Pawn 的死亡与重生存在。

## Decision

- `AShooterPlayerState` 持有 ASC、AttributeSet 和 Inventory。
- 当前 `APlayerCharacter` 作为 ASC Avatar。
- `UShooterPawnExtensionComponent` 协调当前 Pawn 的幂等初始化和解绑，但不拥有 ASC、Inventory 或 Equipment Actor，也不复制新的 Gameplay 真相。
- `BeginPlay`、`OnRep_PlayerState` 和 `NotifyControllerChanged` 只触发同一套前置条件检查。
- `UnPossessed`、Controller cleared 和 `EndPlay` 进入同一个幂等清理路径。
- GameMode 继续拥有重生规则；PawnExtension 不调用 `RestartPlayer`。

## Consequences

- PlayerState-owned 状态可以跨当前 Pawn 重生存在，Pawn-scoped 绑定和表现可以被明确清理。
- 旧 Pawn 只有仍是 ASC Avatar 时，才能取消 Ability、移除 GameplayCue 或清除 Avatar。
- Health、Movement、Death Tag、Combat 和 Equipment 的初始化顺序集中在 PawnExtension。
- 当前所有激活 Ability 在解绑时都按 Pawn-scoped 处理；引入跨 Avatar Ability 前，必须先扩展保留契约。
- 生命周期正确性依赖 PawnExtension 的清理路径保持幂等，并覆盖 Possession、复制回调、UnPossess 和 EndPlay。
