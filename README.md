# MR3 — 熔融金属泄漏 MR 最小 Demo

> 沉浸式生产实习项目：基于 UE 5.4 + VRTemplate，在 Xvisio Seer Pad ONE 头显上以混合现实（MR）形式呈现"熔融金属泄漏"事故的四阶段应急演练沙盘。
>
> 注：UE 工程名（`MR3.uproject` / `MR3.sln`）为 `MR3`，本仓库目录为 `MR5`，二者指同一工程。

作者：Yuquan Sun（北京航空航天大学）

---

## 1. 项目简介

本 Demo 将一座约 100 m × 80 m 的虚拟工厂缩放为桌面级 MR 沙盘（默认缩放 1/67 ≈ 0.015），佩戴 Xvisio 头显后可通过裸手手势对沙盘进行缩放、旋转、平移、阶段切换与目标指向，用于安全事故应急流程的可视化教学。

事故流程分为 4 个阶段：

| 阶段 | 名称 | 说明 |
|------|------|------|
| 1 | 泄漏疏散 | 高炉底部熔融金属泄漏、热区扩散、人员撤离 |
| 2 | 侦察指挥 | 无人机侦察、禁入区划定 |
| 3 | 封控处置 | 消防机器人 / 消防车介入、水雾降温 |
| 4 | 医疗救护 | 急救医生、急救站处置伤员 |

---

## 2. 技术栈

- **引擎**：Unreal Engine 5.4（Forward Shading + Lumen GI/反射 + Virtual Shadow Maps + InstancedStereo + MobileMultiView）
- **模板**：VRTemplate（默认地图 `VRTemplateMap`，GameMode `VRGameMode`）
- **XR 运行时**：OpenXR
- **头显 SDK**：Xvisio OpenXR 插件（`Plugins/XvisioOpenXR/`，提供手部关节、眼动、ASR、注视点渲染等）
- **串流链路**：Xvisio OpenXR 插件 → `xvisio_streamer` → SteamVR → UE
- **目标设备**：Xvisio Seer Pad ONE
- **调试工具**：ADB（`E:\VSCC\Android\platform-tools\adb.exe`）

---

## 3. 目录结构

```
MR5/
├── MR3.uproject          # UE 工程描述（引擎 5.4，启用 OpenXR + XvisioOpenXR 插件）
├── MR3.sln               # Visual Studio 解决方案
├── Source/MR3/           # C++ 游戏模块
│   ├── MR3.Build.cs      # 依赖 Core/Engine/HeadMountedDisplay/XvisioOpenXR
│   ├── MR3.Target.cs     # Game target
│   ├── MR3Editor.Target.cs
│   └── Public/MRSandboxRoot.h
│   └── Private/MRSandboxRoot.cpp
├── Content/
│   └── VRTemplate/       # VRTemplate 资产（Maps/VRTemplateMap 为默认关卡）
│   └── StarterContent/
├── Plugins/
│   └── XvisioOpenXR/     # Xvisio 设备插件（手部追踪 / 眼动 / ASR / 流媒体）
├── Config/
│   ├── DefaultEngine.ini # 默认地图、渲染与安卓打包设置
│   ├── MRSandbox_Handoff.md        # MRSandbox 编辑器侧接线手册
│   └── MRSandboxRoot_Handoff.md    # MRSandboxRoot 使用说明
├── Binaries/  DerivedDataCache/  Intermediate/  Saved/  Build/   # 生成产物，可随引擎重建
```

---

## 4. 环境要求

- Unreal Engine **5.4**
- Visual Studio 2022（含 C++ 工作负载与 Windows SDK）
- SteamVR（串流必备，运行时需保持开启）
- Xvisio 设备驱动 / `xvisio_streamer`
- Windows 10/11（开发主机）；可选 Android（Meta Quest 系列）打包环境

---

## 5. 编译与运行

1. 右键 `MR3.uproject` → **Generate Visual Studio project files**。
2. 用 Visual Studio 打开 `MR3.sln`，配置选 **Development Editor / Win64**，Build。
3. 启动 UE 编辑器（默认打开 `VRTemplateMap`）。
4. PIE / Standalone 运行前确保 SteamVR 与 `xvisio_streamer` 已启动、Xvisio 头显已连接。

> 编译失败常见原因：`XvisioOpenXR.h` 找不到 → 检查 `Plugins/XvisioOpenXR/` 是否完整，并确认 `MR3.Build.cs` 已依赖 `XvisioOpenXR`。

---

## 6. 核心功能

### 6.1 MRSandboxRoot（C++，`Source/MR3`）

沙盘根 Actor，将工厂 73 个 Actor 作为子物体挂载并整体缩放，提供裸手手势交互与四阶段状态机。

**编辑器侧操作**（选中 `MRSandboxRoot`，Details → Sandbox）：

1. **Collect Factory Actors From Level**：自动收集当前关卡工厂 Actor 到 `ChildActors`（已排除光照、相机、玩家、BP_TrackedHands/BP_MRControls 等）。
2. **Attach Children In Editor**：将子物体挂载并应用 `InitialScale`（默认 0.015），沙盘缩成桌面大小放到玩家前方。
3. **Clear Child Actors**：清空 `ChildActors`。

**可调参数**（详情见 `Config/MRSandboxRoot_Handoff.md`）：

| 参数 | 默认值 | 含义 |
|------|--------|------|
| InitialScale | 0.015 | 初始缩放（约 1/67） |
| InitialOffset | (100, 0, 0) | 相对玩家前方向偏移（cm） |
| HoverHeight | 10 | 悬浮高度（cm） |
| ScaleSensitivity / RotationSensitivity / TranslationSensitivity | 2 / 1 / 1 | 各手势灵敏度 |
| PinchThreshold / GrabThreshold | 2 / 4 | 捏合 / 握拳判定阈值（cm） |
| MinScale / MaxScale | 0.5 / 3 | 额外缩放倍数上下限 |

### 6.2 手势操作

| 手势 | 效果 |
|------|------|
| 双手拇指+食指捏合，拉开 / 合拢 | 放大 / 缩小沙盘 |
| 双手捏合后左右旋转 | 绕垂直轴旋转沙盘 |
| 单手握拳（四指贴掌心） | 水平拖拽平移沙盘 |
| 右手食指伸直，其余四指握拳 | 从食指指尖发出射线，命中 Actor 存入 `AimedActor` |

> 手势依赖 Xvisio 手部追踪：未戴头显 / OpenXR runtime 未启动时无响应。PC 上可用键盘 1/2/3/4（切阶段）、R（重置）调试，详见 `Config/MRSandbox_Handoff.md` 步骤 4。

### 6.3 阶段切换

将各阶段专属 Actor 分别填入 `Stage1Actors` ~ `Stage4Actors`，运行时调用 `SetStage(1..4)` 即可切换可见性；`ResetSandbox()` 还原初始变换与缩放。

### 6.4 已实现蓝图

- `BP_TrackedHands1` / `BP_TrackedHands2`：双手追踪可视化
- `BP_MRControls`：MR 视角控制
- `VRTemplateMap` 内工厂场景：73 个 Actor（立方体、墙、发光圆柱 / 平面表示熔融金属、高炉等）

---

## 7. 串流与设备调试

1. 主机启动 SteamVR 与 `xvisio_streamer`。
2. Xvisio Seer Pad ONE 通过 OpenXR runtime 接入。
3. 运行时日志见 `Saved/Logs/MR3.log`；手势订阅状态字段 `bL` / `bR` 为 false 时表示 Xvisio 手部追踪尚未取到数据。
4. 设备联调使用 ADB：
   ```bash
   E:/VSCC/Android/platform-tools/adb.exe devices
   E:/VSCC/Android/platform-tools/adb.exe logcat
   ```

---

## 8. 后续待办

- [ ] 阶段专属实体建模：熔融金属流、热区球、无人机、消防车 / 机器人、急救医生 / 急救站、禁入区围栏
- [ ] 手腕菜单 UMG（`WBP_SandboxMenu`）+ 3D WidgetComponent 接线
- [ ] 信息浮窗（`AimedActor` 命中后展示）
- [ ] 透明度滑块、危险热区高亮、阶段文字说明
- [ ] Passthrough / 平面检测（依赖 Xvisio SDK）

---

## 9. 参考文档

- `Config/MRSandbox_Handoff.md` — MRSandbox 编辑器侧接线手册（4 步）
- `Config/MRSandboxRoot_Handoff.md` — MRSandboxRoot 参数与使用说明
- `Plugins/XvisioOpenXR/XvisioOpenXR_API_Documentation.md` — Xvisio 插件 API 文档
