// Copyright (c) Yuquan Sun. All rights reserved.
// 纯 C++ UMG 面板：Screen Space，永远浮在场景之上，鼠标直接点击。

#include "MR3PanelWidget.h"
#include "MRSandboxRoot.h"
#include "Blueprint/WidgetTree.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"

#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"

// ═══════════════════ 样式常量 ═══════════════════
namespace MR3UI
{
    static const FLinearColor PanelBg    {0.08f, 0.08f, 0.10f, 0.90f};
    static const FLinearColor BtnNormal  {0.18f, 0.18f, 0.20f, 1.0f};
    static const FLinearColor BtnHover   {0.30f, 0.30f, 0.38f, 1.0f};
    static const FLinearColor BtnPress   {0.12f, 0.28f, 0.55f, 1.0f};
    static const FLinearColor BtnActive  {0.15f, 0.35f, 0.70f, 1.0f};
    static const FLinearColor TextWhite  {0.95f, 0.95f, 0.95f, 1.0f};
    static const FLinearColor TextDim    {0.60f, 0.60f, 0.60f, 1.0f};
    static const FLinearColor DividerClr {0.25f, 0.25f, 0.28f, 1.0f};

    static FSlateFontInfo Font(int32 Sz)
    {
        return FCoreStyle::GetDefaultFontStyle("Regular", Sz);
    }

    static void StyleButton(UButton* B, bool bActive = false)
    {
        FButtonStyle S;
        auto Mk = [](FLinearColor C) {
            FSlateBrush Br;
            Br.TintColor = C;
            Br.DrawAs = ESlateBrushDrawType::RoundedBox;
            Br.OutlineSettings.CornerRadii = FVector4(4,4,4,4);
            return Br;
        };
        S.SetNormal(Mk(bActive ? BtnActive : BtnNormal));
        S.SetHovered(Mk(BtnHover));
        S.SetPressed(Mk(BtnPress));
        B->SetStyle(S);
    }
}

// ═══════════════════ NativeConstruct ═══════════════════
void UMR3PanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, TEXT("MR3PanelWidget: NativeConstruct START"));
    }

    BuildLayout();

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, TEXT("MR3PanelWidget: NativeConstruct DONE"));
    }
}

// ═══════════════════ BuildLayout ═══════════════════
void UMR3PanelWidget::BuildLayout()
{
    if (bLayoutBuilt || !WidgetTree) return;
    bLayoutBuilt = true;

    // 根 Canvas：铺满整个 widget
    UCanvasPanel* RootCP = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = RootCP;

    // 背景面板：固定右上角 520×620，距边缘 12px
    UBorder* BG = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.TintColor = MR3UI::PanelBg;
        BG->SetBrush(Brush);
    }
    BG->SetPadding(FMargin(12.f, 10.f));
    BG->SetVisibility(ESlateVisibility::Visible);
    RootCP->AddChild(BG);
    if (auto* CS = Cast<UCanvasPanelSlot>(BG->Slot))
    {
        CS->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
        CS->SetPosition(FVector2D(-532.f, 12.f));
        CS->SetSize(FVector2D(520.f, 620.f));
    }

    // 内容垂直布局
    UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    VBox->SetVisibility(ESlateVisibility::Visible);
    BG->SetContent(VBox);

    // ── 标题 ──
    {
        UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Title->SetText(FText::FromString(TEXT("熔融金属泄漏 MR 训练")));
        Title->SetColorAndOpacity(FSlateColor(MR3UI::TextWhite));
        Title->SetFont(MR3UI::Font(18));
        VBox->AddChild(Title);
    }

    // ── Tab 栏 ──
    {
        UHorizontalBox* HB = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        HB->SetVisibility(ESlateVisibility::Visible);
        VBox->AddChild(HB);

        const TCHAR* Names[] = { TEXT("事故阶段"), TEXT("播放控制"), TEXT("显示选项"), TEXT("手势帮助") };
        for (int32 i = 0; i < 4; ++i)
        {
            UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
            MR3UI::StyleButton(Btn, i == 0);
            Btn->SetVisibility(ESlateVisibility::Visible);

            UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            Txt->SetText(FText::FromString(Names[i]));
            Txt->SetColorAndOpacity(FSlateColor(MR3UI::TextWhite));
            Txt->SetFont(MR3UI::Font(13));
            Btn->AddChild(Txt);

            USizeBox* BtnBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            BtnBox->SetWidthOverride(124.f);
            BtnBox->SetHeightOverride(40.f);
            BtnBox->SetVisibility(ESlateVisibility::Visible);
            BtnBox->AddChild(Btn);
            if (auto* S = Cast<USizeBoxSlot>(Btn->Slot))
            {
                S->SetHorizontalAlignment(HAlign_Center);
                S->SetVerticalAlignment(VAlign_Center);
            }

            HB->AddChild(BtnBox);
            TabButtons[i] = Btn;
        }

        TabButtons[0]->OnClicked.AddDynamic(this, &UMR3PanelWidget::Tab_Stage);
        TabButtons[1]->OnClicked.AddDynamic(this, &UMR3PanelWidget::Tab_Playback);
        TabButtons[2]->OnClicked.AddDynamic(this, &UMR3PanelWidget::Tab_Display);
        TabButtons[3]->OnClicked.AddDynamic(this, &UMR3PanelWidget::Tab_Help);
    }

    // ── 分割线 ──
    {
        UBorder* Div = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.TintColor = MR3UI::DividerClr;
            Div->SetBrush(Brush);
        }
        Div->SetVisibility(ESlateVisibility::Visible);
        VBox->AddChild(Div);
        if (auto* VS = Cast<UVerticalBoxSlot>(Div->Slot))
        {
            VS->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    // ── 内容区 ScrollBox ──
    ContentBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    ContentBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
    ContentBox->SetVisibility(ESlateVisibility::Visible);
    ContentBox->SetClipping(EWidgetClipping::ClipToBounds);
    VBox->AddChild(ContentBox);
    if (auto* VS = Cast<UVerticalBoxSlot>(ContentBox->Slot))
    {
        VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ActiveTab = 0;
    RefreshContent();
}

// ═══════════════════ RefreshContent ═══════════════════
void UMR3PanelWidget::RefreshContent()
{
    if (!ContentBox) return;
    ContentBox->ClearChildren();

    UVerticalBox* GV = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    GV->SetVisibility(ESlateVisibility::Visible);
    ContentBox->AddChild(GV);

    // 更新 Tab 高亮
    for (int32 i = 0; i < 4; ++i)
    {
        if (TabButtons[i]) MR3UI::StyleButton(TabButtons[i], i == ActiveTab);
    }

    auto AddGroupTitle = [&](const FString& Title)
    {
        UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        T->SetText(FText::FromString(FString::Printf(TEXT("%s"), *Title)));
        T->SetColorAndOpacity(FSlateColor(MR3UI::TextDim));
        T->SetFont(MR3UI::Font(12));
        if (auto* VS = Cast<UVerticalBoxSlot>(GV->AddChild(T)))
        {
            VS->SetPadding(FMargin(0.f, 12.f, 0.f, 4.f));
        }
    };

    auto AddButton = [&](const FString& Label, UMR3PanelWidget* Obj, void(UMR3PanelWidget::*Func)()) -> void
    {
        UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        MR3UI::StyleButton(Btn);
        Btn->SetVisibility(ESlateVisibility::Visible);

        UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Txt->SetText(FText::FromString(Label));
        Txt->SetColorAndOpacity(FSlateColor(MR3UI::TextWhite));
        Txt->SetFont(MR3UI::Font(14));
        Btn->AddChild(Txt);

        USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Box->SetHeightOverride(42.f);
        Box->SetVisibility(ESlateVisibility::Visible);
        Box->AddChild(Btn);
        if (auto* S = Cast<USizeBoxSlot>(Btn->Slot))
        {
            S->SetHorizontalAlignment(HAlign_Left);
            S->SetVerticalAlignment(VAlign_Center);
        }

        GV->AddChild(Box);
        if (auto* VS = Cast<UVerticalBoxSlot>(Box->Slot))
        {
            VS->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
        }

        Btn->OnClicked.AddDynamic(Obj, Func);
    };

    switch (ActiveTab)
    {
    case 0: // 事故阶段
        AddGroupTitle(TEXT("主要阶段"));
        AddButton(TEXT("① 泄漏避险"), this, &UMR3PanelWidget::Stage_Btn1);
        AddButton(TEXT("② 侦察指挥"), this, &UMR3PanelWidget::Stage_Btn2);
        AddButton(TEXT("③ 封控处置"), this, &UMR3PanelWidget::Stage_Btn3);
        AddButton(TEXT("④ 医疗救治"), this, &UMR3PanelWidget::Stage_Btn4);

        AddGroupTitle(TEXT("泄漏源"));
        AddButton(TEXT("显示/隐藏泄漏源"), this, &UMR3PanelWidget::Stage_Btn1);
        AddButton(TEXT("泄漏源高亮"), this, &UMR3PanelWidget::Stage_Btn2);
        break;

    case 1: // 播放控制
        AddGroupTitle(TEXT("动画播放"));
        AddButton(TEXT("▶ 播放"),  this, &UMR3PanelWidget::Play_Btn);
        AddButton(TEXT("⏸ 暂停"), this, &UMR3PanelWidget::Pause_Btn);
        AddButton(TEXT("↺ 重置"), this, &UMR3PanelWidget::Reset_Btn);
        break;

    case 2: // 显示选项
        AddGroupTitle(TEXT("外观"));
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            Row->SetVisibility(ESlateVisibility::Visible);
            GV->AddChild(Row);

            UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            L->SetText(FText::FromString(TEXT("透明度")));
            L->SetColorAndOpacity(FSlateColor(MR3UI::TextWhite));
            L->SetFont(MR3UI::Font(13));
            Row->AddChild(L);

            USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
            Slider->SetValue(0.9f);
            Slider->SetMinValue(0.2f);
            Slider->SetMaxValue(1.0f);
            Slider->SetVisibility(ESlateVisibility::Visible);
            Row->AddChild(Slider);
            if (auto* HS = Cast<UHorizontalBoxSlot>(Slider->Slot))
                HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slider->OnValueChanged.AddDynamic(this, &UMR3PanelWidget::Transp_Changed);
        }

        AddGroupTitle(TEXT("图层开关"));
        {
            auto AddCheck = [&](const FString& Label, void(UMR3PanelWidget::*Func)(bool))
            {
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
                Row->SetVisibility(ESlateVisibility::Visible);
                GV->AddChild(Row);

                UCheckBox* C = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
                C->SetCheckedState(ECheckBoxState::Checked);
                C->SetVisibility(ESlateVisibility::Visible);
                Row->AddChild(C);
                C->OnCheckStateChanged.AddDynamic(this, Func);

                UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
                T->SetText(FText::FromString(Label));
                T->SetColorAndOpacity(FSlateColor(MR3UI::TextWhite));
                T->SetFont(MR3UI::Font(13));
                Row->AddChild(T);
            };
            AddCheck(TEXT("显示标签"), &UMR3PanelWidget::Label_Toggled);
            AddCheck(TEXT("显示热区"), &UMR3PanelWidget::Heat_Toggled);
        }
        break;

    case 3: // 手势帮助
        AddGroupTitle(TEXT("操作说明"));
        {
            UTextBlock* Info = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
            Info->SetText(FText::FromString(TEXT(
                "• 鼠标左键：点击 UI 按钮\n"
                "• 拇指+食指捏合：点击 UI（HMD）\n"
                "• 手掌抓取：拖拽/缩放沙盘\n"
                "• 面板始终位于屏幕右上角"
            )));
            Info->SetColorAndOpacity(FSlateColor(MR3UI::TextDim));
            Info->SetFont(MR3UI::Font(13));
            Info->SetAutoWrapText(true);
            GV->AddChild(Info);
        }
        break;
    }
}

// ═══════════════════ 引用设置 ═══════════════════
void UMR3PanelWidget::SetupSandboxRefs(AMRSandboxRoot* InSandboxRoot, UMRHandUIInteractor* InInteractor)
{
    SandboxRoot = InSandboxRoot;
    HandInteractor = InInteractor;
}

void UMR3PanelWidget::SetMouseDebugMode(bool bEnabled)
{
    bMouseDebugMode = bEnabled;
}

// ═══════════════════ 事件处理 ═══════════════════
void UMR3PanelWidget::SwitchTab(int32 Tab)
{
    ActiveTab = FMath::Clamp(Tab, 0, 3);
    RefreshContent();
}

void UMR3PanelWidget::Tab_Stage()    { SwitchTab(0); }
void UMR3PanelWidget::Tab_Playback() { SwitchTab(1); }
void UMR3PanelWidget::Tab_Display()  { SwitchTab(2); }
void UMR3PanelWidget::Tab_Help()     { SwitchTab(3); }

void UMR3PanelWidget::Stage_Btn1()  { if (auto* S = SandboxRoot.Get()) S->SetStage(1); }
void UMR3PanelWidget::Stage_Btn2()  { if (auto* S = SandboxRoot.Get()) S->SetStage(2); }
void UMR3PanelWidget::Stage_Btn3()  { if (auto* S = SandboxRoot.Get()) S->SetStage(3); }
void UMR3PanelWidget::Stage_Btn4()  { if (auto* S = SandboxRoot.Get()) S->SetStage(4); }

void UMR3PanelWidget::Play_Btn()    { if (auto* S = SandboxRoot.Get()) S->SetAnimationPaused(false); }
void UMR3PanelWidget::Pause_Btn()   { if (auto* S = SandboxRoot.Get()) S->SetAnimationPaused(true); }
void UMR3PanelWidget::Reset_Btn()   { if (auto* S = SandboxRoot.Get()) S->ResetSandbox(); }

void UMR3PanelWidget::Transp_Changed(float Val)
{
    if (!WidgetTree || !WidgetTree->RootWidget) return;
    if (UCanvasPanel* CP = Cast<UCanvasPanel>(WidgetTree->RootWidget))
    {
        if (UBorder* BG = Cast<UBorder>(CP->GetChildAt(0)))
        {
            FLinearColor C = MR3UI::PanelBg;
            C.A = FMath::Clamp(Val, 0.f, 1.f);
            BG->SetBrushColor(C);
        }
    }
}

void UMR3PanelWidget::Label_Toggled(bool bChecked)
{
    bLabelsVisible = bChecked;
}

void UMR3PanelWidget::Heat_Toggled(bool bChecked)
{
    bHeatzonesVisible = bChecked;
}
