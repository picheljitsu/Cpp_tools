#ifndef TCPCLIENT_H
#define TCPCLIENT_H

bool ConnectToHost(int PortNo, char* IPAddress);
void CloseConnection();
int sendData(std::string sendbuf);

#endif // !TCPCLIENT_H
