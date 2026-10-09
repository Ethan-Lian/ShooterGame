# 第一人称表现与换弹

当前实现保留 `BP_PlayerCharacter`、GAS 和固定武器装备链路。第一人称双臂使用从 `SK_BlockCharacter` 裁出的独立网格，保留原角色双臂拓扑、袖口、UV 和材质。前臂长度、掌心与手指参考几何按模板关节校准，参考绑定转换到 Infima FPS 模板骨架，直接播放模板原动画；世界角色网格与骨架保持原有配置。视角枪使用模板原枪及其独立弹匣，以保持握枪和换弹骨骼位置一致。

## 职责和数据

| 内容 | 所在位置 / 写入方 |
|---|---|
| 相机、FOV、瞄准偏移、输入摇摆、枪与镜头后坐力 | `APlayerCharacter`，本地 C++ Tick 与 GameplayCue |
| 移动、瞄准、冲刺动画状态 | `UShooterFirstPersonAnimInstance` 与原生父类 |
| 姿势播放和布尔混合、DefaultSlot | `ABP_OwnFirstPerson` AnimGraph，EventGraph 为空 |
| 手部 Montage 与枪械 Sequence 播放 | `APlayerCharacter` 的 C++ 方法 |
| 开火判定、冷却和伤害 | ServerOnly `GA_FireWeapon`，沿用原有 GAS / Hitscan |
| 弹匣与备弹真相 | PlayerState 的 Inventory，服务器修改，OwnerOnly 复制 |
| 换弹开始、计时、结束与取消 | Pawn 的 Combat Component，服务器计时并复制开始时间 |

输入配置保留原有操作，增加 `IA_Reload` → `Input.Reload` → R 键。换弹期间禁止开火和瞄准；动画时长决定换弹计时（当前约 3.13 秒），完成时一次性转移弹药。死亡、解绑和 EndPlay 取消换弹，不补充弹药。客户端按服务器开始时间播放到相应位置；没有本地预测。

## 资产

生成的资产集中在 `/Game/ShooterGameContent/FirstPerson`：

- `Meshes/SK_OwnFirstPersonArms`：原角色双臂与袖口，3298 个三角形，绑定到 `SKEL_UE5_Mannequin`。不包含身体、头、帽子、腿或背包。
- `ABP_OwnFirstPerson`：原生父类为 `ShooterFirstPersonAnimInstance`，五个原始循环动画、四个布尔混合、三个原生状态读取节点、DefaultSlot 与输出节点。
- `Animations/Own_AM_FP_AssaultRifle_Fire`、`Fire_Aimed` 和 `Reload`：使用 Infima 原始手部动画与 DefaultSlot。
- `Meshes/SK_FP_AssaultRifle`：模板原枪的本地副本，新增 `FP_Muzzle` Socket；未修改模板原枪网格。

`BP_PlayerCharacter` 配置以上资源，以及原模板 `A_FP_WEP_AssaultRifle_Fire`、`A_FP_WEP_AssaultRifle_Reload` 和 `SM_AssaultRifle_Magazine`。第一人称枪以零相对变换挂到双臂的 `ik_hand_gun`，静态弹匣以零相对变换挂到枪的 `Magazine` 骨骼。手部与枪械原始动画同步播放，运行时不做重定向，也不依赖 Python 或模板玩法蓝图。

视角偏移从模板的 `head` 骨骼与相机挂接变换求逆得到：Idle 约 `(-0.708, 0, -162.575)` cm，Aim 约 `(-0.743, 0, -162.575)` cm。相机 FOV 100，瞄准过渡到 70；第一人称深度缩放为 0.25。`CalcCamera` 为当前 Pawn 视角设置 0.25 cm 近裁剪面，避免压缩后的手和枪被默认 10 cm 近裁剪面裁掉。相机覆盖关闭 DOF，使近处视角模型保持清晰。

Owner 看见第一人称双臂和枪，其他玩家看见原世界角色和模板步枪的完整静态版本 `Meshes/SM_World_AssaultRifle`。该模型由模板枪参考姿势与独立弹匣合并，材质保持 `MI_AssaultRifle`，按扳机接触点平移 `(0, 4.200551, 7.006591)` cm 对齐旧装备挂点，再围绕 `(0, 4.932438, 5.834661)` cm 旋转 `Roll=20°`，保留右手扳机接触位置并降低前端以靠近原第三人称左掌。该变换烘焙到世界静态模型，第一人称模型和动画不受影响。现有 `DA_Weapon_AK47` 的 `WeaponMesh` 指向此资源，保留原装备类、数值与 C++ 装备链路；资源与类名暂沿用 AK47 名称。视角模型不参与碰撞或伤害追踪；视角内枪口特效挂到原枪副本的 `FP_Muzzle`，将原特效的 +Z 发射轴旋转到原枪的 +Y，设置已使用特效的 `User.Global Scale=0.08` 并关闭视角烟雾，所有参数在 Activate 前由 C++ 配置。服务器从装备的世界模型 `Muzzle` Socket 计算枪口追踪；新 Socket 位于 `(-0.000001, 51.048031, -6.387195)` cm，旋转 `Pitch=0, Yaw=0, Roll=110°`，使特效 +Z 跟随变换后的枪管方向。本次未新增第三人称换弹动画、空仓换弹或独立拉栓逻辑。

世界身体标记为 `WorldSpaceRepresentation`，按 UE 5.8 的第一人称阴影规则隐藏 Owner 身体并保留场景中的角色阴影，避免用普通 Hidden Shadow 遮暗近处视角模型。

以前生成的 `ABP_BlockFirstPerson`、`Block_A_FP_*`、AK 弹匣烘焙动画和 Retarget 资源已不用于当前视角配置，保留在本地供回溯。

二进制 Content 资产按项目现有规则不进入 Git；本地 authoring 脚本、资源备份和报告在 `Saved/FirstPerson`。交付或换机器时需要同时复制这些 Content 资产。

双臂的当前 authoring 流程为 `inspect_hand_binding.py` → `fit_hand_reference.py` → `create_calibrated_arms.py` → `finalize_hand_binding.py`。前臂按模板实际骨段长度缩放；掌心连续拟合，手指保留原网格体积；蒙皮按模板表面逐顶点转移，并分别处理左右手与各手指。不要重新执行旧 `create_own_arms.py` 覆盖当前资源：仅按骨骼名称转换原绑定会产生手腕与掌心变形。材质槽保持原角色的 Tunic、SleevesShorts、Skin 分配，顶点颜色保持白色；旧候选网格曾把所有渲染区块分配到 Tunic，因此旧截图里的蓝绿色手部并非正确皮肤材质。

## 第三人称左手握枪 IK

世界枪新增 `LeftHandGrip` Socket，定义的是左手 `hand_l` 骨骼的目标姿势，不是掌心表面的接触点。初始位置约 `(4.992082, 27.895344, -3.703362)` cm，位置和旋转从原角色 `Idle_equip_ak47` 第 0 帧、原 `AK47Socket` 与装备相对变换推导，保留静止握枪的手腕姿势。校准 authoring 在 `Saved/FirstPerson/create_left_hand_grip.py`，只用于生成资产，运行时不执行。

`UShooterAnimInstance::UpdateLeftHandIK` 在 C++ 中读取世界枪挂点，将其转为 `hand_r` 骨骼空间的 `LeftHandIKTransform`。`LeftHandIKAlpha` 在第三人称世界网格、存活且有带挂点的装备时为 1；第一人称网格、死亡、无装备或无挂点时为 0。两个值只服务于本地动画表现，不增加复制状态。枪与右手来自同一已完成姿势，转为右手空间后由当前帧 AnimGraph 的右手姿势定位，避免直接使用上一帧的世界空间目标。

`ABP_Player` 保留原状态机和移动混合，在最终混合之后使用 `LocalToComponent → FABRIK → ComponentToLocal → Output`。FABRIK 链为 `upperarm_l → hand_l`，Effector 使用 `hand_r` Bone Space，旋转为 `CopyFromTarget`，Precision 0.1 cm，MaxIterations 10；Transform 与 Alpha 直接读取 C++，没有增加 EventGraph 逻辑。原手指动画保持不变，IK 只约束左臂与手腕。

维护时优先在 `SM_World_AssaultRifle` 的 Socket Manager 调整 `LeftHandGrip` 的位置与旋转并保存，不用改每个行走动画；换新世界枪时给新网格配置同名挂点。角色骨架发生变化时再核对 FABRIK 的三处骨骼名。将节点 Alpha 临时设为 0 可对照原动画；正式配置仍连接 `LeftHandIKAlpha`。新增第三人称换弹等需要左手离枪的动作时，应在 C++ 中明确相应的 IK 启用条件。
