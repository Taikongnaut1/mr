#include "UXVisioXRWorldSubsystem.h"

#include "HeadMountedDisplayFunctionLibrary.h"
#include "Engine/World.h"
#include "OpenXRCore.h"
#include "IXRTrackingSystem.h"


void UXVisioXRWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (XvisioOpenXR::FXVisioOpenXRModule::IsAvailable())
	{
		xVisioOpenXRModule = XvisioOpenXR::FXVisioOpenXRModule::Get();
		keyEventPlugin = &xVisioOpenXRModule->GetKeyEventPlugin();
		if (keyEventPlugin)
		{
			keyEventRunable = XvisioOpenXR::AKeyEventRunable::GetInstance(keyEventPlugin);
		}

		XvisioFunctionPlugin = &xVisioOpenXRModule->GetXvisioFunctionPlugin();
	}

	// 获取 XR 追踪系统引用，用于 SLAM -> UE 坐标系转换
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == FName(TEXT("OpenXR"))))
	{
		XRTrackingSystem = GEngine->XRSystem.Get();
	}
}

void UXVisioXRWorldSubsystem::Deinitialize()
{
	StopEvent();
	Super::Deinitialize();
}

bool UXVisioXRWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}


void UXVisioXRWorldSubsystem::Tick(float DeltaTime)
{
	if (!keyEventPlugin || !keyEventRunable)
		return;
	
	keyEventRunable->TickEvent->Trigger();
	
}

bool UXVisioXRWorldSubsystem::IsTickable() const
{
	return isTickable && GetWorld() && GetWorld()->IsGameWorld();
}

TStatId UXVisioXRWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UXVisioXRWorldSubsystem, STATGROUP_Tickables);
}


bool UXVisioXRWorldSubsystem::StartEvent()
{
	if (!keyEventPlugin)
	{
		UE_LOG(LogTemp, Log, TEXT("keyEventPlugin 未获取到事件插件"));
		return false;
	}
		
	int ret = -1;
	bool success = keyEventPlugin->StartEvent(&ret);
	UE_LOG(LogTemp, Log, TEXT("事件启动 --- %d **** %d"),success,ret);
	if (!success && ret == -1)
	{
		UE_LOG(LogTemp, Log, TEXT("启动事件获取失败 未获取到事件插件"));
		return false;
	}
	//启动线程
	keyEventRunable = XvisioOpenXR::AKeyEventRunable::GetInstance(keyEventPlugin);
	if (!keyEventRunable)
	{
		UE_LOG(LogTemp, Log, TEXT("线程 获取 失败"));
		return false;
	}
	isTickable = true;
	PressedKeys.Empty();
	
	keyEventRunable->KeyEventDownDelegate.AddUObject(
		this,
		&UXVisioXRWorldSubsystem::HandleNativeKeyDown);
	
	keyEventRunable->KeyEventDownTickDelegate.AddUObject(
		this,
		&UXVisioXRWorldSubsystem::HandleNativeOnKeyDownTick);
	
	keyEventRunable->KeyEventUpDelegate.AddUObject(
		this,
		&UXVisioXRWorldSubsystem::HandleNativeKeyUp);

	// 绑定方向键委托
	Dir1DownHandle = keyEventRunable->KeyEventDir1DownDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 1;
		Data.KeyCode = 1;
		Data.EventState = 254;
		LastKeyEventData = Data;
		PressedKeys.Add(1);
		OnXvisioKeyDown.Broadcast(1);
	});
	Dir1UpHandle = keyEventRunable->KeyEventDir1UpDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 1;
		Data.KeyCode = 1;
		Data.EventState = 255;
		LastKeyEventData = Data;
		PressedKeys.Remove(1);
		OnXvisioKeyUp.Broadcast(1);
	});

	Dir2DownHandle = keyEventRunable->KeyEventDir2DownDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 2;
		Data.KeyCode = 2;
		Data.EventState = 254;
		LastKeyEventData = Data;
		PressedKeys.Add(2);
		OnXvisioKeyDown.Broadcast(2);
	});
	Dir2UpHandle = keyEventRunable->KeyEventDir2UpDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 2;
		Data.KeyCode = 2;
		Data.EventState = 255;
		LastKeyEventData = Data;
		PressedKeys.Remove(2);
		OnXvisioKeyUp.Broadcast(2);
	});

	Dir3DownHandle = keyEventRunable->KeyEventDir3DownDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 4;
		Data.KeyCode = 4;
		Data.EventState = 254;
		LastKeyEventData = Data;
		PressedKeys.Add(4);
		OnXvisioKeyDown.Broadcast(4);
	});
	Dir3UpHandle = keyEventRunable->KeyEventDir3UpDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 4;
		Data.KeyCode = 4;
		Data.EventState = 255;
		LastKeyEventData = Data;
		PressedKeys.Remove(4);
		OnXvisioKeyUp.Broadcast(4);
	});

	Dir4DownHandle = keyEventRunable->KeyEventDir4DownDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 5;
		Data.KeyCode = 5;
		Data.EventState = 254;
		LastKeyEventData = Data;
		PressedKeys.Add(5);
		OnXvisioKeyDown.Broadcast(5);
	});
	Dir4UpHandle = keyEventRunable->KeyEventDir4UpDelegate.AddLambda([this]()
	{
		FXvisioKeyEventData Data;
		Data.EventType = 5;
		Data.KeyCode = 5;
		Data.EventState = 255;
		LastKeyEventData = Data;
		PressedKeys.Remove(5);
		OnXvisioKeyUp.Broadcast(5);
	});

	return true;
}

bool UXVisioXRWorldSubsystem::StopEvent()
{
	if (!keyEventPlugin)
		return false;
	int ret = -1;
	bool success = keyEventPlugin->StopEvent(&ret);
	if (!success && ret == -1)
	{
		return false;
	}
	// 解绑方向键委托
	if (keyEventRunable)
	{
		keyEventRunable->KeyEventDir1DownDelegate.Remove(Dir1DownHandle);
		keyEventRunable->KeyEventDir1UpDelegate.Remove(Dir1UpHandle);
		keyEventRunable->KeyEventDir2DownDelegate.Remove(Dir2DownHandle);
		keyEventRunable->KeyEventDir2UpDelegate.Remove(Dir2UpHandle);
		keyEventRunable->KeyEventDir3DownDelegate.Remove(Dir3DownHandle);
		keyEventRunable->KeyEventDir3UpDelegate.Remove(Dir3UpHandle);
		keyEventRunable->KeyEventDir4DownDelegate.Remove(Dir4DownHandle);
		keyEventRunable->KeyEventDir4UpDelegate.Remove(Dir4UpHandle);
	}
	//关闭线程
	if (keyEventRunable)
	{
		keyEventRunable->DestroyInstance();
	}
	isTickable = false;
	PressedKeys.Empty();
	return true;
}

bool UXVisioXRWorldSubsystem::testEvent()
{

	return false;

}

bool UXVisioXRWorldSubsystem::StopApriltag(int ret)
{
	
	XvisioFunctionPlugin->StopApriltag(ret);
	return false;
}

bool UXVisioXRWorldSubsystem::GetApriltag(const FString& InTagFamily, float InTagSize, int32 InDetectType, int32& OutTagCount)
{
	if (!XvisioFunctionPlugin)
	{
		UE_LOG(LogTemp, Error, TEXT("GetApriltag: XvisioFunctionPlugin is null"));
		OutTagCount = 0;
		return false;
	}

	// 将 FString 转换为 null-terminated unsigned char 数组（最多5个字符+'\0'）
	FTCHARToUTF8 TagFamilyUtf8(*InTagFamily);
	int32 Utf8Len = FMath::Min(TagFamilyUtf8.Length(), 5);
	FMemory::Memzero(TagFamily, sizeof(TagFamily));
	FMemory::Memcpy(TagFamily, TagFamilyUtf8.Get(), Utf8Len);

	// 更新成员变量（供后续查询）
	this->TagSize = InTagSize;
	this->DetectType = InDetectType;

	bool bSuccess = XvisioFunctionPlugin->getApriltag(
		TagFamily,
		static_cast<double>(InTagSize),
		tagsArray,
		&TagCout,
		static_cast<uint32_t>(MaxArraySize),
		static_cast<uint32_t>(InDetectType)
	);

	OutTagCount = static_cast<int32>(TagCout);
	return bSuccess;
}

bool UXVisioXRWorldSubsystem::GetApriltagData(int32 Index, int32& OutTagID, FVector& OutPosition, FRotator& OutOrientation, FQuat& OutQuaternion, float& OutConfidence)
{
	if (Index < 0 || Index >= static_cast<int32>(TagCout) || Index >= MaxArraySize)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetApriltagData: Index %d out of range (TagCout=%d, MaxArraySize=%d)"), Index, TagCout, MaxArraySize);
		return false;
	}

	const XrTagDataXV& TagData = tagsArray[Index].detect[0];
	OutTagID = static_cast<int32>(TagData.tagID);
	OutConfidence = TagData.confidence;

	// ---- 数据是世界坐标(s_pose * d.transform), 直接轴映射 ----
	float WorldToMetersScale = 1.0f;
	if (XRTrackingSystem)
	{
		WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale();
	}

	// 位置: Unity→UE 轴映射 (z,x,y)
	OutPosition = FVector(
		TagData.position.z * WorldToMetersScale,
		TagData.position.x * WorldToMetersScale,
		TagData.position.y * WorldToMetersScale);

	// 旋转: 轴映射(z,x,y/w) — orientation→FRotator, quaternion→FQuat
	OutOrientation = FRotator(
		FMath::RadiansToDegrees(TagData.orientation.z),
		FMath::RadiansToDegrees(TagData.orientation.x),
		FMath::RadiansToDegrees(TagData.orientation.y));
	
	FQuat QuatRaw = FQuat(TagData.quaternion.z, TagData.quaternion.x, TagData.quaternion.y, TagData.quaternion.w);
	
	OutQuaternion = QuatRaw;

	return true;
}

void UXVisioXRWorldSubsystem::HandleNativeKeyDown()
{
	FXvisioKeyEventData Data;
	Data.EventType = 3;
	Data.KeyCode = 3;
	Data.EventState = 254;
	LastKeyEventData = Data;
	PressedKeys.Add(3);
	OnXvisioKeyDown.Broadcast(3);
	OnKeyDown.Broadcast(); 
}

void UXVisioXRWorldSubsystem::HandleNativeKeyUp()
{
	FXvisioKeyEventData Data;
	Data.EventType = 3;
	Data.KeyCode = 3;
	Data.EventState = 255;
	LastKeyEventData = Data;
	PressedKeys.Remove(3);
	OnXvisioKeyUp.Broadcast(3);
	OnKeyup.Broadcast();
}

void UXVisioXRWorldSubsystem::HandleNativeOnKeyDownTick()
{
	OnXvisioKeyTick.Broadcast(3);
	OnKeyDownTick.Broadcast();
}

	