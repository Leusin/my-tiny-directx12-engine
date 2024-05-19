#pragma once

// #pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
// #pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
// #pragma warning(disable:4239) // A non-const reference may only be bound to an lvalue; assignment operator takes a reference to non-const
// #pragma warning(disable:4324) // structure was padded due to __declspec(align())

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>

//  Windows 플랫폼에서, min 및 max 매크로는
// <algorithm> 의 std::min 및 std::max와 충돌합니다.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

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