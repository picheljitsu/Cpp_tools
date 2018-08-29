// EventSink.cpp
#include "EventSink.h"
#include <string>
#include <iostream>
#include <stdio.h>

ULONG EventSink::AddRef()
{
	return InterlockedIncrement(&m_lRef);
}

ULONG EventSink::Release()
{
	LONG lRef = InterlockedDecrement(&m_lRef);
	if (lRef == 0)
		delete this;
	return lRef;
}

HRESULT EventSink::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
	{
		*ppv = (IWbemObjectSink *) this;
		AddRef();
		return WBEM_S_NO_ERROR;
	}
	else return E_NOINTERFACE;
}


HRESULT EventSink::Indicate(long lObjectCount,
	IWbemClassObject **apObjArray)
{
	HRESULT hr = S_OK;
	_variant_t vtProp;

	for (int i = 0; i < lObjectCount; i++)
		
	{
		printf("Number: %d", i);

		hr = apObjArray[i]->Get(_bstr_t(L"TargetInstance"), 0, &vtProp, 0, 0);
		if (!FAILED(hr))
		{
			IUnknown* str = vtProp;
			hr = str->QueryInterface(IID_IWbemClassObject, reinterpret_cast< void** >(&apObjArray[i]));
			if (SUCCEEDED(hr))
			{
				_variant_t cn;
				hr = apObjArray[i]->Get(L"Caption", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
						wcout << "Process: " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl;
					else
						if ((cn.vt & VT_ARRAY))
							wcout << "Process:  " << "Array types not supported (yet)" << endl;
						else
							wcout << "Process:  " << cn.bstrVal << endl;
				}

				VariantClear(&cn);

				hr = apObjArray[i]->Get(L"CommandLine", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
						wcout << "CommandLine: " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl;
					else
						if ((cn.vt & VT_ARRAY))
							wcout << "CommandLine : n/a" << endl;
						else
							wcout << "CommandLine : " << cn.bstrVal << endl;
				}

				VariantClear(&cn);

				hr = apObjArray[i]->Get(L"Handle", 0, &cn, NULL, NULL);
				if (SUCCEEDED(hr))
				{
					if ((cn.vt == VT_NULL) || (cn.vt == VT_EMPTY))
						wcout << "ProcessID: " << ((cn.vt == VT_NULL) ? "NULL" : "EMPTY") << endl;
					else
						if ((cn.vt & VT_ARRAY))
							wcout << "ProcessID: n/a" << "Array types not supported (yet)" << endl;
						else
							wcout << "ProcessID: " << cn.bstrVal << endl;
				}
				VariantClear(&cn);
			}
		}
		VariantClear(&vtProp);

	}

	return WBEM_S_NO_ERROR;
}

HRESULT EventSink::SetStatus(
	/* [in] */ LONG lFlags,
	/* [in] */ HRESULT hResult,
	/* [in] */ BSTR strParam,
	/* [in] */ IWbemClassObject __RPC_FAR *pObjParam
)
{
	if (lFlags == WBEM_STATUS_PROGRESS)
	{
		printf("Process Starting.\n");
	}

	return WBEM_S_NO_ERROR;
}    // end of EventSink.cpp
