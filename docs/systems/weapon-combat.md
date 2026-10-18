# 武器与战斗

当前只有一把固定 Hitscan 武器 `DA_Weapon_AK47`，没有拾取、切枪、丢弃或掉落。

## 数据与表现

- Inventory 条目保存 ItemId、WeaponDefinition 和弹匣弹药，只有服务器修改，OwnerOnly 复制。
- Equipment 用 ItemId 查询条目，负责武器 Actor 的生成、附着和销毁；Actor 只保存武器定义和表现，不存弹药。
- Pawn GameplayReady 且存活时，服务器授予并装备默认武器；重复刷新沿用已有条目和 Actor。
- 死亡删除条目并销毁武器，不生成掉落；新 Pawn 重新授予默认弹药。
- 第一人称相机、双臂和瞄准表现由 Character 与 AnimInstance 驱动，不另存 Gameplay 数据。

## 开火与换弹

```text
输入 → Controller → Combat → ASC → GA_FireWeapon（ServerOnly）
  → 校验当前装备和战斗状态 → Inventory 扣一发
  → 相机射线确定瞄点 → 枪口射线判断实际命中
  → Damage GE → DamageExecutionCalculation → Health
```

开火和换弹在服务器执行，没有客户端预测。射线按 Object Type 查询 WorldStatic、WorldDynamic 和 Pawn。

Combat 管换弹 Timer，完成后由 Inventory 补满弹匣，备弹无限。每发射击、换弹开始和完成都检查 [比赛动作许可](match-lifecycle.md)。
死亡后的事件链见 [Pawn 生命周期](pawn-lifecycle.md)。

## 代码入口

- [ShooterInventoryComponent.cpp](../../Source/ShooterGame/Private/Components/ShooterInventoryComponent.cpp)：条目和弹药。
- [ShooterWeaponEquipmentComponent.cpp](../../Source/ShooterGame/Private/Components/ShooterWeaponEquipmentComponent.cpp)：装备和复制后的附着。
- [ShooterCombatComponent.cpp](../../Source/ShooterGame/Private/Components/ShooterCombatComponent.cpp)：战斗请求和换弹。
- [GA_FireWeapon.cpp](../../Source/ShooterGame/Private/AbilitySystem/Abilities/GA_FireWeapon.cpp)：连续开火、射线和伤害。

## 踩坑/变更

- 武器定义、装备指针和 Inventory 复制到达顺序不固定，依赖齐了再刷新附着。
- 客户端解绑只清本地表现，不能清掉服务器复制的装备指针和 ItemId。
- `FinishSpawning` 会执行 Gameplay 回调，调用前先记录装备归属，返回后再确认仍是本次装备。
