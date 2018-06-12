//CONNECT TO REMOTE HOST (CLIENT APPLICATION)
//Include the needed header files.
//Don't forget to link libws2_32.a to your program as well
#include <winsock.h>
#include <iostream>
#include "thread"
#include <stdio.h>
#include <sstream>
#include <ctime>
#include <string>
#include "C:\dev\netstat\tcpclient.h"
#define DEFAULT_BUFLEN 32

using namespace std;

SOCKET s; //Socket handle

//CONNECTTOHOST – Connects to a remote host
bool ConnectToHost(int PortNo, char* IPAddress)
{
	//Start up Winsock…
	WSADATA wsadata;
	
	int error = WSAStartup(0x0202, &wsadata);

	//Did something happen?
	if (error)
		return false;

	//Did we get the right Winsock version?
	if (wsadata.wVersion != 0x0202)
	{
		WSACleanup(); //Clean up Winsock
		return false;
	}

	SOCKADDR_IN target; //Socket address information

	target.sin_family = AF_INET; // address family Internet
	target.sin_port = htons(PortNo); //Port to connect on
	target.sin_addr.s_addr = inet_addr(IPAddress); //Target IP

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket

	if (s == INVALID_SOCKET)
	{
		cout << "[!] Failed at socket" << endl;
		return false; //Couldn't create the socket
	}

	if (connect(s, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR)
	{
		cout << WSAGetLastError() << endl;
		cout << "[!] Couldn't connect" << endl;
		return false; //Couldn't connect
	}
	else
		cout << "[+] Connected." << endl;
		return true; //Success
}

void CloseConnection()
{
	//Close the socket if it exists
	if (s)
		closesocket(s);

	WSACleanup(); //Clean up Winsock
}

int sendData(std::string sendbuf)
{
	std::string close_connection = "end";
	bool end = true;

	cout << "[*] Sending buffer.." << endl;				//DEBUG
	int sendres;
	int recvbuflen = DEFAULT_BUFLEN ;
	char recvbuf[DEFAULT_BUFLEN];
	memset(recvbuf, '\0', DEFAULT_BUFLEN);
	sendres = send(s, sendbuf.data(), sendbuf.size(), 0);
	if (sendres == SOCKET_ERROR)
	{
		cout << "[!] Unable to connect" << endl; //DEBUG
		return 1;
	}
	

	//Handle connection hangs
	clock_t startTime = clock(); //Start timer
	double secondsPassed;
	double secondsToDelay = atof("10");

	std::string closeMessage;

	do 
	{

		secondsPassed = (clock() - startTime) / CLOCKS_PER_SEC;
		if (secondsPassed >= secondsToDelay)
		{
			closeMessage = "[!]Connection timed out.";
			end = false;
			return 1;
			break;
		}

		sendres = recv(s, recvbuf, recvbuflen, 0);
		if (recvbuf == close_connection )
		{
			closeMessage = "[+] Remote terminated connection successfully. ";
			end = false;
			return 0;
			break;
		}
		if (sendres > 0)
		{
			cout << "[+] Bytes received: " << recvbuf << endl;
			std::this_thread::sleep_for(2s);

		}
		else if (sendres == 0)
		{
			closeMessage = "[+] Connection closed unexpectedly.";
			std::this_thread::sleep_for(2s);
		}

	} while (end != true);

	cout << closeMessage << endl;
	CloseConnection();

	return 0;
}


