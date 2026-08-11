# MRSandboxRoot 使用说明

## 1. 编译

1. 右键 `E:/VSCC/test1/MR5/MR3.uproject` → **Generate Visual Studio project files**
2. 打开生成的 `MR3.sln`
3. 编译配置选 **Development Editor / Win64**，然后 Build
4. 编译成功后启动编辑器

## 2. 在关卡里创建沙盘

1. 把 `MRSandboxRoot`（在 Content Browser 的 C++ Classes / MR3 下）拖到场景里，**保存关卡**。完成。
2. 运行时（PIE / Standalone / 打包）BeginPlay 会**全自动**：
   - 若 `ChildActors` 为空 → 自动 `CollectFactoryActorsFromLevel()` 收集关卡里所有工厂 Actor
   - `AttachChildren()` 挂到根节点下
   - `SetActorScale3D(InitialScale)` 缩到 1/10
   - `AdjustLightsForScale()` 自动缩放点光源衰减半径
   - 放到玩家前方 1 m、桌面 0.8 m 高处
3. **编辑器里预览**（不运行就看效果）：选中 `MRSandboxRoot` → Details → Sandbox 三个按钮：
   - **Collect Factory Actors From Level**：手动收集（运行时会自动做，这步仅用于编辑器预览）
   - **Attach Children In Editor**：挂载并应用 `InitialScale`（编辑器预览用）
   - **Clear Child Actors**：清空

> 不想自动收集？提前手动 Collect + 保存关卡，`ChildActors` 非空时 BeginPlay 跳过自动收集，直接用你配置的。

## 3. 参数说明（选中 MRSandboxRoot 后在 Details 面板调整）

| 参数 | 默认值 | 含义 |
|------|--------|------|
| InitialScale | 0.1 | 初始缩放（1/10）。BeginPlay 自动应用，进 PIE 即缩好，无需手动调 |
| InitialOffset | (100, 0, 0) | 相对玩家前方向偏移（cm） |
| HoverHeight | 10 | 悬浮高度（cm） |
| ScaleSensitivity | 2 | 双手捏合缩放灵敏度 |
| RotationSensitivity | 1 | 双手旋转灵敏度 |
| TranslationSensitivity | 1 | 单手拖拽平移灵敏度 |
| PinchThreshold | 2 | 拇指食指距离小于此值算捏合（cm） |
| GrabThreshold | 4 | 四指离掌心距离均小于此值算握拳（cm） |
| MinScale | 0.5 | 最小额外缩放倍数（相对于 InitialScale） |
| MaxScale | 3 | 最大额外缩放倍数（相对于 InitialScale） |

## 4. 阶段切换

- `Stage1Actors` ~ `Stage4Actors`：把对应阶段的 Actor 拖进去
- 运行中用蓝图调用 `SetStage(1)` ~ `SetStage(4)` 切换
- 也可以用键盘测试：后续可在任意 Pawn/PlayerController 里绑定 `ResetSandbox()` 和 `SetStage()`

## 5. 手势说明

| 手势 | 效果 |
|------|------|
| 双手拇指+食指捏合，拉开/合拢 | 放大 / 缩小沙盘 |
| 双手捏合后左右旋转 | 绕垂直轴旋转沙盘 |
| 单手握拳（四指贴掌心） | 水平拖拽平移沙盘 |
| 右手食指伸直，其余四指握拳 | 从食指指尖发出射线，命中 Actor 会存到 `AimedActor` |

## 6. PC 调试键（C++ 自动绑，无需画蓝图）

`AMRSandboxRoot` 在 BeginPlay 时自动通过 `InputComponent->BindKey` 绑定物理键，**不依赖 Enhanced Input Mapping**，PIE / Standalone 立即可用。无头显也能测缩放 / 旋转 / 阶段切换。

| 按键 | 效果 | 调用 |
|------|------|------|
| 鼠标滚轮上 / 下 | 沙盘 ±0.05 缩放 | `ScaleSandbox(±0.05)` |
| Q / E | 沙盘绕垂直轴 ∓2° 旋转 | `RotateSandbox(∓2)` |
| 1 / 2 / 3 / 4 | 切到阶段 1~4 | `SetStage(N)` |
| R | 重置到初始姿态 | `ResetSandbox()` |

> 滚轮步长 `ScrollScaleStep` 与旋转步长 `KeyRotateStep` 是 `static constexpr`，改源码后重编译生效。
> 蓝图 / 其他 Actor 也可直接调用 `ScaleSandbox` / `RotateSandbox` / `TranslateSandboxXY` 三个 BlueprintCallable（例如把鼠标右键 drag 接到 `TranslateSandboxXY`）。

## 7. 灯光缩放（必须单独处理）

**原因**：缩放根 Actor 时，子物体的**位置 / 缩放**会跟随，但光源的 `AttenuationRadius`（衰减半径）是绝对值，**不会跟着缩**。缩到 0.015 后，原来 500 cm 的点光源照 7.5 cm，相对场景反而变成"巨灯"，光照全错。

**处理**：缩放后选中 `MRSandboxRoot` → Details → **Sandbox|Lights** → 点 **Adjust Lights For Scale**：
- 首次调用会缓存所有 `ChildActors` 下 `UPointLightComponent`（含 SpotLight）的原始 `AttenuationRadius`，再按当前 `InitialScale` 缩放。
- 之后每次手势 / 滚轮缩放完，再点一次会用缓存的原值 × 当前 Scale 重新应用（不会累积误差）。
- **Restore Light Radii** 还原所有缓存光源到原始值。

**不需要调的光源**：
- `DirectionalLight`（平行光，无衰减半径，全场照射）
- `SkyLight` / `SkyAtmosphere`（半球采集，与缩放无关）

这些已经在 `CollectFactoryActorsFromLevel` 的排除名单里，不会被 Attach 到沙盘根。

## 8. 动画 / 移动的相对位置约定

后续给场景里的 Actor（消防车、无人机、伤员等）加动画时，**所有位移必须用相对父节点的 Local 坐标**，不要用世界坐标。

| 做法 | 结果 |
|------|------|
| ✅ `AddActorLocalOffset` / Timeline 驱动 Relative Location | 沙盘缩放 / 旋转 / 平移时，动画自动跟随，比例正确 |
| ❌ `SetActorLocation(世界坐标)` / `AddActorWorldOffset` | 沙盘一动，动画就"飞出去"，相对位置全乱 |

蓝图里：用 `Set Relative Location` 节点，或在 Actor 已 Attach 到 `MRSandboxRoot` 的前提下用 `Add Actor Local Offset`。

## 9. 常见问题

- **收集时漏了某个 Actor**：把它的名字告诉我，我加进排除名单；或者你手动拖到 `ChildActors` 里
- **场景还是太大/太小**：调 `InitialScale`
- **手势没反应**：戴上 Xvisio 眼镜并确认 OpenXR runtime 已启动；PC 上没眼镜时手势自然无反应
