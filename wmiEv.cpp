#include "C:\dev\netstat\EventSink.h"

int wmiEvent(int Pid)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x"
			<< hex << hres << endl;
		return 1;                  // Program has failed.
	}

	// Step 2: --------------------------------------------------
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
		cout << "Failed to initialize security. Error code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                    // Program has failed.
	}

	// Obtain the initial locator to WMI -------------------------
	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		cout << "Failed to create IWbemLocator object."
			<< " Err code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Connect to WMI through the IWbemLocator::ConnectServer method
	IWbemServices *pSvc = NULL;

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

	if (FAILED(hres))
	{
		cout << "Could not connect. Error code = 0x"
			<< hex << hres << endl;
		pLoc->Release();
		CoUninitialize();
		return 1;                // Program has failed.
	}

	//debug - cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;

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
		cout << "Could not set proxy blanket. Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

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

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 7: -------------------------------------------------
	// Get the data from the query in step 6 -------------------

	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hrPid = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);
		HRESULT hrName = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);
		HRESULT hrCmd = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

        VARIANT vtProp;

        // Get the value of the Name property
		//Convert Pid to a BSTR Type
		wchar_t PidStr[11]; // reserve len of 10 for Pid
		_itow(Pid, PidStr, 10);
		BSTR bstr_Pid = SysAllocString(PidStr);

		//Enum Process IDs of all processes
		hrPid = pclsObj->Get(L"Handle", 0, &vtProp, 0, 0);
		//compare Pids
		int bstr_comp = wcscmp(vtProp.bstrVal, bstr_Pid);

		if (bstr_comp == 0) {
			//if (vtProp.bstrVal == NULL) {
			//	wcout << "ProcessID:None";
			//}
			//else {
			//	wcout << " ProcessID:" << vtProp.bstrVal;
			//}
			//VariantClear(&vtProp);

			hrName = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			wcout << "\tProcess:" << vtProp.bstrVal;
			VariantClear(&vtProp);
			//Get commandline if available
			hrCmd = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);
			if (vtProp.bstrVal == NULL) {
				wcout << "\tNone";
			}
			else {
				wcout << "\t" << vtProp.bstrVal << endl;
			}
			VariantClear(&vtProp);
		}
        pclsObj->Release();

		// Cleanup
		// ========
	}
	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();

	return 0;   // Program successfully completed.

}