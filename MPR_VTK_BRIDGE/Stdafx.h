// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once
#include <windows.h>
#pragma comment(lib, "user32.lib") // added this to avoid ambiguous symbols when calling windows API from Managed c++
using namespace std;
using namespace System;
using namespace System::Runtime::InteropServices;

#include <iostream>
#include <wchar.h>
