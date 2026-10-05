# Pawn Lifecycle

## Why

同一个玩家可以经历 Possession、客户端 PlayerState/Controller 复制、UnPossess、死亡、Pawn 销毁和重生。ASC 与 Inventory 位于 PlayerState，而 Health、Movement、Combat、Equipment 和表现位于当前 Pawn；如果每个引擎回调分别初始化这些关系，就容易产生重复 Delegate、旧 Avatar 或错误清理新 Pawn。

`UShooterPawnExtensionComponent` 的职责是协调当前 Pawn 的绑定顺序和幂等清理。它不是新的状态所有者，也不替代 PlayerState、ASC 或各 Gameplay Component。

## Ownership and Lifetime

```text
AShooterPlayerState                         current APlayerCharacter
  ├── UShooterAbilitySystemComponent          ├── UShooterPawnExtensionComponent
  ├── UCombatAttributeSet                     ├── UShooterHealthComponent
  ├── UMovementAttributeSet                   ├── UShooterCombatComponent
  └── UShooterInventoryComponent              ├── UShooterMovementStateComponent
                                               └── UShooterWeaponEquipmentComponent
```

- ASC Owner：`AShooterPlayerState`
- ASC Avatar：当前 `APlayerCharacter`
- Inventory Owner：`AShooterPlayerState`
- Equipment、Health、Movement、Combat 和 PawnExtension：当前 `APlayerCharacter`

PlayerState-owned 状态跨当前 Pawn 重生存在；Pawn-owned 绑定和表现必须在旧 Pawn 结束前清理。死亡规则可以主动从 Inventory 删除武器，因此长期所有权不代表数据永远保留。

## Authority and Replication

- 服务器 Possess Pawn、授予 Startup Ability/Effect、修改 Inventory、创建 Equipment Actor、处理死亡并调用 `RestartPlayer`。
- 客户端通过 `OnRep_PlayerState` 和 Controller 变更得知依赖已经可用，再运行相同的幂等初始化检查。
- PawnExtension 可以在服务器和客户端建立本地 Delegate/表现绑定，但不写入一份新的复制状态。
- `State.Dead` 由 ServerOnly Death Ability 应用；客户端监听复制后的 Tag 并更新当前 Pawn 表现。
- 客户端 Equipment 刷新只能消费 `EquippedWeapon`、OwnerOnly `EquippedItemId` 和 Inventory 复制，不得 Spawn 权威 Equipment Actor。

## Runtime Flow

### Initialization

```text
BeginPlay / OnRep_PlayerState / NotifyControllerChanged
  -> CheckDefaultInitialization
  -> resolve ShooterPlayerState + ASC
  -> if bound PlayerState/ASC changed, uninitialize old binding
  -> PlayerStateReady
  -> detach a stale old Avatar when necessary
  -> PlayerState.InitializeAbilitySystem(CurrentPawn)
  -> ASCReady
  -> initialize Health and Movement with ASC
  -> bind State.Dead delegate
  -> ComponentBindingsReady
  -> GameplayReady
  -> apply presentation from current State.Dead value
  -> refresh Equipment: authority grants fixed weapon, clients refresh presentation
```

状态顺序是：

```text
Spawned -> PlayerStateReady -> ASCReady -> ComponentBindingsReady -> GameplayReady
```

这些状态只表达初始化前置条件，不替代组件内部状态。重复调用到达 `GameplayReady` 后，只执行需要幂等刷新的 Equipment 和 dead/alive presentation。

### Controller changes

- `PossessedBy` 调用 `Super` 后，由 UE 的 Controller 变更路径进入 `NotifyControllerChanged`。
- Controller 有效时，PawnExtension 刷新 ASC ActorInfo 并重新检查依赖。
- Controller 清空时，进入统一解绑。
- `OnRep_Controller` 不额外重复转发初始化逻辑。

### Uninitialize

```text
UnPossessed / Controller cleared / EndPlay
  -> if this Pawn is still ASC Avatar:
       cancel active Pawn-scoped abilities
       clear ability input
       remove gameplay cues
  -> uninitialize Combat and Equipment
  -> unbind Movement and Health from ASC
  -> remove State.Dead delegate
  -> if ASC Avatar is still this Pawn:
       Owner valid   -> SetAvatarActor(nullptr)
       Owner invalid -> ClearActorInfo()
  -> clear weak bindings
  -> Spawned
```

清理函数允许重复调用。最重要的保护条件是 `ASC->GetAvatarActor() == Pawn`：旧 Pawn 不能清除已经绑定到新 Pawn 的 Avatar。

### Death and respawn

```text
Damage Effect
  -> Health <= 0 on authority
  -> GameplayEvent.Death
  -> GA_Death (ServerOnly)
  -> State.Dead + Message.Player.Death
  -> Pawn applies death presentation from Tag
  -> GameMode respawn timer
  -> UnPossess + destroy old Pawn
  -> PlayerState.ResetCombatStateForRespawn
  -> RestartPlayer
  -> new Pawn repeats initialization with same PlayerState-owned ASC
```

GameMode 决定重生时机；PawnExtension 只保证旧 Pawn 清理和新 Pawn 绑定正确。

## Invariants

1. 项目完成 ActorInfo 绑定后，ASC Owner 是有效的 `AShooterPlayerState`，Avatar 是当前 Pawn；解绑后可以为 `nullptr`。引擎组件初始阶段可能先以 Owner 同时作为 Avatar，不能把这个过渡状态误作完成绑定。
2. 同一个 PlayerState/ASC/Pawn 组合只能有一套 Health、Movement 和 Death Tag 绑定。
3. Startup Ability/Effect 的长期授予状态位于 PlayerState，不因新 Pawn 重复授予。
4. 旧 Pawn 只有仍是 ASC Avatar 时才能取消 Ability、移除 Cue 或清除 Avatar。
5. Inventory 属于 PlayerState；Equipment Actor 和 transient WeaponInstance 属于当前 Pawn。
6. 只有服务器可以创建/销毁权威 Equipment Actor 和修改 Inventory。
7. `State.Dead` 是死亡事实，Character 的移动、碰撞、输入和 Montage 是表现。

## Failure Modes

| 失败模式 | 防护 |
|---|---|
| `OnRep_PlayerState` 和 Controller 回调顺序变化 | 所有入口调用同一套幂等前置条件检查 |
| 旧 Pawn 销毁时 ASC 已绑定新 Pawn | 清理前验证 `GetAvatarActor() == OldPawn` |
| 重复注册 Death Tag Delegate | 保存 `FDelegateHandle`，绑定前先解绑 |
| Movement/Health 继续引用旧 ASC | Pawn 解绑时显式调用各组件的 Uninitialize |
| 客户端复制回调重复创建武器 | 客户端只刷新表现；服务器才 Spawn Actor |
| ASC Owner 已失效仍调用 `SetAvatarActor` | Owner 有效时 SetAvatarActor，否则 ClearActorInfo |
| 新 Pawn 继承旧 Pawn 激活中的短期 Ability | 解绑时取消当前激活 Ability 并清空输入/Cue |
| 未来需要跨 Avatar Ability | 当前尚无保留契约，必须先设计标签/分类再引入 |

## Code Entry Points

| 关注点 | 入口 |
|---|---|
| 生命周期状态和公开入口 | `Source/ShooterGame/Public/Components/ShooterPawnExtensionComponent.h` |
| 状态推进、绑定与清理 | `Source/ShooterGame/Private/Components/ShooterPawnExtensionComponent.cpp` |
| 引擎生命周期回调和 Pawn 表现 | `Source/ShooterGame/Private/Character/PlayerCharacter.cpp` |
| ASC 初始化与重生状态重置 | `Source/ShooterGame/Private/PlayerState/ShooterPlayerState.cpp` |
| 死亡消息和重生规则 | `Source/ShooterGame/Private/GameMode/ShooterGameMode.cpp` |
| Equipment 重建/解绑 | `Source/ShooterGame/Private/Components/ShooterWeaponEquipmentComponent.cpp` |

## 验证状态

| 日期 / 场景 | 结果 | 范围与证据 |
|---|---|---|
| 2026-10-04，UE 5.7.4 Editor/Game Development 构建 | PASS | 固定武器最终版本构建成功；`Saved/Validation/20261004-fixed-weapon/Build-Editor-final.log` 与 `Build-Game-final.log` |
| 2026-10-05，阶段 3，UE 5.7.4 Editor/Game Development 构建 | PASS | 旧玩法删除后的构建；`Saved/Stage3/Build-Editor-final.log`、`Saved/Stage3/Build-Game-final.log` |
| 2026-10-05，阶段 3，资源迁移与全新进程加载 | PASS | 17 个相关 Blueprint 重新编译并保存，5 张项目地图加载；清除废弃资源依赖。最终全新进程检查为 0 errors / 0 warnings；`Saved/Stage3/Assets-finalize-final.log`、`Saved/Stage3/Assets-fresh-load.log` 与 `assets-fresh-load.json`。这些检查不代表 Gameplay 运行通过 |
| 2026-10-05，精简阶段最终双人 Gameplay 回归 | 开发者确认通过 | 按下方四项清单测试后反馈“都测试成功了”；覆盖双方持枪、双向伤害/击杀、各至少两次重生恢复、死亡不掉落及无重复武器或残留表现 |
| Steam 好友跨设备、三人及以上观察、弱网、Dedicated Server、完整 Seamless Travel | 待分别验证 | 本次双人 Gameplay 反馈未单独说明这些场景 |

### 2026-10-05：精简阶段双人 Gameplay 回归

- **日期与版本**：2026-10-05；记录时分支为 `refactor/1-restart-minimal-gameplay`，HEAD 为 `961e222`。项目使用 UE 5.7，已有构建记录为 UE 5.7.4；本次手动测试未单独说明引擎补丁版本。
- **拓扑与人数**：主机与客户端，两人；具体入口（PIE、本地 IP 或 Steam）及是否跨设备未说明。

开发者按以下步骤测试，并确认全部成功：

1. 主机和客户端出生自动持枪，双方能看到对方武器。
2. 双方分别开火、造成伤害并击杀对方。
3. 双方各至少重生两次，移动、瞄准、开火与血量恢复。
4. 死亡不掉落，重生后没有重复武器或残留表现。

- **证据**：本次对话中的开发者手动测试反馈，未提供独立日志、截图或录像路径；不是助手执行的运行测试。
- **结论与边界**：精简阶段的双人 Gameplay 回归完成。此结果不证明 Steam 建房/邀请/加入、完整 Seamless Travel、多客户端第三方观察、弱网或 Dedicated Server 已通过；也不覆盖弹药扣减与射击预测。

### 固定武器契约

- Equipment 在存活 Pawn 的 ASC/组件绑定完成、Combat 死亡阻塞解除后，服务器从 `DefaultWeaponDefinition` 授予并装备一把 Hitscan 武器；原生默认值为现有 `DA_Weapon_AK47`。
- 重复初始化沿用当前装备 Actor，不重复添加库存。Inventory 仅允许一个默认武器条目，仍保留 ItemId 与 OwnerOnly 复制；不再有槽位、拾取转移或循环切换。
- 死亡销毁权威武器、移除对应库存项并清理 transient WeaponInstance，不生成掉落；重生重新初始化默认弹药数据，扣弹与预测尚未实现。
- 客户端解绑或死亡只隐藏/解绑表现与清理缓存，不主动清空服务器复制的 `EquippedWeapon`、`EquippedItemId`；恢复存活状态后刷新表现。
- 拾取、切槽位、丢弃、掉落表现与 Projectile 的代码、对应 Blueprint 和废弃输入已移除。Lobby 与 GameLevel1 中的旧拾取武器已清除，Steam Session/邀请/加入与必要 Travel 保留。
- 删除前的代码、配置、文档与 ShooterGameContent 资源基线保存在本地 `Saved/CodeBaselines/stage3-20261005-102823/`；资源和维护记录仍按仓库规则保留在 Git 跟踪之外。

### 后续验证方式

不再新增自动化 test 或 smoke 脚本；相关现有测试代码与脚本已删除。验证采用必要的 UE 构建检查和开发者游戏内手动验证。

Gameplay 改动后，在 GameLevel 检查出生自动持枪、双方移动/开火造成伤害、死亡不掉落、连续重生后的输入/Health/武器恢复，以及武器 Actor 是否累积。记录实际拓扑、人数、步骤、结果和证据路径；Steam、弱网与 Travel 使用各自的实际运行结果，不由本地验证推断通过。
