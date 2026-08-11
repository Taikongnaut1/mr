# XvisioOpenXR 插件文档

## 版本信息

| 项目 | 说明 |
|------|------|
| 插件名称 | XvisioOpenXR |
| 版本 | 1.2 |
| 适用引擎 | Unreal Engine 5.3.0+ |
| 支持平台 | Windows x64、Linux、Android |
| 依赖 | OpenXR 插件 |

---

## 1. 概述

XvisioOpenXR 是 Xvisio Technology 为 Unreal Engine 5 提供的 OpenXR 扩展插件，用于集成 Xvisio XR 设备的各项功能。插件支持两种工作模式：

- **Standalone 模式（本地）**：通过 OpenXR 运行时直接与本地 XR 设备通信，提供眼动追踪、手势识别、按键事件、AprilTag 检测、CSLAM 地图等功能。
- **Streaming 模式（串流）**：通过网络与远程 Xvisio 设备通信（基于 `xv_client.dll`），支持眼动数据+AprilTag 统一接收、语音识别（ASR）、SLAM 地图操作等功能。

---

## 2. 架构概览

```
┌────────────────────────────────────────────────────┐
│                   XvisioOpenXR 插件                  │
├─────────────────────┬──────────────────────────────┤
│   Standalone (本地)   │      Streaming (串流)         │
│                     │                              │
│ UXVisioXRWorld       │  UXVisioStreamWorld          │
│ Subsystem            │  Subsystem                   │
│  ├ 按键事件           │   ├ DLL 生命周期               │
│  ├ AprilTag 检测      │   ├ 网络客户端管理              │
│  └ 插件协调           │   ├ 眼动数据（远程）            │
│                     │   ├ 语音识别（远程）            │
│ 内部子插件：          │   └ SLAM 地图操作              │
│  ├ GazeCalibration   │                              │
│  ├ HandJointExtra    │  内部线程：                    │
│  ├ KeyEvent          │   ├ EyeGazeDataRunnable       │
│  │                   │   │  (眼动+Tag 统一拉取)        │
│  └ FXvisioFunction   │   └ ASRDataRunNable           │
└─────────────────────┴──────────────────────────────┘
```

两个 World Subsystem 均自动随 UWorld 创建/销毁，无需手动管理生命周期。

---

## 3. Standalone 模式（本地 OpenXR）

Standalone 模式通过 OpenXR 扩展接口与本地 XR 设备直接交互，适用于设备直连场景。

### 3.1 眼动追踪与标定

通过 `GazeCalibrationPlugin` 提供完整的眼动追踪功能：

| C++ 方法 | 说明 |
|----------|------|
| `StartGaze(ret)` | 启动注视追踪 |
| `StopGaze(ret)` | 停止注视追踪 |
| `GetGazeCallback(data)` | 获取注视回调数据（含左右眼注视点、瞳孔信息等） |
| `GazeSetConfigPath(path)` | 设置注视配置文件路径 |
| `SetGazeConfigs(data)` | 设置注视参数配置 |
| `StartEyetracking(ret)` | 启动眼动追踪 |
| `StopEyetracking(ret)` | 停止眼动追踪 |
| `GazeCalibrationPubBegin(idx, ret)` | 开始发布标定数据 |
| `GazeCalibrationPubSetConfGaze(type, idx, x, y, ret)` | 设置标定置信度注视点 |
| `GazeCalibrationPubEnd(idx, ret)` | 结束发布标定数据 |
| `SetExposure(lGain, lTime, rGain, rTime, ret)` | 设置摄像头曝光参数 |
| `SetBright(eye, led, brightness, ret)` | 设置 LED 亮度 |
| `ChangeIpd(distance, ret)` | 修改瞳距（IPD） |
| `CameraDistance(distance)` | 设置摄像头距离 |
| `GetDeviceCameraImageData(...)` | 获取设备摄像头图像 |
| `GetGazeIpd(ipd)` | 获取当前瞳距值 |
| `SetUsrEyeReady(ret)` | 通知用户眼睛就绪 |
| `GazeEnableDump(enable, ret)` | 启停注视数据 Dump |
| `SetPref(etIdx, data, size, ret)` / `GetPref(etIdx, data, size, ret)` | 读写偏好设置 |
| `GetHiValue(ret)` / `GetSrValue(ret)` | 获取 HI / SR 值 |

### 3.2 手势追踪

通过 `HandJointExtraPlugin` 获取扩展的手部关节数据：

| 蓝图方法 | 参数 | 说明 |
|----------|------|------|
| `GetHandJointExtraData` | Hand（Left/Right） | 获取指定手的关节位置、旋转和半径 |

**返回值**：
- `OutPositions` — 关节位置数组（FVector）
- `OutRotations` — 关节旋转数组（FQuat）
- `OutRadii` — 关节半径数组（float）

### 3.3 按键事件

| 蓝图方法 | 返回值 | 说明 |
|----------|--------|------|
| `StartEvent()` | bool | 启动按键事件监控 |
| `StopEvent()` | bool | 停止按键事件监控 |

| 蓝图事件代理 | 触发时机 |
|-------------|---------|
| `OnKeyDown` | 按键按下时广播 |
| `OnKeyDownTick` | 按键持续按住时每帧广播 |
| `OnKeyup` | 按键松开时广播 |

### 3.4 AprilTag 检测

| 蓝图方法 | 参数 | 说明 |
|----------|------|------|
| `GetApriltag(InTagFamily, InTagSize, InDetectType, OutTagCount)` | InTagFamily: 标签族字符串（如 "36h11"）<br>InTagSize: 标签物理尺寸（米）<br>InDetectType: 检测类型<br>OutTagCount: 输出检测到的标签数量 | 触发 AprilTag 检测（最多 32 个），返回 bool 表示成功/失败 |
| `GetApriltagData(Index, OutTagID, OutPosition, OutOrientation, OutQuaternion, OutConfidence)` | Index: 标签索引（0 ~ OutTagCount-1）<br>OutTagID: 输出标签 ID<br>OutPosition: 输出位置 (FVector)<br>OutOrientation: 输出朝向 (FVector)<br>OutQuaternion: 输出四元数 (FVector4)<br>OutConfidence: 输出置信度 (float) | 按索引获取单个标签的位姿数据，返回 bool 表示索引是否有效 |
| `StopApriltag(ret)` | ret: 返回码 | 停止 AprilTag 检测 |

**使用流程**：先调用 `GetApriltag` 触发检测并获取标签数量，再遍历调用 `GetApriltagData` 逐个读取标签位姿。

### 3.5 CSLAM 地图操作

| C++ 方法 | 说明 |
|----------|------|
| `GetCslamCallbackData(data)` | 获取 CSLAM 回调数据（map_quality / percent / status_of_saved_map） |
| `LoadMapAndSwitchToCslam(path)` | 加载地图文件并切换到 CSLAM 模式 |
| `SaveMapAndSwitchToCslam(path)` | 保存当前地图并切换到 CSLAM 模式 |

---

## 4. Streaming 模式（串流）

Streaming 模式通过网络连接到远程 Xvisio 设备（Box 端，默认 IP `192.168.31.93`），底层依赖 `xv_client.dll`。

### 4.1 DLL 与连接管理

| 蓝图方法 | 参数 | 返回值 | 说明 |
|----------|------|--------|------|
| `LoadXvisioDLL` | ClientIP（默认 192.168.31.93） | void | 加载 DLL 并建立连接 |
| `FreeXvisioDLL` | — | void | 释放 DLL |
| `CreateClient` | — | int32 | 创建客户端（返回 clientId） |
| `CreateClientWithIp` | ip | int32 | 使用指定 IP 创建客户端 |
| `CloseClient` | — | void | 关闭客户端连接 |

### 4.2 眼动追踪（远程，含 AprilTag 统一数据流）

`StartEyeTracking` 启动后，设备端会一并发送眼动数据和 AprilTag 检测结果。`EyeGazeDataRunnable` 后台线程统一拉取并解析。该方法内置防重复调用保护，多次调用不会重复启动。

| 蓝图方法 | 参数 | 返回值 | 说明 |
|----------|------|--------|------|
| `StartEyeTracking` | — | void | 启动远程眼动追踪（同时开启 AprilTag 数据流），重复调用安全 |
| `EndEyeTracking` | — | void | 停止远程眼动追踪（同时停止 AprilTag 数据流） |
| `GetEyeReceivedData` | OutEyeData（FEyeData 引用） | bool | 获取最新眼动数据 |

### 4.3 语音识别（ASR）

| 蓝图方法 | 参数 | 返回值 | 说明 |
|----------|------|--------|------|
| `StartASR` | bnf（BNF 语法字符串） | void | 启动语音识别 |
| `EndASR` | — | void | 停止语音识别 |
| `GetReceivedMessage` | — | FString | 获取语音识别结果消息 |

**ASR 消息类型**（内部 JSON action 字段）：
- `onSpeechStart` — 语音识别开始
- `onSpeechEnd` — 语音识别结束
- `onAsrResult` — 识别成功（msg 字段含识别文本）
- `onAsrNoResult` — 未匹配或匹配置信度过低
- `onKeyPressed` — Box 端按键事件
- `loadMapDone` / `saveMapDone` — 地图加载/保存完成

### 4.4 AprilTag 检测（远程）

Streaming 模式中，AprilTag 数据与眼动数据在同一个 JSON 报文中由设备端一并发送。底层由 `EyeGazeDataRunnable` 统一拉取并解析 `tags` 数组，无需单独启动 AprilTag 检测线程。Tag 检测参数（标签族、尺寸、检测类型）由设备端 `startEyeTracking` 统一配置，客户端无需传入。

| 蓝图方法 | 参数 | 返回值 | 说明 |
|----------|------|--------|------|
| `GetApriltag` | OutTagCount (int32&) | bool | 获取当前检测到的标签数量（从 EyeGazeDataRunnable 缓存读取） |
| `GetApriltagData` | Index (int32)<br>OutTagID (int32&)<br>OutPosition (FVector&)<br>OutOrientation (FVector&)<br>OutQuaternion (FVector4&)<br>OutConfidence (float&) | bool | 按索引获取单个标签的位姿数据 |
| `StopApriltag` | — | void | 标记停止（tag 数据随 EndEyeTracking 统一停止） |

**使用流程**：先调用 `StartEyeTracking` 启动统一数据流，然后每帧调用 `GetApriltag` 获取标签数量、`GetApriltagData` 遍历标签数据，结束后调用 `EndEyeTracking` 即可（`StopApriltag` 为兼容保留，内部为空操作）。

**注意**：数据来自远程设备上的 SLAM 坐标系（Unity 惯例：X=右, Y=上, Z=前），使用时需要转换为 UE 坐标系（X=前, Y=右, Z=上）。

**统一 JSON 数据格式**（`GetEyeReceivedData` 返回）：
```json
{
  "glassKeyType": [-1],
  "gestureIndex": [6, -1],
  "position": [0, 0, 0],
  "orientation": [-0.033, 0.015, 0.0008, -1],
  "boxOrientation": [0.847, 0.450, -0.036, -0.279],
  "tags": [
    {
      "tagID": 0,
      "position": [-0.364, -0.091, 0.618],
      "orientation": [0.368, -0.218, -1.520],
      "quaternion": [0.205, 0.048, -0.659, 0.722],
      "edgeTimestamp": 382330619,
      "hostTimestamp": 10907.674,
      "confidence": 0.059
    }
  ]
}
```

### 4.5 SLAM 地图操作

| 蓝图方法 | 参数 | 说明 |
|----------|------|------|
| `StartCSLAMMap` | — | 启动 CSLAM 建图 |
| `SetMapPath` | filePath | 设置服务器端地图路径 |
| `SaveSlamMap` | pcPath, fileName | 保存 SLAM 地图 |
| `SendMapFile` | filePath | 发送地图文件到服务器，发送成功后设备端自动加载该地图 |

---

## 5. 蓝图 API 快速参考

### 5.1 UXvisioStreamWorldSubsystem（串流）

获取方式：蓝图节点 `Get Subsystem` → Class 选择 `UXVisioStreamWorldSubsystem`

```
## DLL/连接
LoadXvisioDLL(ClientIP) -> void
FreeXvisioDLL() -> void
CreateClient() -> int32
CreateClientWithIp(ip) -> int32
CloseClient() -> void

## 眼动 (远程)
StartEyeTracking() -> void
EndEyeTracking() -> void
GetEyeReceivedData(OutEyeData) -> bool

## 语音识别
StartASR(bnf) -> void
EndASR() -> void
GetReceivedMessage() -> FString

## SLAM
StartCSLAMMap() -> void
SetMapPath(filePath) -> void
SaveSlamMap(pcPath, fileName) -> void
SendMapFile(filePath) -> void

## AprilTag (远程，统一数据流)
GetApriltag(OutTagCount) -> bool
GetApriltagData(Index, OutTagID, OutPosition, OutOrientation, OutQuaternion, OutConfidence) -> bool
StopApriltag() -> void
```

### 5.2 UXVisioXRWorldSubsystem（本地）

获取方式：蓝图节点 `Get Subsystem` → Class 选择 `UXVisioXRWorldSubsystem`

```
## 按键
StartEvent() -> bool
StopEvent() -> bool
OnKeyDown (Event Dispatcher)
OnKeyDownTick (Event Dispatcher)
OnKeyup (Event Dispatcher)

## AprilTag
GetApriltag(InTagFamily, InTagSize, InDetectType, OutTagCount) -> bool
GetApriltagData(Index, OutTagID, OutPosition, OutOrientation, OutQuaternion, OutConfidence) -> bool
StopApriltag(ret) -> bool
```

### 5.3 UXvisioOpenXR（静态函数库）

可用蓝图节点直接搜索：

```
StartEvent(WorldContextObject) -> bool
StopEvent(WorldContextObject) -> bool
GetHandJointExtraData(Hand, OutPositions, OutRotations, OutRadii) -> bool
```

### 5.4 UXvisioStream（静态函数库，串流便捷封装）

可用蓝图节点直接搜索（每个节点自动带 WorldContextObject 引脚）：

```
LoadXvisioDLL(WorldContextObject, ClientIP) -> void
CreateClient(WorldContextObject) -> int32
StartEyeTracking(WorldContextObject) -> void
GetEyeReceivedData(WorldContextObject, OutEyeData) -> bool
StartASR(WorldContextObject, bnf) -> void
GetReceivedMessage(WorldContextObject) -> FString
GetApriltag(WorldContextObject, OutTagCount) -> bool
GetApriltagData(WorldContextObject, Index, OutTagID, OutPosition, OutOrientation, OutQuaternion, OutConfidence) -> bool
StopApriltag(WorldContextObject) -> void
...
```

---

## 6. 数据结构

### 6.1 FEyeData（蓝图可用）

| 字段 | 类型 | 说明 |
|------|------|------|
| Position | FVector | 注视射线起点 |
| PositionEnd | FVector | 注视射线终点 |

### 6.2 FApriltagStreamData（蓝图可用，串流模式）

| 字段 | 类型 | 说明 |
|------|------|------|
| TagID | int32 | 标签 ID |
| Position | FVector | 标签位置（SLAM 坐标系） |
| Orientation | FVector | 标签朝向欧拉角（Pitch, Yaw, Roll） |
| Quaternion | FVector4 | 标签朝向四元数（X, Y, Z, W） |
| Confidence | float | 检测置信度 |

### 6.3 XrEtEyeDataEx（C++）

完整眼动数据结构，包含：
- 左右眼注视点（`leftGaze` / `rightGaze`，含原始值、平滑值、置信度）
- 左右眼瞳孔信息（`leftPupil` / `rightPupil`，含中心、直径等）
- 眼睑数据（`leftExData` / `rightExData`，含眨眼、开合度）
- 瞳距（`ipd`）

### 6.4 XrCSlamCallbackDataXV（C++）

| 字段 | 类型 | 说明 |
|------|------|------|
| map_quality | uint32_t | 地图质量 |
| percent | float | 完成百分比 |
| status_of_saved_map | uint32_t | 保存状态 |

### 6.5 XrEventData（C++）

| 字段 | 类型 | 说明 |
|------|------|------|
| hostTimestamp | double | 主机时间戳 |
| degeTimestampUs | int64_t | 设备时间戳（微秒） |
| eventType | int32_t | 事件类型 |
| eventState | int32_t | 事件状态 |

---

## 7. 集成指引

### 7.1 安装

1. 将 `XvisioOpenXR` 文件夹放入项目的 `Plugins/` 目录
2. 确保项目已启用 OpenXR 插件
3. 在 `.uproject` 中添加插件引用（或通过 UE 编辑器启用）

```json
{
    "Plugins": [
        { "Name": "XvisioOpenXR", "Enabled": true }
    ]
}
```

### 7.2 Standalone 模式使用流程

1. 启动 UE 项目，插件自动加载
2. 蓝图或 C++ 中调用 `UXvisioOpenXR::StartEvent()` 开始监听设备按键
3. 绑定 `OnKeyDown` / `OnKeyup` 事件处理设备按键
4. 通过 `UXvisioOpenXR::GetHandJointExtraData()` 获取手部数据
5. 通过 C++ 调用 `GazeCalibrationPlugin` 进行眼动追踪标定

### 7.3 Streaming 模式使用流程

1. 调用 `UXvisioStream::LoadXvisioDLL(IP)` 加载 DLL 并连接远程设备
2. 调用 `StartEyeTracking` 启动统一数据流（眼动 + AprilTag），调用 `StartASR` 启动语音识别
3. 在 Tick 中轮询：
   - `GetEyeReceivedData` 获取眼动数据
   - `GetApriltag` / `GetApriltagData` 获取 AprilTag 标签数据（与眼动同一 JSON 报文中解析）
   - `GetReceivedMessage` 获取语音识别结果
4. 结束时调用 `EndEyeTracking`（同时停止 tag 数据）、`EndASR`，最后 `CloseClient` 断开连接
5. SLAM 地图：`SendMapFile` 发送地图文件后设备端会自动加载，可通过 ASR 消息 `loadMapDone` 确认加载完成
6. 注意：`StartEyeTracking` 内置防重复调用保护，多次调用不会重复启动

### 7.4 注意事项

- Streaming 模式需要 `xv_client.dll`（Windows）或 `libxv_client.so`（Linux）位于 `Binaries/<Platform>/` 目录
- Streaming 默认连接 IP 为 `192.168.31.93`，可在 `LoadXvisioDLL` 中指定
- Windows 上 DLL 搜索路径为 `项目目录/Binaries/Win64/xv_client.dll`
- Linux 上 SO 搜索路径为 `项目目录/Binaries/Linux/libxv_client.so`
- `WorldContextObject` 参数在蓝图中自动填充，C++ 中需传入任意有效的 UObject

---

## 8. 平台支持

| 功能 | Windows x64 | Linux | Android |
|------|:-----------:|:-----:|:-------:|
| Standalone 模式 | ✓ | ✓ | ✓ |
| Streaming 模式（xv_client） | ✓ | ✓ | ✗ |
| 眼动追踪 | ✓ | ✓ | — |
| 手势识别 | ✓ | ✓ | — |
| 语音识别 | ✓ | ✓ | ✗ |
| AprilTag 检测 | ✓ | ✓ | — |
| CSLAM 地图 | ✓ | ✓ | — |

---

## 9. 联系方式

- 公司：Xvisio Technology
- 创建者：xvisio
- 技术支持：[联系 Xvisio 技术支持获取详细信息]

---

*文档版本 1.2，最后更新 2026-05*