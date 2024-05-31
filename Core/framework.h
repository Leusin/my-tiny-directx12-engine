#pragma once

// #pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
// #pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
// #pragma warning(disable:4239) // A non-const reference may only be bound to an lvalue; assignment operator takes a reference to non-const
// #pragma warning(disable:4324) // structure was padded due to __declspec(align())


#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

//  Windows 플랫폼에서, min 및 max 매크로는
// <algorithm> 의 std::min 및 std::max와 충돌합니다.
#define NOMINMAX

// Windows 헤더 파일
#include <windows.h>

#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// STL
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>

// DirectX 12 헤더

// 다음 컴파일 경고 무시하기 위해 사용합니다:
#pragma warning(disable : 6001)  // C6001 초기화 되지 않음 메모리 'LocalPlacement' 을(를) 사용하고 있습니다.
#pragma warning(disable : 26827) // C26827 열거형 상수를 초기화하는 것을 잊었거나 다른 유형을 사용하려고 하셨나요?

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>

// CRT 라이브러리
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> 

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define DBG_NEW new
#endif