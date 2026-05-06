<div align="center">

# OpenArma Mod Main

**Arma Reforger 核心 AI 指挥官模组**

[OpenArma 主仓库](https://github.com/chenhaha99/OpenArma) | [MapExporter](https://github.com/chenhaha99/OpenArma-Mod-MapExporter) | [MapScanner](https://github.com/chenhaha99/OpenArma-Mod-MapScanner)

</div>

---

## 简介

OpenArma Mod Main 是 [OpenArma](https://github.com/chenhaha99/OpenArma) 在 Arma Reforger 中的核心运行时模组。它实现 AI 指挥官的完整运行闭环：观察战场 → 上报态势 → 接收指令 → 执行命令。

模组本身不做决策 — 它是一个 **Agent MCP（模型上下文协议）客户端**，通过 REST API 与 OpenArma 后端通信，由后端的 Multi-Agent 引擎驱动所有战术决策。

## 架构

```
OA_Main (单例控制器)
  ├── OA_WorldObserver     观察：扫描战场态势，构建 JSON 态势报告
  ├── OA_DecisionBridge    通信：REST API 收发（心跳轮询）
  ├── OA_CommandExecutor   执行：将 AI 指令转化为游戏内 Waypoint
  ├── OA_EventTracker      事件：追踪战斗事件（交战/伤亡/目标发现）
  ├── OA_HudIndicator      UI：连接状态/运行状态/阵营显示
  └── OA_Config            配置：API 地址/密钥/决策间隔
```

## 运行流程

```
1. Login     玩家输入 API Key → 与后端建立连接
2. Heartbeat 每 N 秒：WorldObserver 扫描 → 构建态势 JSON → 发送到后端
3. Response  后端返回：待执行指令 + 最新配置（启停/阵营/间隔）
4. Execute   CommandExecutor 将 JSON 指令转为游戏 Waypoint（移动/防守/巡逻/攻击）
5. Loop      重复 2-4，直到 Logout
```

## 核心功能

- **多阵营支持**：动态 N 方配置（AI vs 人类 / AI vs AI），每方独立的控制模式
- **态势报告**：自动扫描友军/敌军位置、编制、武器、健康、弹药、当前任务
- **指令执行**：支持 `move` / `defend` / `patrol` / `attack` / `regroup` 等战术命令
- **战斗事件追踪**：交战开始/结束、单位阵亡、敌军发现等事件上报
- **断线重连**：指数退避重试机制，最多 10 次自动重连
- **HUD 显示**：连接状态指示灯（绿/黄/红/灰）+ 当前阵营配置

## 安装

1. 在 Arma Reforger Workbench 中导入本模组
2. 将 `OA_GameModeInjector` 或 `OA_GameModeComponent` 添加到场景 GameMode
3. 确保 OpenArma 后端已运行
4. 游戏内输入 API Key 连接

## 文件说明

```
OA/
├── Scripts/Game/OA/
│   ├── OA_Main.c                 单例控制器：Login → Heartbeat → Logout
│   ├── OA_WorldObserver.c        战场态势扫描 & JSON 构建
│   ├── OA_DecisionBridge.c       REST API 通信层
│   ├── OA_CommandExecutor.c      AI 指令 → 游戏 Waypoint 转化
│   ├── OA_EventTracker.c         战斗事件追踪
│   ├── OA_HudIndicator.c         HUD 状态显示
│   ├── OA_InputHandler.c         键盘输入处理
│   ├── OA_DataStructs.c          数据结构定义
│   ├── OA_Config.c               配置管理
│   ├── OA_GameModeComponent.c    GameMode 组件注入
│   ├── OA_GameModeInjector.c     GameMode 自动注入
│   └── OA_PlayerControllerInjector.c  玩家控制器注入
├── UI/
│   ├── layouts/                  HUD 布局文件
│   └── Textures/                 状态指示灯贴图
└── addon.gproj                   Enfusion 项目文件
```

## 许可证

[MIT](LICENSE)

## 关联项目

- **[OpenArma](https://github.com/chenhaha99/OpenArma)** — 后端 + 前端（Multi-Agent 平台）
- **[OpenArma-Mod-MapExporter](https://github.com/chenhaha99/OpenArma-Mod-MapExporter)** — Workbench 地图数据导出工具
- **[OpenArma-Mod-MapScanner](https://github.com/chenhaha99/OpenArma-Mod-MapScanner)** — 游戏内地图扫描工具
