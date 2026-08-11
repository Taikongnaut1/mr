# MRSandbox 插件 — 编辑器侧接线手册

> 本文档配套 `Plugins/MRSandbox/` 纯 C++ 骨架使用。
> **C++ 已交付**: 沙盘根 Actor 变换 + 4 阶段状态机 + 手势订阅（Pinch/Grab/双手缩放/旋转/单手平移/食指射线/掌心菜单）。
> **编辑器侧仍需手动完成**: 4 步。

---

## 步骤 0：编译工程（一次）

1. 用 **Rider / Visual Studio** 打开 `MR3.uproject` 右键 → **Generate Project Files** → 编译。
2. 打开 UE 编辑器 → **Edit → Plugins**：应当能看到 "MR Sandbox Controller" 处于 *Enabled* 状态。
3. 如果编译失败，常见原因：
   - **找不到 `XvisioOpenXR.h`**：检查 `Plugins/XvisioOpenXR/` 是否完整（这次没动它，理论上没问题）。
   - **`External.zip` 没解压**：Xvisio 插件目录 `Source/XvisioOpenXR/Private/External.zip` 是它自己的 OpenXR 头备份，与我们无关；如果你看到 `External/openxr/` 已经解压出来就不管它。

---

## 步骤 1：在关卡里放置沙盘根 Actor

1. 打开 `Content/VRTemplate/Maps/VRTemplateMap`。
2. **Place Actors → All → MRSandbox → BP_MRSandboxController**。
   - 若下拉里看不到，先右键 *C++ Classes → MRSandbox* 刷新一下。
3. 在 *Details* 面板里：
   - **Transform**：Location 改到你希望沙盘"初始放置"的位置（建议 `X=0, Y=100, Z=70` cm = 玩家前方 1 m、桌面 0.7 m 高）。
   - **Composition → ControlledActors**：点 `+` 把关卡里 73 个 Actor 全部加进来（截图里的所有静态物体：高炉圆柱、立方体、墙、发光圆柱/平面、地面 Plane 等）。
     - **省时技巧**：在 *World Outliner* 里 `Ctrl+A` → 拖到 *ControlledActors*。
4. **Composition → StageActors**：按方案附录 A 把 Actor 按 4 个阶段分到 4 个 key（拖拽到 StageActors map 的 4 行里即可）：
   - `Stage1_LeakAndEvacuate`：高炉底部的泄漏口 Mesh、熔融金属流（你已有的发光圆柱+发光平面）、热区球（待新建）。
   - `Stage2_ReconAndCommand`：无人机（待新建）、禁入区围栏（待新建）。
   - `Stage3_ContainAndHandle`：消防机器人、消防车（待新建）、水雾粒子（待新建）。
   - `Stage4_MedicalTreatment`：急救医生、急救站高亮（待新建）。

   > **现状说明**：你现在的截图里基础几何体已就位，主要缺**熔融金属流、热区球、无人机、消防车/机器人、急救医生、急救站、禁入区**这些"区分阶段"的实体。
   > 本轮**没有**自动新建它们 — 按"精准修改"原则，留给后续逐阶段完善，免得一次性塞 50 个 Actor 让场景变得不可读。

---

## 步骤 2：挂手势订阅组件 + 配 Pawn

1. 找到当前关卡里负责"玩家"的 Pawn（即头显视角的载体，看 *World Outliner* 里和 `VRSpectator` 关联的那个，通常是 `BP_VRPawn` 或 VRTemplate 自带的）。
2. *Add Component* → **Hand Interactor**（类名 `UMRSandboxHandInteractor`，来自 MRSandbox 模块）。
3. 在组件的 *Details* 里：
   - **Sandbox**：拖入步骤 1 放的 `BP_MRSandboxController`。
   - 其他参数先用默认值。

---

## 步骤 3：手腕菜单 UMG（**必须在编辑器里画**，C++ 只暴露委托）

> 这一步**不能**在代码里完成。方案 5.3 节要求 3D 面板（WidgetComponent + Billboard），需要你画 UMG。

### 3.1 画菜单资产
1. *Content Browser → Add → User Interface → Widget Blueprint*，命名 `WBP_SandboxMenu`。
2. 在 UMG 里画 5 区块（参考方案 5.3 节那张 ASCII 图）：
   - 阶段按钮：`Stage1_Leak_Btn` / `Stage2_Recon_Btn` / `Stage3_Contain_Btn` / `Stage4_Medical_Btn`
   - 播放控制：`Play_Btn` / `Pause_Btn` / `ResetStage_Btn`
   - 显示：`Transparency_Slider`（可选）、`Hazards_Toggle`（可选）
3. 控件留 9 个**蓝图事件绑定接口**（详见 3.2）。

### 3.2 创建菜单 Actor 蓝图
1. *Add → Blueprint Class → Actor*，命名 `BP_SandboxMenuActor`。
2. 添加组件：
   - **WidgetComponent**：Class = `WBP_SandboxMenu`，空间 = `World`，draw at = `Translucent`。
   - **设置 WidgetComponent → Pivot = (0.5, 0.0, 0.0)**，让面板从手腕上方 10 cm 处升起而非居中。
3. 在 BeginPlay：
   - 把 WidgetComponent 隐藏（默认关闭，`SetVisibility(false)`）。
   - 等 `UMRSandboxHandInteractor::OnIntentChanged` 广播 `MenuVisible` 时显示，广播 `None` 时隐藏。
   - 订阅 `UMRSandboxHandInteractor::OnSandboxTap` 作为"刚呼出菜单"事件。
4. 把按钮的 OnClicked 绑定到 `AMRSandboxController::SetStage / Play / Pause / ResetSandbox` 调用：
   - **Stage1_Btn → Set Stage1_LeakAndEvacuate**
   - Play_Btn → **Play**，Pause_Btn → **Pause**，ResetStage_Btn → **ResetStage**

### 3.3 挂在 Player Pawn 上
把 `BP_SandboxMenuActor` 作为子组件挂到 Pawn，**可见性跟随 OnIntentChanged 切**。

---

## 步骤 4：PC 调试键（强烈建议加，无头显即可测）

> 编辑器内用键盘模拟手势，避免每次都要进头显验证。在 `BP_VRGameMode` 或玩家 Pawn 蓝图里：

| 输入动作 | 蓝图节点 | 调用 |
|---------|---------|------|
| **鼠标右键 drag** | Mouse X/Y 事件 → `SetActorLocation` 增加 XY | `TranslateSandboxXY((ΔX,ΔY))` |
| **鼠标滚轮** | Mouse Wheel → 累计 → `ScaleSandbox` | `ScaleSandbox(±0.05)` |
| **Q / E** | Key Q / Key E | `RotateSandbox(±2°)` |
| **1 / 2 / 3 / 4** | Key 1..4 | `SetStage(StageN)` |
| **R** | Key R | `ResetSandbox` |
| **空格** | Key Space | `Play` / `Pause`（按当前 bPlaying 反向） |

触发后立刻 **松开 Q / E / 鼠标右键** 才更新 — 一次性按下/松开。

---

## 已交付能力的快速清单

| 功能 (方案编号) | 状态 | 触发 |
|----------|------|------|
| F01 沙盘放置 (固定水平位置) | ✅ C++ | `AMRSandboxController` 构造里 SetActorLocation |
| F02 双手 Pinch 缩放 0.5~3x | ✅ C++ | `UMRSandboxHandInteractor` Tick 自动识别 |
| F03 双手 Pinch 旋转 Z 轴 360° | ✅ C++ | 同上 |
| F04 单手 Grab 平移 (XY) | ✅ C++ | 同上 |
| F05 食指射线命中 → 信息浮窗 | 🟡 C++ 已发委托 `OnEntityAimed`，**UMG 由你画** |
| F06 阶段切换 | ✅ C++ 接口 + 🟡 菜单 UMG 待画 |
| F07 播放/暂停 | ✅ C++ 接口 + 🟡 菜单按钮待画 |
| F08 重置 | ✅ C++ 接口 + 🟡 菜单按钮待画 + R 键可测 |
| A01 信息浮窗 | 🟡 UMG WidgetComponent 待画 |
| A02 透明度 | ❌ 待写（下一步迭代） |
| A03 危险热区高亮 | 🟡 Stage1 列表待你创建球体 |
| A04 阶段文字说明 | ❌ 待写 |
| ~~抓起物体~~ | ❌ 不做（方案 3.3 明确不做） |
| Passthrough / 平面检测 | ❌ 依赖 Xvisio SDK；当前 C++ 不涉及 |

---

## 编译期可能的常见错误

| 报错 | 修复 |
|------|------|
| `fatal error: XvisioOpenXR.h: No such file` | 确认 `MRSandbox.Build.cs` 里 `"XvisioOpenXR"` 在 PublicDependencyModuleNames |
| `unknown override: bHavePrevBothPinch` 等 | C++ 内部，不要从 BP 改这些字段 |
| `StageActors is not a TMap of ...` | 一定要在 C++ Class 里关联 key-value 类型，不要在 BP 里手动加没用 key |
| `GEngine->XRSystem 返回 null` | 启动 OpenXR HMD（PIE 模式或打包后会自动启用，纯编辑器不动 head 不会触发） |

---

## 测试节奏（建议）

1. **第一轮：PC 调试键** → 不戴头显，编辑器里直接走 1/2/3/4 + R 切阶段 + 滚轮缩放。最快验证状态机和 Clamp。
2. **第二轮：双手 Pinch 缩放** → 戴头显，确认 Pinch 灵敏度（参数可以微调 `PinchThreshold` 和 `ScaleSensitivity`）。
3. **第三轮：单手 Grab 平移** → 戴头显，确认 Grab 灵敏度（`GrabThreshold` / `TranslationSensitivity`）。
4. **第四轮：手腕菜单** → UMG 画完后一起测，验证 `OnIntentChanged` 和按钮回调。
5. **第五轮：4 阶段实体显隐** → 在 StageActors 里逐个 key 验证可见性切换。

遇到问题时优先看 `Saved/Logs/MR3.log`，手势订阅组件的 `bL/bR` 状态有 false 就是 Xvisio 手部追踪还没拿到，别急着改 C++。
