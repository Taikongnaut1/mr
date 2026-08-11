# MRSandboxRoot 修改指南

> 这份文档告诉你：代码在哪个文件、关键参数在哪一行、怎么改、怎么编译。

---

## 1. 文件位置

所有手势/缩放/平移逻辑都在这两个文件：

| 文件   | 路径                                                       | 作用         |
| ---- | -------------------------------------------------------- | ---------- |
| 头文件  | `E:\VSCC\test1\MR5\Source\MR3\Public\MRSandboxRoot.h`    | 声明参数、变量、函数 |
| 实现文件 | `E:\VSCC\test1\MR5\Source\MR3\Private\MRSandboxRoot.cpp` | 所有逻辑代码     |

模块入口（一般不用改）：

| 文件 | 路径                                                   | 作用                   |
| -- | ---------------------------------------------------- | -------------------- |
| 模块 | `E:\VSCC\test1\MR5\Source\MR3\Private\MR3Module.cpp` | UE 模块加载入口，注册自动 Spawn |

---

## 2. 关键参数（在 .h 文件里改）

打开 `MRSandboxRoot.h`，找到 `Category = "Sandbox"` 的这些变量：

### 2.1 缩放参数

```cpp
// 初始缩放（进游戏自动缩到多少）0.1 = 1/10
float InitialScale = 0.1f;

// 缩放灵敏度（双手捏合拉开时，数值越大缩放越快）
float ScaleSensitivity = 2.0f;

// 最小/最大缩放倍数（相对 InitialScale）
float MinScale = 0.5f;   // 最小 = InitialScale × 0.5
float MaxScale = 3.0f;   // 最大 = InitialScale × 3
```

### 2.2 旋转参数

```cpp
// 旋转灵敏度（双手捏合左右转时，数值越大转得越快）
float RotationSensitivity = 3.0f;
```

### 2.3 平移参数

```cpp
// 平移灵敏度（单手握拳移动时，数值越大场景跟手越快）
float TranslationSensitivity = 5.0f;
```

### 2.4 手势阈值

```cpp
// 捏合阈值：拇指尖 ↔ 食指尖 距离 < 此值算捏合（cm）
float PinchThreshold = 2.0f;

// 握拳阈值：手指尖到掌心距离 < 此值算弯曲（cm）
float GrabThreshold = 7.0f;
```

### 2.5 场景位置

```cpp
// 场景放在玩家前方的距离（cm），300 = 3 米
float SandboxForwardDistance = 300.0f;

// 场景高度偏移（cm），-80 = 比眼睛低 80cm
float SandboxHeightOffset = -80.0f;
```

---

## 3. 手势判定逻辑（在 .cpp 文件里）

### 3.1 捏合和握拳判定

文件：`MRSandboxRoot.cpp`，函数 `GetHandPinchAndGrab`（搜索 `GetHandPinchAndGrab`）

```cpp
// 捏合：拇指食指距离 < PinchThreshold，且没握拳
bOutPinching = (PinchDist < PinchThreshold) && !bOutGrabbing;

// 握拳：3/4 投票 + 滞回
// 触发：3+ 手指弯曲
// 解除：1 或更少手指弯曲（中间区域保持上一帧状态）
if (bLastGrab) {
    bOutGrabbing = (CurlCount >= 2);  // 已握拳，宽松解除
} else {
    bOutGrabbing = (CurlCount >= 3);  // 未握拳，严格触发
}
```

### 3.2 食指射线判定

文件：`MRSandboxRoot.cpp`，函数 `IsIndexExtended`（搜索 `IsIndexExtended`）

```cpp
// 食指伸直：食指离掌心 > 8cm
// 其余三指有 2+ 个弯曲
return (IndexCurl > 8.0f) && (OtherCurlCount >= 2);
```

### 3.3 缩放/旋转/平移触发

文件：`MRSandboxRoot.cpp`，函数 `UpdateHandInteraction`（搜索 `UpdateHandInteraction`）

| 条件                | 效果 | 代码位置                 |
| ----------------- | -- | -------------------- |
| 双手捏合 + 距离变化 > 1cm | 缩放 | 搜索 `bWantScale`      |
| 双手捏合 + 角度变化 > 3°  | 旋转 | 搜索 `bWantRotate`     |
| 单手握拳 + 另一只张开      | 平移 | 搜索 `bGrabbingNow`    |
| 右手食指伸直            | 射线 | 搜索 `IsIndexExtended` |

### 3.4 场景缩放公式

文件：`MRSandboxRoot.cpp`，函数 `ApplySandboxScale`（搜索 `ApplySandboxScale`）

```cpp
// 每个物体的新位置 = 目标中心 + 平移 + 旋转(相对位置 × 缩放)
NewLocation = SandboxTargetCenter + SandboxTranslation + Rotate(RelativePos × CurrentScale)
```



---

## 4. 怎么编译

### 4.1 编译命令

每次改完代码，运行这个命令编译（**必须先关掉 UE 编辑器**）：

```bash
"E:\software\UE\UE_5.4\Engine\Build\BatchFiles\Build.bat" MR3Editor Win64 Development -Project="E:\VSCC\test1\MR5\MR3.uproject"
```

编译成功会显示 `Total execution time: X.XX seconds`，`Exit Code: 0`。

### 4.2 完整流程

1. **改代码**：用 VS Code / Notepad++ / Visual Studio 打开 `.h` 或 `.cpp` 文件，改参数或逻辑
2. **关编辑器**：完全关掉 UE 编辑器（任务管理器确认 `UnrealEditor.exe` 没了）
3. **编译**：运行上面的命令
4. **启动编辑器**：双击 `E:\VSCC\test1\MR5\MR3.uproject`
5. **PIE 测试**：按 Play，点 VR 对话框确认

### 4.3 常见编译错误

| 错误                                            | 原因        | 解决           |
| --------------------------------------------- | --------- | ------------ |
| `Unable to build while Live Coding is active` | UE 编辑器还开着 | 关掉编辑器再编译     |
| `error C2065: 'XXX': undeclared identifier`   | 变量名拼错或没声明 | 检查 .h 里有没有声明 |
| `error C2440`                                 | 类型不匹配     | 检查赋值类型       |

---

## 5. 怎么调参数（不改代码）

有些参数可以在 UE 编辑器里直接调（不用编译）：

1. PIE 时按 `F8` 进入自由视角
2. 选中 `MRSandboxRoot`（World Outliner 里找）
3. Details 面板 → `Sandbox` 分类
4. 直接改 `ScaleSensitivity` / `RotationSensitivity` / `TranslationSensitivity` 等

**注意**：因为 MRSandboxRoot 是自动 Spawn 的，编辑器里改的值**不会保存**。要永久生效必须改 .h 里的默认值 + 重新编译。

---

## 6. 怎么看日志（调试用）

日志文件：`E:\VSCC\test1\MR5\Saved\Logs\MR3.log`

关键日志（搜索关键词）：

| 关键词                              | 含义                       |
| -------------------------------- | ------------------------ |
| `MRSandboxRoot: Tick`            | 每帧手势状态（bL/bR/pinch/grab） |
| `MRSandboxRoot: Hand=`           | 每只手的 curl 距离值            |
| `MRSandboxRoot: Grab started`    | 握拳开始位置                   |
| `MRSandboxRoot: Translate Delta` | 平移量                      |
| `MRSandboxRoot: ScaleSandbox`    | 缩放触发                     |
| `MR3: FMR3Module::StartupModule` | 模块加载成功                   |

---

## 7. 快速参考：改什么效果

| 想要效果    | 改哪里                          | 怎么改                                         |
| ------- | ---------------------------- | ------------------------------------------- |
| 场景更大/更小 | `.h` InitialScale            | 0.1 改成 0.05（更小）或 0.2（更大）                    |
| 缩放更快/更慢 | `.h` ScaleSensitivity        | 2.0 改成 5.0（更快）或 1.0（更慢）                     |
| 旋转更快/更慢 | `.h` RotationSensitivity     | 3.0 改成 5.0（更快）或 1.0（更慢）                     |
| 平移更明显   | `.h` TranslationSensitivity  | 5.0 改成 10.0（更明显）或 2.0（更克制）                  |
| 捏合更难触发  | `.h` PinchThreshold          | 2.0 改成 1.0（更难）或 3.0（更容易）                    |
| 握拳更难触发  | `.h` GrabThreshold           | 7.0 改成 5.0（更难）或 9.0（更容易）                    |
| 场景更近/更远 | `.h` SandboxForwardDistance  | 300 改成 200（更近）或 400（更远）                     |
| 场景更高/更低 | `.h` SandboxHeightOffset     | -80 改成 -40（更高）或 -120（更低）                    |
| 缩放旋转阈值  | `.cpp` UpdateHandInteraction | `bWantScale` 的 1.0cm / `bWantRotate` 的 3.0° |
