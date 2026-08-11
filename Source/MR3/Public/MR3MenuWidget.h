// Copyright (c) Yuquan Sun. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MR3MenuWidget.generated.h"

class AMRSandboxRoot;

/** 折叠组里的一项，对应截图里的 "构建所有关卡 / 仅构建光照 / 光照质量 ..."。 */
USTRUCT(BlueprintType)
struct MR3_API FMR3MenuItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Shortcut;   // 右侧灰字（"CTRL+SHIFT+分号"）
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemId;     // 唯一 ID，回调里用它分支
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasSubmenu = false; // 右侧显示 > 箭头
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsToggle   = false; // 复选框样式
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bToggleOn   = false; // 复选框初始
};

/** 一个分类（截图里的 "关卡 / 光照 / 反射 / 可视性 / 几何体 / 导航 / 层级LOD / 纹理流送 ..."）。 */
USTRUCT(BlueprintType)
struct MR3_API FMR3MenuGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bExpanded = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMR3MenuItem> Items;
};

/** 顶部 Tab（截图里的 "构建 / 选择 / Actor / 帮助"）——Demo 里改名对应本项目四阶段。 */
USTRUCT(BlueprintType)
struct MR3_API FMR3MenuTab
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TabId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMR3MenuGroup> Groups;
};

/**
 * UMG 基类。负责：
 *   - 持有菜单数据（Tabs），蓝图里能用 Designer 把它绑定到 VerticalBox/Switcher。
 *   - 暴露 OnMenuItemClicked 事件给蓝图，由子蓝图把它接到垂直栈生成。
 *   - 暴露 OnExpandToggled / OnTabSwitched 事件。
 */
UCLASS(Abstract, Blueprintable, ClassGroup = "MR3")
class MR3_API UMR3MenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UMR3MenuWidget(const FObjectInitializer& ObjectInitializer);

    /** 全部菜单数据。蓝图可在构造时填充，或运行时调用 SetTabs 注入。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    TArray<FMR3MenuTab> Tabs;

    /** 当前激活的 Tab 索引。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    int32 ActiveTabIndex = 0;

    /** 搜索关键词（外部更新会触发 RefreshList）。 */
    UPROPERTY(BlueprintReadWrite, Category = "MR3|UI")
    FString FilterText;

    /** 整体刷新列表（蓝图子类通常在此清空 VerticalBox 后重新 Spawn 行）。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void RequestRefresh();

    /** Tab 被点击。蓝图里可把它接到 Button.OnClicked 后调用。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void HandleTabClicked(int32 NewTabIndex);

    /** 大标题（Group）被点击。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void HandleGroupToggled(int32 TabIndex, int32 GroupIndex);

    /** 子项被点击。蓝图里接到 OnClicked 后调用该函数。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void HandleItemClicked(FName ItemId);

    /** 子项是 Toggle 时，翻转开关。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void HandleItemToggle(FName ItemId, bool bNewState);

    /** 蓝图在调 RequestRefresh 时被调用来做真正生成。 */
    UFUNCTION(BlueprintImplementableEvent, Category = "MR3|UI")
    void OnRefreshRequested();

    /** 蓝图子项点击后回调（蓝图可在这里再广播到 BP_MRSandboxMenuController）。 */
    UFUNCTION(BlueprintImplementableEvent, Category = "MR3|UI")
    void OnMenuItemClicked(FName ItemId);

    /** 蓝图 Tab 切换后回调。 */
    UFUNCTION(BlueprintImplementableEvent, Category = "MR3|UI")
    void OnTabSwitched(int32 NewTabIndex);

    /** 蓝图 Group 折叠切换后回调。 */
    UFUNCTION(BlueprintImplementableEvent, Category = "MR3|UI")
    void OnGroupToggled(int32 TabIndex, int32 GroupIndex, bool bNewExpanded);

    /** 设置绑定的沙盘根（可选；C++ 控制台里通过它访问 SetStage/Play 等）。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void SetSandboxRoot(AMRSandboxRoot* InRoot);

    UFUNCTION(BlueprintPure, Category = "MR3|UI")
    AMRSandboxRoot* GetSandboxRoot() const { return SandboxRoot.Get(); }

protected:
    UPROPERTY(Transient) TWeakObjectPtr<AMRSandboxRoot> SandboxRoot;
};
