# 联机流程

## 分工

| 类 | 职责 |
| --- | --- |
| MultiplayerSessionsSubsystem | Create / Join / Destroy / Update、OSS Delegate、连接地址 |
| LobbyInviteSubsystem | 好友刷新、缓存、按好友 ID 邀请 |
| MultiplayerSessionFlowSubsystem | 流程阶段、旅行、加入许可、待确认邀请、失败恢复 |
| MultiplayerGameSession | 服务器登录检查，保留父类容量校验 |
| MultiplayerSessionUIManagerSubsystem | Widget 生命周期、输入模式和鼠标 |

子系统由 GameInstance 持有，跨地图存在。Widget 只发命令、显示状态，不持有在线请求。
当前只处理一个本地玩家，使用 OnlineSubsystemSteam。地图与 UI 类配置见 [DefaultGame.ini](../../Config/DefaultGame.ini) 的 MultiplayerSessionSettings。

## 建房与加入

```text
HostGame → CreateSession → ServerTravel 到 Lobby
接受邀请 → JoinSession → 解析地址 → ClientTravel → 确认真实本地 Controller 就绪
```

已在房间时接受另一份邀请，先弹确认；确认后退出、回菜单，再加入。取消则保留当前房间。
好友列表不是房间成员列表，发送邀请成功不表示对方已经加入。只有大厅房主且加入开放时才能邀请或开始。

## 开始与关闭加入

```text
StartHostedGame → 关闭服务器登录许可 → StartingGame
  → UpdateSession 关闭加入和邀请 → 成功后 Seamless ServerTravel 到 Gameplay
```

- LobbyGameMode 和 ShooterGameMode 共用 MultiplayerGameSession；PreLogin 与最终 Login 都经 ApproveLogin 检查许可。
- 原成员指已完成服务器登录的玩家，通过 HandleSeamlessTravelPlayer 旅行；旧邀请和仅进入 Steam Lobby 不算。
- 许可跨地图保留，比赛、结算、重开都不开放新登录；退出后重新连接也按新登录拒绝。
- 只有新 HostGame 创建 Session 成功才重新开放；Update 或 Travel 失败进入清理，迟到回调不能继续旅行。

## 请求与恢复

- bool 返回值只表示受理，完成可能在返回前同步发生；结果看完成回调或 OnStateChanged。
- 底层拒绝的请求不广播完成，受理后只完成一次。先记录 operation id 与 Interface / Handle，完成广播前释放自己的绑定。
- 忙碌时拒绝新操作，不排队；清理等待已有 OSS 操作结束，不额外加自定义超时。
- Leave 和失败恢复都先关闭加入，Destroy 本地 Session，再回 Menu 断开连接；成功后 Idle。
- Destroy 失败仍回菜单，保持 CleanupFailed，阻止建房/加入；再次 Leave 重试，回菜单失败不循环旅行。
- 客户端掉线不关闭房主房间。Network / Travel failure 按 GameInstance 和相关 NetDriver 过滤，延迟到引擎默认处理后恢复。
- Deinitialize 解绑自己持有的 OSS、引擎、LocalPlayer 和 Ticker 回调，不再推进流程。

## 代码入口

- [MultiplayerSessionFlowSubsystem.cpp](../../Plugins/MultiplayerSession/Source/MultiplayerSession/Private/MultiplayerSessionFlowSubsystem.cpp)：流程和旅行。
- [MultiplayerSessionsSubsystem.cpp](../../Plugins/MultiplayerSession/Source/MultiplayerSession/Private/MultiplayerSessionsSubsystem.cpp)：在线操作。
- [MultiplayerGameSession.cpp](../../Plugins/MultiplayerSession/Source/MultiplayerSession/Private/MultiplayerGameSession.cpp)：服务器准入。

## 踩坑/变更

- 地图加载或 Join 回调不代表连接完成；首次客户端 Controller 可能是临时对象，要与 ServerConnection 的 Controller 一致。
- Steam 可能同步完成后再返回 false；只有本次 operation id 仍有效时才能补报失败，不能重复完成。
- 关闭加入不能只改 Steam 设置，服务器最终登录也要拦；Standalone 菜单的本地登录单独放行，不重开旧房间许可。
