#pragma once

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

// Windows ��� ����
#include <windows.h>
#include <wrl/client.h>
#include <wrl/event.h>

// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// CRT ���̺귯��
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
