// Copyright (c) Yuquan Sun. All rights reserved.
// 纯 C++ 屏幕空间 UMG 面板（事故阶段 / 播放控制 / 显示选项 / 手势帮助）。

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MR3PanelWidget.generated.h"

class AMRSandboxRoot;
class UMRHandUIInteractor;
class UButton;
class UScrollBox;
class UBorder;

UCLASS()
class MR3_API UMR3PanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "MR3")
    void SetupSandboxRefs(AMRSandboxRoot* InSandboxRoot, UMRHandUIInteractor* InInteractor);

    UFUNCTION(BlueprintCallable, Category = "MR3")
    void SetMouseDebugMode(bool bEnabled);

    /** 屏幕坐标点击命中（供 FaceLocked 层的手部射线交互调用）。
     *  ScreenPos 是 WidgetDrawSize（1920×1080）布局空间坐标，左上角为原点。
     *  命中某个按钮则触发它的 OnClicked 并返回 true。 */
    UFUNCTION(BlueprintCallable, Category = "MR3")
    bool HandleScreenTap(FVector2D ScreenPos);

protected:
    /** 关键：必须在 RebuildWidget 阶段构建 UI 树，此时 WidgetTree->RootWidget 才会被 TakeWidget。 */
    virtual TSharedRef<SWidget> RebuildWidget() override;

    void BuildLayout();
    void RefreshContent();
    void SwitchTab(int32 Tab);

    UFUNCTION() void Tab_Stage();
    UFUNCTION() void Tab_Playback();
    UFUNCTION() void Tab_Display();
    UFUNCTION() void Tab_Help();

    UFUNCTION() void Stage_Btn1();
    UFUNCTION() void Stage_Btn2();
    UFUNCTION() void Stage_Btn3();
    UFUNCTION() void Stage_Btn4();

    UFUNCTION() void Play_Btn();
    UFUNCTION() void Pause_Btn();
    UFUNCTION() void Reset_Btn();

    UFUNCTION() void LeakToggle_Btn();
    UFUNCTION() void LeakHighlight_Btn();

    UFUNCTION() void Transp_Changed(float Val);
    UFUNCTION() void Label_Toggled(bool bChecked);
    UFUNCTION() void Heat_Toggled(bool bChecked);

    UPROPERTY()
    TWeakObjectPtr<AMRSandboxRoot> SandboxRoot;

    UPROPERTY()
    UMRHandUIInteractor* HandInteractor = nullptr;

    UPROPERTY()
    UScrollBox* ContentBox = nullptr;

    UPROPERTY()
    UButton* TabButtons[4] = {};

    UPROPERTY()
    UBorder* PanelBG = nullptr;

    /** 所有可点击按钮（Tab + 内容区），供 HandleScreenTap 做命中检测。 */
    UPROPERTY()
    TArray<UButton*> AllButtons;

    int32 ActiveTab = 0;
    bool bLayoutBuilt = false;
    bool bMouseDebugMode = false;
    bool bLabelsVisible = true;
    bool bHeatzonesVisible = true;
};
