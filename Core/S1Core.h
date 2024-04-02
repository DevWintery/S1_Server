#pragma once

/*-----------------
* std
------------------- */
#include<iostream>
#include<string>
#include<queue>
#include<vector>
#include<list>
#include<array>
#include<stack>
#include<set>
#include<map>
#include<unordered_map>
#include<unordered_set>
#include<memory>
#include<functional>
#include<thread>
#include<mutex>

using namespace std;

/*-----------------
* WinSock
------------------- */
#include<WinSock2.h>
#include<MSWSock.h>
#include<WS2tcpip.h>
#include<windows.h>
#pragma comment(lib, "ws2_32.lib")

/*-----------------
* Network
------------------- */
#include "Types.h"
#include "UtilMacro.h"
#include "S1TLS.h"

#include "S1Global.h"

#include "Lock.h"
#include "Session.h"
#include "SendBuffer.h"
#include "LockQueue.h"