# MR3 沙盘系统 — 技术文档

> 适用版本：UE 5.4 / 工程 MR3（仓库目录 MR5）/ Xvisio Seer Pad ONE 头显  
> 文档目的：让接手的人能看懂整个沙盘系统是怎么搭起来的、为什么这么搭、踩过哪些坑。



---

## 1. 项目背景

MR3（仓库目录 MR5）是北航"沉浸式生产实习"项目的一个最小 Demo：在 Xvisio Seer Pad ONE 头显上以混合现实（MR）方式呈现"熔融金属泄漏"事故的四阶段应急演练。

工厂场景原始约 100 m × 80 m，玩家戴上头显后看到的是这个工厂的"桌面沙盘"版本——**整体缩到 1/10**，自动移到玩家前方约 3 m 处，可以用裸手手势或键鼠对沙盘进行缩放、阶段切换。

工程目录是 `E:\VSCC\test1\MR5\`，UE 工程名是 `MR3.uproject`，C++ 游戏模块是 `MR3`（`Source/MR3/`）。

---

## 2. 核心功能一览

| 功能                        | 触发方式            | 实现位置                                   |
| ------------------------- | --------------- | -------------------------------------- |
| 一进去就缩放（1/10）并定位到玩家前方      | 自动（无需手动放 Actor） | `MRSandboxRoot::BeginPlay`             |
| 鼠标滚轮缩放 ±5%                | 滚轮              | `MRSandboxRoot::ScaleSandbox`          |
| 双手拇指食指捏合（Pinch）拉开/合拢 → 缩放 | 手势              | `MRSandboxRoot::UpdateHandInteraction` |
| 单手握拳（Grab）→ 水平平移          | 手势              | 同上                                     |
| 双手 Pinch 旋转 Z 轴           | 手势              | 同上                                     |
| 右手食指伸出 → 射线命中 Actor       | 手势              | `IsIndexExtended` + `LineTrace`        |
| 1/2/3/4 切阶段               | 键盘              | `MRSandboxRoot::SetStage`              |
| Q/E 旋转                    | 键盘              | `MRSandboxRoot::RotateSandbox`         |
| R 重置                      | 键盘              | `MRSandboxRoot::ResetSandbox`          |
| 点光源衰减半径自动跟随缩放             | 自动              | `MRSandboxRoot::AdjustLightsForScale`  |

---

## 3. 系统架构

### 3.1 技术栈

| 层      | 技术                                                                  |
| ------ | ------------------------------------------------------------------- |
| 引擎     | Unreal Engine 5.4（Forward Shading + Lumen GI + Virtual Shadow Maps） |
| 模板     | VRTemplate（`/Game/VRTemplate/Blueprints/VRGameMode.VRGameMode_C`）   |
| XR 标准  | OpenXR（`XR_EXT_hand_tracking` 扩展）                                   |
| 手部追踪   | UE5 原生 `IHandTracker` 接口（与 `BP_TrackedHands` 同源）                    |
| 头显     | Xvisio Seer Pad ONE（通过 `XvisioOpenXR` 插件接入 + `xvisio_streamer` 串流）  |
| C++ 模块 | MR3（项目模块）                                                           |

### 3.2 模块与文件

```
MR5/                                  # 仓库根
├── MR3.uproject                      # UE 工程描述（含 Modules 字段）
├── MR3.sln                           # VS 解决方案
├── Source/MR3/
│   ├── MR3.Build.cs                  # 依赖：Core/CoreUObject/Engine/InputCore/HeadMountedDisplay/XvisioOpenXR
│   ├── MR3.Target.cs                  # Game target
│   ├── MR3Editor.Target.cs            # Editor target
│   ├── Public/
│   │   └── MRSandboxRoot.h           # 沙盘 Actor 声明（参数 + 状态 + 接口）
│   ├── Private/
│   │   ├── MRSandboxRoot.cpp          # 沙盘核心实现（700+ 行）
│   │   └── MR3Module.cpp              # 模块入口（IMPLEMENT_MODULE + World 委托注册）
├── Config/
│   ├── DefaultEngine.ini              # 默认地图 VRTemplateMap、渲染设置
│   ├── MRSandbox_Handoff.md           # 编辑器侧接线手册
│   └── MRSandboxRoot_Handoff.md       # MRSandboxRoot 使用说明
└── Plugins/XvisioOpenXR/              # Xvisio 头显插件（OpenXR 接入 + 串流）
```

### 3.3 核心类

**`AMRSandboxRoot`**：整个沙盘系统的核心 Actor。

职责：

1. 自动收集关卡里的工厂 Actor（`CollectFactoryActorsFromLevel`）
2. BeginPlay 计算场景中心 + 目标中心并缩放（`ApplySandboxScale`）
3. 每 Tick 读取手部关节数据，识别 Pinch/Grab 手势（`UpdateHandInteraction`）
4. 响应 PC 调试键（滚轮/键盘）
5. 阶段切换时控制对应 Actor 显隐（`SetStage`）

**`FMR3Module`**：模块入口。

职责：在 `StartupModule` 注册 `FWorldDelegates::OnPostWorldInitialization`，让任何 Game/PIE World 初始化时自动 Spawn 一个 `AMRSandboxRoot`（用户无需手动放 Actor）。

---

## 4. 关键技术原理

### 4.1 UE 模块加载机制（最关键、最容易踩坑）

**为什么 `.uproject` 必须有 `Modules` 字段？**

UE 项目模块的加载由 `.uproject` 控制。如果 `.uproject` **没有 `Modules` 字段**，UE 默认把项目当**蓝图项目**，**不加载任何 C++ 模块**——`dll` 文件虽然在磁盘上，但不被 `LoadLibrary`，模块的 UClass 不注册到反射系统，`StartupModule` 永远不会被调用。

正确配置（缺它一切不工作）：

```json
{
    "Modules": [
        {
            "Name": "MR3",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ]
}
```

**为什么还需要 `IMPLEMENT_MODULE`？**

UE5 中每个模块必须在某个 `.cpp` 中有 `IMPLEMENT_MODULE(FModuleClass, ModuleName)`，作为模块入口。如果 UBT 没自动生成（我们项目里 grep 全树都没找到自动生成的入口），必须**手动添加**。

加载路径：

```
FModuleManager::LoadModule
  → dll 导出 InitializeModule
  → 构造 GModuleClass（FMR3Module 实例）
  → 调 GModuleClass.StartupModule()
```

**不走 CRT 静态构造**——这是大坑！文件级 `static struct { ... } GVar;` 在 UE 模块 dll 中**不会执行**（因为 UE 模块 dll 不调标准 CRT 初始化）。所以委托注册**不能**写在文件级 static 构造里，必须写在 `StartupModule` 里。

模板：

```cpp
// MR3Module.cpp
class FMR3Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FWorldDelegates::OnPostWorldInitialization.AddLambda(
            [](UWorld* World, const UWorld::InitializationValues IVS)
            {
                AMRSandboxRoot::OnPostWorldInit(World, IVS);
            });
    }
};
IMPLEMENT_MODULE(FMR3Module, MR3)
```

### 4.2 Actor 自 Spawn（无需手动放）

**目标**：用户不想在编辑器里把 `AMRSandboxRoot` 拖到关卡，希望进游戏自动有一个实例。

**实现**：在 `StartupModule` 注册 `FWorldDelegates::OnPostWorldInitialization`，当任何 Game/PIE World 初始化时自动 Spawn。

```cpp
void AMRSandboxRoot::OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
    if (!World) return;
    // 只在游戏/PIE World 里 Spawn（不在编辑器预览 World 里）
    if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE) return;

    // 如果关卡已有实例（用户手动放了），不重复 Spawn
    TArray<AActor*> Existing;
    UGameplayStatics::GetAllActorsOfClass(World, AMRSandboxRoot::StaticClass(), Existing);
    if (Existing.Num() > 0) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    World->SpawnActor<AMRSandboxRoot>(AMRSandboxRoot::StaticClass(), FTransform::Identity, Params);
}
```

### 4.3 缩放方案：scale-in-place（避开 Static/Movable 限制）

**核心问题**：UE 中 **Static Actor 不能 Attach 到 Movable Root**。缩放整个场景需要把所有 Actor 集中到一个"根"下统一缩放，但：

- 关卡默认 Actor（Brush / StaticMesh / NavMesh）都是 **Static**（Mobility = Static）
- `AMRSandboxRoot` 因为有 `Tick` + `BeginPlay` SetActorLocation 是 **Movable**
- 错误：*"附加到 MRSandboxRoot.Root 并非静态，无法对其附加静态的 BrushComponent0"*

**替代方案：scale-in-place**

不对 Actor 做 Attach，而是在每个 Actor 原位置**直接修改 Scale 和 Location**：

```cpp
NewLocation = SandboxTargetCenter + (OriginalLocation - SandboxSceneCenter) * CurrentScale;
NewScale    = OriginalScale * CurrentScale;
Child->SetActorScale3D(NewScale);
Child->SetActorLocation(NewLocation);
```

这样所有 Actor 保持独立，但视觉上按比例缩小，且整体移到 `SandboxTargetCenter`。

### 4.4 运行时改 Mobility

**问题**：scale-in-place 需要 `SetActorLocation`，但 Static Actor 的 `SetActorLocation` 静默失败（UE 只警告不执行），Actor 不动——场景"东一块西一块"漂浮在远处。

**解决**：在缩放前，遍历 ChildActor 的所有 SceneComponent，运行时调用 `SetMobility(EComponentMobility::Movable)`：

```cpp
TArray<USceneComponent*> Comps;
Child->GetComponents<USceneComponent>(Comps);
for (USceneComponent* C : Comps)
{
    if (C && C->Mobility != EComponentMobility::Movable)
    {
        C->SetMobility(EComponentMobility::Movable);
    }
}
```

**副作用**：光照缓存（Lightmap）失效——缩放后光照可能错位。对于原型可接受。

### 4.5 场景中心 + 目标中心定位

**`SandboxSceneCenter`**：所有 ChildActor 原始位置的几何平均（场景几何中心）。  
**`SandboxTargetCenter`**：缩放后场景应该出现的位置（玩家前方 3 m、低于眼睛 80 cm）。

**位置公式**：

```
NewLocation = SandboxTargetCenter + (OriginalLocation - SandboxSceneCenter) × CurrentScale
```

效果：

- 缩放后场景中心在 `SandboxTargetCenter`（玩家前方可见位置）
- 每个 Actor 相对中心按比例缩放（保持原始相对布局）

**为什么用 `GetPlayerViewPoint` 而不是 `GetPawn`？**

```cpp
// PIE Standalone 启动时 Pawn 可能还没 Spawn，GetPawn() 返回 nullptr
APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
APawn* Pawn = PC ? PC->GetPawn() : nullptr;  // 可能 nullptr！

// GetPlayerViewPoint 更可靠——相机位置总是可用的
FVector CamLoc; FRotator CamRot;
PC->GetPlayerViewPoint(CamLoc, CamRot);
```

最终 TargetCenter：

```cpp
SandboxTargetCenter = AnchorLoc + CamRot.Vector() * SandboxForwardDistance
                    + FVector(0, 0, SandboxHeightOffset);
// AnchorLoc = CamLoc
// SandboxForwardDistance = 300 (cm)
// SandboxHeightOffset = -80 (cm)
```

### 4.6 手势识别（OpenXR 原生 IHandTracker）

**问题**：Xvisio 插件的 `UXvisioOpenXR::GetHandJointExtraData` 内部调用 `xrLocateHandJointsEXT` 失败（`ReceivedJointPoses = false`），但 `BP_TrackedHands` 用 OpenXR 原生 API 能正常追踪。

**解决**：改用 UE5 标准 `IHandTracker` 接口（与 `BP_TrackedHands` 同源）：

```cpp
IHandTracker* HandTracker = IModularFeatures::Get()
    .GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());

if (HandTracker && HandTracker->IsHandTrackingStateValid())
{
    TArray<FVector> Positions;
    TArray<FQuat> Rotations;
    TArray<float> Radii;
    HandTracker->GetAllKeypointStates(EControllerHand::Right, Positions, Rotations, Radii);
    // Positions[Palm], Positions[ThumbTip], Positions[IndexTip], ...
}
```

**关键点索引**（`EHandKeypoint` 枚举）：

| 关节        | 索引 |
| --------- | -- |
| Palm      | 0  |
| ThumbTip  | 5  |
| IndexTip  | 10 |
| MiddleTip | 15 |
| RingTip   | 20 |
| LittleTip | 25 |

**手势判定阈值**：

- **Pinch（捏合）**：拇指 + 食指距离 < `PinchThreshold`（默认 2 cm）
- **Grab（握拳）**：四指到掌心距离均 < `GrabThreshold`（默认 4 cm）

**手势交互逻辑**（在 `UpdateHandInteraction` 的 Tick 里）：

| 手势组合            | 效果                                     |
| --------------- | -------------------------------------- |
| 双手 Pinch        | 测两手拇指/食指中点距离（Span），变化驱动缩放；两手连线角度变化驱动旋转 |
| 单手 Grab         | 测掌心位置变化，驱动水平平移                         |
| 右手食指伸出 + 其余四指握拳 | 从食指指尖发出射线，命中 Actor 写入 `AimedActor`     |

### 4.7 PC 调试键（不戴头显也能测）

为了让没有头显时也能调试，用 `InputComponent->BindKey` 直接绑物理键，**不依赖 Enhanced Input Mapping**：

```cpp
EnableInput(PC);
InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AMRSandboxRoot::OnScrollUp);
InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AMRSandboxRoot::OnKeyQ);
InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AMRSandboxRoot::OnKeyR);
// ...
```

按键映射：

| 按键            | 功能     | 内部调用                  |
| ------------- | ------ | --------------------- |
| 鼠标滚轮上/下       | 缩放 ±5% | `ScaleSandbox(±0.05)` |
| Q / E         | 旋转 ∓2° | `RotateSandbox(∓2)`   |
| 1 / 2 / 3 / 4 | 切到阶段 N | `SetStage(N)`         |
| R             | 重置沙盘   | `ResetSandbox()`      |

### 4.8 灯光衰减半径单独处理

**问题**：缩放 Actor **不影响光源的 `AttenuationRadius`**（这是绝对值）。500 cm 半径的点光源，缩到 0.1 倍后仍照 500 cm，相对场景变成"巨灯"——光照全错。

**解决**：遍历 ChildActor 的所有 PointLight/SpotLight，缓存原始 `AttenuationRadius`，按 `CurrentScale` 缩放：

```cpp
void AMRSandboxRoot::AdjustLightsForScale()
{
    const float ScaleMul = CurrentScale;

    if (!bLightsCached)
    {
        // 首次：缓存所有 PointLight/SpotLight 的原始 AttenuationRadius，按 CurrentScale 应用
        for (AActor* Child : ChildActors)
        {
            TArray<ULightComponent*> Lights;
            Child->GetComponents<ULightComponent>(Lights, false);
            for (ULightComponent* Light : Lights)
            {
                UPointLightComponent* PL = Cast<UPointLightComponent>(Light);
                if (PL)
                {
                    FLightCache Cache{ PL, PL->AttenuationRadius };
                    CachedLights.Add(Cache);
                    PL->SetAttenuationRadius(PL->AttenuationRadius * ScaleMul);
                }
            }
        }
        bLightsCached = true;
    }
    else
    {
        // 再次：用缓存的原值 × 当前 Scale 重新应用（避免累积误差）
        for (const FLightCache& Cache : CachedLights)
        {
            if (UPointLightComponent* PL = Cast<UPointLightComponent>(Cache.Comp.Get()))
            {
                PL->SetAttenuationRadius(Cache.OriginalRadius * ScaleMul);
            }
        }
    }
}
```

**不处理的光源**：

- `DirectionalLight`（平行光，无 AttenuationRadius，全场照射）
- `SkyLight` / `SkyAtmosphere`（半球采集，与缩放无关）

这些在 `CollectFactoryActorsFromLevel` 的排除名单里。

### 4.9 四阶段应急演练（Actor 显隐）

`SetStage(1..4)` 切换对应阶段 Actor 的可见性：

```cpp
void AMRSandboxRoot::UpdateStageVisibility()
{
    auto SetVisibility = [this](TArray<AActor*>& Actors, bool bVisible)
    {
        for (AActor* A : Actors)
        {
            if (A) A->SetActorHiddenInGame(!bVisible);
        }
    };
    SetVisibility(Stage1Actors, CurrentStage == 1);
    SetVisibility(Stage2Actors, CurrentStage == 2);
    SetVisibility(Stage3Actors, CurrentStage == 3);
    SetVisibility(Stage4Actors, CurrentStage == 4);
}
```

**当前关卡未填 StageActors 数组**——阶段实体（熔融金属流、热区球、无人机、消防车、急救医生等）需要在编辑器 Details 面板填进对应数组。当前阶段切换只切换当前 stage 显示/隐藏（如果数组填了的话）。

---

## 5. 关键文件

| 文件                                     | 作用                                                | 行数   |
| -------------------------------------- | ------------------------------------------------- | ---- |
| `MR3.uproject`                         | 工程描述（含 Modules 字段——必须有）                           | ~60  |
| `Source/MR3/MR3.Build.cs`              | 模块依赖（Core/Engine/HeadMountedDisplay/XvisioOpenXR） | ~25  |
| `Source/MR3/Private/MR3Module.cpp`     | 模块入口（IMPLEMENT_MODULE + World 委托注册）               | ~35  |
| `Source/MR3/Public/MRSandboxRoot.h`    | 沙盘 Actor 头文件（参数 + 状态 + 接口）                        | ~175 |
| `Source/MR3/Private/MRSandboxRoot.cpp` | 沙盘核心实现（缩放、手势、灯光、阶段、调试键）                           | ~560 |
| `Config/MRSandboxRoot_Handoff.md`      | 使用说明 + 参数表                                        | ~80  |

---

## 6. 踩过的坑（最有价值的部分）

这一节列实际开发中遇到的问题、原因、解决。**下次遇到类似症状直接查这里**。

### 坑 1：模块根本没加载（症状：Content Browser / Class Viewer 找不到 AMRSandboxRoot）

- **症状**：Class Picker 偶尔看到一次（缓存），Class Viewer 永远搜不到，Place Actors 找不到
- **日志**：`MR3: FMR3Module::StartupModule called` 不出现
- **根因**：`.uproject` **缺 `Modules` 字段**，UE 把项目当蓝图项目不加载 dll
- **解决**：加 Modules 字段（见 §4.1）
- **为什么我反复犯**：改了 .uproject 后必须**完全关闭编辑器**重新打开才能生效，缓存的"没加载"状态会持续误导

### 坑 2：FWorldDelegates.AddStatic 编译失败

- **症状**：`error: 'AddStatic' is not a member of 'TMulticastDelegate<...>'`
- **根因**：UE5 的 multicast delegate **不支持静态函数绑定**（因为 multicast 用 weak ptr 跟踪）
- **解决**：用 `AddLambda` 包装

### 坑 3：文件级 static lambda 不执行

- **症状**：dll 里有 `static struct { ... } GVar;`，模块加载时不跑，委托没注册
- **根因**：UE 模块 dll **不走 CRT 静态构造**（用 `FModuleManager::LoadModule` → `InitializeModule` 路径）
- **解决**：用 `IMPLEMENT_MODULE` + `StartupModule` 替代文件级 static

### 坑 4：Static Actor 不能 Attach 到 Movable Root

- **症状**：大量 `附加到 MRSandboxRoot.Root 并非静态，无法对其附加静态的 BrushComponent0` 错误
- **根因**：UE Mobility 规则——Static Actor 只能 Attach 到 Static Root
- **解决**：放弃 Attach，改 scale-in-place + 运行时 SetMobility

### 坑 5：SetActorLocation 在 Static Actor 上静默失败

- **症状**：scale-in-place 调用了 SetActorLocation，但 Actor 没移动，"东一块西一块"
- **根因**：UE 对 Static Actor 的 SetActorLocation 只警告不执行
- **解决**：scale-in-place 前对所有 SceneComponent 调 `SetMobility(EComponentMobility::Movable)`

### 坑 6：Xvisio 手部 API 返回空

- **症状**：`Tick bL=0 bR=0`，永远拿不到关节数据，但 BP_TrackedHands 手部模型能动
- **根因**：Xvisio `HandJointExtraPlugin` 内部 `xrLocateHandJointsEXT` 失败（`ReceivedJointPoses = false`）
- **解决**：改用 OpenXR 原生 `IHandTracker`（与 BP_TrackedHands 同源）
- **教训**：插件自带的 API 不一定工作，要直接用 UE 标准接口（IModularFeatures::GetModularFeature）

### 坑 7：TargetCenter 依赖 GetPawn 导致场景消失

- **症状**：场景完全看不到（移到世界原点，玩家在 PlayerStart 看不到）
- **根因**：PIE Standalone 启动时 `PC->GetPawn()` 返回 nullptr（Pawn 还没 Spawn），Location 保持默认 (0,0,0)
- **解决**：用 `GetPlayerViewPoint`（相机位置更可靠）+ Fallback 默认 (0,0,0)

### 坑 8：TargetCenter 在相机**上方**导致看不到

- **症状**：场景移到玩家头顶上方 2 m，相机朝前看不到
- **根因**：相机默认朝前（+X 方向），不在上方
- **解决**：TargetCenter = 相机**前方** × SandboxForwardDistance + **下方** × SandboxHeightOffset

### 坑 9：PIE 卡在"在VR环境中运行该关卡"对话框

- **症状**：PIE 启动后画面不动，World 没初始化，OnPostWorldInit 不触发
- **根因**：VR 确认对话框没点确认/取消
- **解决**：必须点确认或取消，PIE 才能继续

### 坑 10：编辑器还开着时编译被 Live Coding 拦截

- **症状**：`Unable to build while Live Coding is active`
- **解决**：完全关闭编辑器（任务管理器确认 `UnrealEditor.exe` 没了）再编译

---

## 7. 运行与调试

### 7.1 编译

```bash
# 编辑器必须完全关闭（Live Coding 会拦截）
"E:/software/UE/UE_5.4/Engine/Build/BatchFiles/Build.bat" \
    MR3Editor Win64 Development \
    -Project="E:/VSCC/test1/MR5/MR3.uproject"
```

增量编译约 5-10 秒。从零重编约 20-40 分钟。

### 7.2 运行

1. 双击 `MR3.uproject` 启动 UE 编辑器
2. 编辑器完全加载后按 **Play（PIE）**
3. 弹出"**在VR环境中运行该关卡**"对话框 → **必须点确认或取消**（不点会卡住）
4. 进游戏后场景自动缩放到 1/10、移到玩家前方 3 m、低于眼睛 80 cm

### 7.3 PC 调试键（无需头显）

| 按键            | 功能       |
| ------------- | -------- |
| 鼠标滚轮上 / 下     | 缩放 ±5%   |
| Q / E         | 旋转 ∓2°   |
| 1 / 2 / 3 / 4 | 切到阶段 1~4 |
| R             | 重置沙盘     |

### 7.4 日志诊断

关键 log 在 `Saved/Logs/MR3.log`：

```
LogTemp: Warning: MR3: FMR3Module::StartupModule called                                       ← 模块加载成功
LogTemp: Warning: MRSandboxRoot: BeginPlay ChildActors=N SceneCenter=... AnchorLoc=...        ← 沙盘初始化
LogTemp: Warning: MRSandboxRoot: Tick bL=1 bR=1 pinch(L=0 R=0) grab(L=0 R=0) ...             ← 手部数据
LogTemp: Warning: MRSandboxRoot: ScaleSandbox delta=0.050000 old=0.100000 new=0.150000       ← 滚轮触发
```

**诊断速查表**：

| log 现象                          | 原因                   | 修复                                    |
| ------------------------------- | -------------------- | ------------------------------------- |
| `StartupModule called` 不出现      | 模块没加载                | 查 `.uproject` Modules 字段              |
| `BeginPlay ...` 不出现             | Spawn 失败             | 查 `OnPostWorldInit` 日志                |
| `Tick bL=0 bR=0` 永远             | 手部 API 没数据           | 已用 IHandTracker，应工作                   |
| `Tick bL=1 bR=1 pinch=0 grab=0` | 阈值不满足                | 调小 `PinchThreshold` / `GrabThreshold` |
| `ScaleSandbox` log 不出现          | 滚轮没绑到 InputComponent | 查 `SetupPCDebugInput`                 |

---

## 8. 参数说明

| 参数                              | 默认值     | 单位                         | 含义                      |
| ------------------------------- | ------- | -------------------------- | ----------------------- |
| `InitialScale`                  | 0.1     | 倍数                         | 初始缩放（1/10）              |
| `SandboxHeightOffset`           | -80     | cm                         | 场景中心低于相机的高度（负=低）        |
| `SandboxForwardDistance`        | 300     | cm                         | 场景中心在相机前方的距离            |
| `PinchThreshold`                | 2       | cm                         | 拇指食指距离 < 此值算捏合          |
| `GrabThreshold`                 | 4       | cm                         | 四指到掌心距离均 < 此值算握拳        |
| `MinScale` / `MaxScale`         | 0.5 / 3 | 倍数                         | 相对 InitialScale 的额外缩放倍数 |
| `ScaleSensitivity`              | 2       | 倍数                         | 双手 Pinch 缩放灵敏度          |
| `RotationSensitivity`           | 1       | 倍数                         | 双手 Pinch 旋转灵敏度          |
| `TranslationSensitivity`        | 1       | 倍数                         | 单手 Grab 平移灵敏度           |
| `ScrollScaleStep`               | 0.05    | 倍数                         | 滚轮缩放步长                  |
| `KeyRotateStep`                 | 2       | 度                          | Q/E 旋转步长                |
| `Stage1Actors` ~ `Stage4Actors` | 空       | 各阶段显示的 Actor 数组（Details 填） |                         |

修改 `InitialScale` / `SandboxHeightOffset` / `SandboxForwardDistance` 后需要重新编译（UPROPERTY 默认值在 C++ 编译时确定）。

---

## 9. 后续可扩展

按优先级：

- [ ] **手势旋转/平移沙盘**：当前手势只支持缩放，旋转/平移需要 `ApplySandboxScale` 也旋转/平移每个 Actor
- [ ] **手部方块修复**：`BP_TrackedHands` 蓝图里的 Box Collision 组件（默认显示且尺寸大），需要在蓝图里 `Hidden In Game = true`
- [ ] **阶段实体**：在 `Stage1Actors`~`Stage4Actors` 填熔融金属流、热区球、无人机、消防车、急救医生等
- [ ] **手腕菜单 UMG**：3D WidgetComponent 显示在玩家手腕上方，阶段按钮 + 播放控制
- [ ] **信息浮窗**：食指射线命中 Actor 后显示该 Actor 信息（`AimedActor` 已识别）
- [ ] **静态光照重建**：运行时改 Mobility 导致 Lightmap 失效，正式版本应该重新构建光照
- [ ] **手部模型适配**：当前手部模型小幅度动（Xvisio 关节精度），调整 BP_TrackedHands 的关节数据源/平滑参数

---

## 10. 引用

- UE5 模块加载机制：`FModuleManager::LoadModule` → `InitializeModule` → `StartupModule`
- OpenXR 手部追踪：`XR_EXT_hand_tracking` 扩展 + `IHandTracker` 接口（`HeadMountedDisplay/Public/IHandTracker.h`）
- UE Mobility 规则：`Static` Actor 不能 Attach 到 `Movable` Root
- World 委托：`FWorldDelegates::OnPostWorldInitialization`（World 初始化后触发）
- 委托注册：`IModularFeatures::Get().IsModularFeatureAvailable + GetModularFeature<T>()`（返回引用，用 `&` 转指针）
