#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <unordered_map>
#include <stdio.h>
#include <sstream>
#include <string>
#include <iostream>
#include <ctime>
#include <iomanip>
#include "wmiEvhead.h"
using namespace std;

#pragma warning(suppress : 4996)
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

std::string delim = ",";


void clear() {
	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

std::string& BstrToStdString(const BSTR bstr, std::string& dst, int cp = CP_UTF8)
{
	if (!bstr)
	{

		dst.clear();
		return dst;
	}

	int res = WideCharToMultiByte(cp, 0, bstr, -1, NULL, 0, NULL, NULL);
	if (res > 0)
	{
		dst.resize(res);
		WideCharToMultiByte(cp, 0, bstr, -1, &dst[0], res, NULL, NULL);
	}
	else
	{    
		dst.clear();
	}
	return dst;
}

IWbemServices* wmiSetup()
{
	IWbemServices *pSvc = NULL;
	HRESULT hres;
	std::stringstream failmsg;

	// Initialize COM
	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		failmsg << "Failed to initialize COM library. Error code = 0x"
			<< hex << hres << endl;
		cout << failmsg.str();
	}

	// Set general COM security levels --------------------------
	hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
	);

	if (FAILED(hres))
	{
		failmsg << "Failed to initialize security. Error code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		cout << failmsg.str();
	}

	// Obtain the initial locator to WMI 
	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		failmsg << "Failed to create IWbemLocator object."
			<< " Err code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		cout << failmsg.str();
	}


	// Connect to WMI through the IWbemLocator::ConnectServer method
	//IWbemServices *pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (for example, Kerberos)
		0,                       // Context object 
		&pSvc                    // pointer to IWbemServices proxy
	);
	//cout << "PSVC address: " << pSvc << endl;
	if (FAILED(hres))
	{
		failmsg << "Could not connect. Error code = 0x"
			<< hex << hres << endl;
		pLoc->Release();
		CoUninitialize();
		cout << failmsg.str();

	}

	// Set security levels on the proxy -------------------------
	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
	);

	if (FAILED(hres))
	{
		failmsg << "Could not set proxy blanket. Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		cout << failmsg.str();

	}
	if (FAILED(hres))
	{
		failmsg << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		//pLoc->Release();
		CoUninitialize();
		cout << failmsg.str();

	}

	return pSvc;

}


std::string getWMIEvent(int pid, IWbemServices *pSvc)
{
	HRESULT hres;
	std::stringstream failmsg;

	//WMI query for the win32_process class
	char wqlQuery[512];
	sprintf_s(wqlQuery, "SELECT * "
		"FROM Win32_Process");
	//printf(wqlQuery);
	//printf("\n");
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t(wqlQuery),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);



	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	std::stringstream WMIProcInfo;
	std::string sWMIProcInfo;
	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}
		//Get the value of the Name property
		//Convert Pid to a BSTR Type
		wchar_t PidStr[10];

		_itow(pid, PidStr, 10);
		BSTR bstr_Pid = SysAllocString(PidStr);
		VARIANT vtProp;

		hr = pclsObj->Get(L"Handle", 0, &vtProp, 0, 0);

		int bstr_comp = wcscmp(vtProp.bstrVal, bstr_Pid);

		VariantClear(&vtProp);

		if (bstr_comp == 0) {

			hr = pclsObj->Get(L"CreationDate", 0, &vtProp, 0, 0);
			std::string creationdate;

			if (vtProp.bstrVal == NULL) {
				WMIProcInfo << "None";
			}
			else {
				BstrToStdString(vtProp.bstrVal, creationdate);
				//procnamestr.resize(24);
				WMIProcInfo << creationdate << delim;
			}
			VariantClear(&vtProp);

			hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			std::string procnamestr;

			if (vtProp.bstrVal == NULL) {
				WMIProcInfo << "None";
			}
			else {
				BstrToStdString(vtProp.bstrVal, procnamestr);
				WMIProcInfo << procnamestr << delim;
			}
			VariantClear(&vtProp);

			hr = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);
			std::string cmdlinestr;

			if (vtProp.bstrVal == NULL) {
				WMIProcInfo << "None" ;
			}
			else {
				BstrToStdString(vtProp.bstrVal, cmdlinestr);
				WMIProcInfo << cmdlinestr;
			}
			VariantClear(&vtProp);
			pclsObj->Release();
		}
		else { pclsObj->Release(); }
		SysFreeString(bstr_Pid);
		sWMIProcInfo = WMIProcInfo.str();
		WMIProcInfo.clear();


	}

	pEnumerator->Release();
	failmsg.clear();
	return sWMIProcInfo;
}

connnectInfo getConnectInfo()
{

    //Declare and initialize variables
    PMIB_TCPTABLE2 pTcpTable;
    ULONG ulSize = 0;
    DWORD dwRetVal = 0;

    struct in_addr IpAddr;
    int i;

    pTcpTable = (MIB_TCPTABLE2 *) MALLOC(sizeof (MIB_TCPTABLE2));
    if (pTcpTable == NULL) {
        cout << "Error allocating memory\n" << endl;

    }

    ulSize = sizeof (MIB_TCPTABLE);

    if ((dwRetVal = GetTcpTable2(pTcpTable, &ulSize, TRUE)) ==
        ERROR_INSUFFICIENT_BUFFER) {
        FREE(pTcpTable);
        pTcpTable = (MIB_TCPTABLE2 *) MALLOC(ulSize);
        if (pTcpTable == NULL) {
			cout << "Error allocating memory\n" << endl;


        }
    }
	// Make a second call to GetTcpTable2 to get
	// the actual data we require
	unsigned short rportNo ;
	unsigned short lportNo;
	unsigned short smbPort;
	unsigned short nbPort;
	std::stringstream lineConnection;

	smbPort = 445;
	nbPort = 135;
	int PidNo;
	connnectInfo newConnection;
	newConnection.connectionDataFound = false;

    if ((dwRetVal = GetTcpTable2(pTcpTable, &ulSize, TRUE)) == NO_ERROR) {
		
        for (i = 0; i < (int) pTcpTable->dwNumEntries; i++) {
			rportNo = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
			lportNo = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
			PidNo = pTcpTable->table[i].dwOwningPid;
			IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwLocalAddr;
			std::string szLocalAddr = inet_ntoa(IpAddr);

			if (szLocalAddr != "0.0.0.0" && (lportNo == smbPort || rportNo == smbPort ||
				 lportNo == nbPort  || rportNo == nbPort )) 
			{
				newConnection.connectionDataFound = true;

				std::string LocalPort = std::to_string(lportNo);
				IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwRemoteAddr;
				std::string szRemoteAddr = inet_ntoa(IpAddr);
				std::string RemotePort = std::to_string(rportNo);
				newConnection.processid = PidNo;
				newConnection.localaddress = szLocalAddr;
				newConnection.localport = LocalPort;
				newConnection.remoteaddress = szRemoteAddr;
				newConnection.remotePort = RemotePort;

			}

			else {	continue;	}
                   
        }

		dwRetVal = NULL;

	} else {

		FREE(pTcpTable);	

    }
    
    if (pTcpTable != NULL) {
        FREE(pTcpTable);
        pTcpTable = NULL;
    }

	return newConnection;
}

std::string getOutput(IWbemServices *pSvc)
{
	std::string output;
	connnectInfo getConnection;
	getConnection = getConnectInfo();

	if (getConnection.connectionDataFound != true)
	{

		return std::string();

	}

	//DEBUG
	else
	{

	std::string wmiOutput;
	std::stringstream lineConnection;
	wmiOutput = getWMIEvent(getConnection.processid, pSvc);

	lineConnection
		<< std::to_string(getConnection.processid)	<< ","
		<< getConnection.localaddress  << ":" 
		<< getConnection.localport << ","
		<< getConnection.remoteaddress << ":" 
		<< getConnection.remotePort << ","
		<< wmiOutput
		<< endl;

	output = lineConnection.str();
	cout << "Output: \n" << output << endl;
	return output;
	}
}

std::string makehash(std::string procentry)
{
	std::stringstream sshash;
	hash<string> hasher;
	size_t hash = hasher(procentry);
	sshash << hash << endl;
	return (sshash.str()).substr(0,6);

}

bool in_array(const std::string &value, const std::vector<string> &array)
{
	return std::find(array.begin(), array.end(), value) != array.end();
}