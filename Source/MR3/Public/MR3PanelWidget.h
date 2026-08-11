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

protected:
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

    int32 ActiveTab = 0;
    bool bLayoutBuilt = false;
    bool bMouseDebugMode = false;
    bool bLabelsVisible = true;
    bool bHeatzonesVisible = true;
};
