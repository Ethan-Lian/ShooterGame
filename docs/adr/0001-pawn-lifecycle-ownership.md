# ADR-0001：PlayerState 持有长期状态，PawnExtension 管绑定

已采纳 · 2026-08-10

## 背景

Pawn 会销毁重生。初始化分散在 Character 的多个回调里，容易重复绑定 Delegate，或让旧 Pawn 清掉新 Avatar。

## 决策

- `AShooterPlayerState` 持有 ASC、AttributeSet 和 Inventory；当前 Pawn 作为 ASC Avatar。
- `UShooterPawnExtensionComponent` 统一编排 Pawn 的初始化和解绑，不持有这些数据，也不决定重生规则。

## 代价

长期状态和当前身体的绑定要分开清理，初始化与解绑都要幂等。后续若要让 Ability 跨 Avatar 保留，需要另定契约。

运行流程见 [Pawn 生命周期](../systems/pawn-lifecycle.md)。
