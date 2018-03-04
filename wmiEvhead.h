#pragma once
#ifndef wmiEvhead_H
#define wmiEvhead_H
#include <stdio.h>
#include <sstream>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
using namespace std;

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

struct connnectInfo {
	bool connectionDataFound;
	int processid;
	std::string localaddress;
	std::string localport;
	std::string remoteaddress;
	std::string remotePort;
	};

IWbemServices* wmiSetup();
std::string getOutput(IWbemServices *pSvc);
std::string makehash(std::string procentry);
std::string getWMIEvent(int pid, IWbemServices *pSvc);
bool in_array(const std::string &value, const std::vector<string> &array);
connnectInfo getConnectInfo();
void clear();
#endif







