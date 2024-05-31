#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

// Winodws.h 에 정의된 min max 매크로는 안전하지 않기 때문에 사용하지 않습니다.
#define NOMINMAX

// Windows 헤더 파일
#include <windows.h>
#include <wrl/client.h>
#include <wrl/event.h>

// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// CRT 라이브러리
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif