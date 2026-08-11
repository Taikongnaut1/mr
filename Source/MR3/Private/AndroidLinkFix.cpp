// AndroidLinkFix.cpp
// 修复预编译引擎 Android 打包时缺失的符号定义
// 预编译引擎的 .o 文件缺少 GInternalProjectName / GIsGameAgnosticExe / StdMalloc 等定义
// 这些符号在 UnrealGame-arm64.so 里有，但 UBT 没有链接 SO
// 在项目代码里手动定义，让链接器能解析

#include "CoreMinimal.h"

#if PLATFORM_ANDROID

// --- GInternalProjectName ---
// 引擎全局变量，存储项目名（TCHAR[64] 类型，见 CoreGlobals.h）
TCHAR GInternalProjectName[64] = TEXT("MR3");

// --- GIsGameAgnosticExe ---
// 标记是否为通用可执行文件（非游戏特定）
bool GIsGameAgnosticExe = false;

// --- StdMalloc / StdFree / StdRealloc ---
// Mimalloc 的 C 接口别名，被 Chaos（物理）和 GeometryAlgorithms 引用
// 预编译引擎的 MiMalloc.c.o 缺少这些定义，用 FMemory 代替

void* StdMalloc(unsigned long Size, unsigned long Alignment)
{
    (void)Alignment;
    return FMemory::Malloc(Size);
}

void StdFree(void* Ptr)
{
    FMemory::Free(Ptr);
}

void* StdRealloc(void* Ptr, unsigned long Size, unsigned long Alignment)
{
    (void)Alignment;
    return FMemory::Realloc(Ptr, Size);
}

#endif // PLATFORM_ANDROID
