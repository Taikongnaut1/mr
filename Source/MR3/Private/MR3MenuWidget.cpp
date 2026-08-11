// Copyright (c) Yuquan Sun. All rights reserved.

#include "MR3MenuWidget.h"
#include "MRSandboxRoot.h"

UMR3MenuWidget::UMR3MenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMR3MenuWidget::RequestRefresh()
{
    OnRefreshRequested();
}

void UMR3MenuWidget::HandleTabClicked(int32 NewTabIndex)
{
    if (!Tabs.IsValidIndex(NewTabIndex) || NewTabIndex == ActiveTabIndex)
    {
        return;
    }
    ActiveTabIndex = NewTabIndex;
    OnTabSwitched(NewTabIndex);
    RequestRefresh();
}

void UMR3MenuWidget::HandleGroupToggled(int32 TabIndex, int32 GroupIndex)
{
    if (!Tabs.IsValidIndex(TabIndex)) return;
    if (!Tabs[TabIndex].Groups.IsValidIndex(GroupIndex)) return;

    FMR3MenuGroup& Group = Tabs[TabIndex].Groups[GroupIndex];
    Group.bExpanded = !Group.bExpanded;

    OnGroupToggled(TabIndex, GroupIndex, Group.bExpanded);
    RequestRefresh();
}

void UMR3MenuWidget::HandleItemClicked(FName ItemId)
{
    OnMenuItemClicked(ItemId);
}

void UMR3MenuWidget::HandleItemToggle(FName ItemId, bool bNewState)
{
    // 在所有 tabs 里找 ItemId，同步状态后触发回调。
    for (FMR3MenuTab& Tab : Tabs)
    {
        for (FMR3MenuGroup& Group : Tab.Groups)
        {
            for (FMR3MenuItem& Item : Group.Items)
            {
                if (Item.ItemId == ItemId)
                {
                    Item.bToggleOn = bNewState;
                }
            }
        }
    }
    OnMenuItemClicked(ItemId);
}

void UMR3MenuWidget::SetSandboxRoot(AMRSandboxRoot* InRoot)
{
    SandboxRoot = InRoot;
}
