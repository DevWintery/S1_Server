#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

#ifdef _DEBUG
#pragma comment(lib, "lib\\S1\\Debug\\Core.lib")
#pragma comment(lib, "lib\\Protobuf\\Debug\\libprotobufd.lib")
#pragma comment(lib, "lib\\cpprest\\cpprest_2_10.lib")
#else
#pragma comment(lib, "lib\\S1\\Release\\Core.lib")
#pragma comment(lib, "lib\\Protobuf\\Release\\libprotobuf.lib")
#pragma comment(lib, "lib\\cpprest\\cpprest_2_10.lib")
#endif

#include "S1Core.h"					