#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <thread>
#include <vector>
#include "WgSocket.h"
using namespace std;

#define MAX_PLAYER 4
#define SIGN_SPLIT (char)22

enum Sign {
	SG_NAMELIST, //<num> <name1>....
	SG_REMOVE, //<index>
	SG_PUSH, //<name>
	SG_NAMEUSED,
	SG_START,
	SG_DONE,
	SG_NEXT, //<index>
	SG_MOVECHESS, //<i> <j>
	SG_SETWALL, //<i> <j> <dir>
	SG_GAMEOVER,
	SG_ECHO //<name> <msg>
};

struct ClientData {
	SOCKET fd = SOCKET_ERROR;
	string name = "";
	bool isDone = false;
};

class Server : public WgSocket
{
private:
	bool m_isOpened;
	int m_port;
	int m_gameOrder;
	vector<ClientData> m_clientList;
	fd_set m_fdClient;

	thread m_thread;
	string m_errorMsg;
public:
	Server();
	~Server();
	bool StartServer(int port);
	void CloseServer();
	void RecvLoop();

	void ReadCommand(int clientIndex, string cmd);
	bool SendAll(string msg, int exceptIndex);
	int ReadClient(const SOCKET sock, char* data, int len);
	bool WriteClient(const SOCKET sock, const void* data, int len);

	int GetIndex(int fd);
	bool IsAllDone();
	bool IsOpenServer();
	string GetErrorMsg();
};

Server::Server()
{
	m_gameOrder = -1;
	m_errorMsg = "";
	m_isOpened = false;
}

Server::~Server()
{
	CloseServer();
}

bool Server::StartServer(int port)
{
	CloseServer();
	if (!Listen(port)) {
		cout << "<server>Server啟動失敗\n";
		m_errorMsg = "Error : Failed to Startup Server";
		return false;
	}
	cout << "<server>Server啟動成功\n";
	m_isOpened = true;
	m_port = port;
	m_gameOrder = -1;

	m_thread = thread(&Server::RecvLoop, this);
	return true;
}

void Server::CloseServer() {
	if (m_isOpened) {
		cout << "<server>Server關閉\n";
	}
	m_isOpened = false;
	FD_CLR(m_Socket, &m_fdClient);
	Close();
	for (auto c : m_clientList) {
		closesocket(c.fd);
	}
	if (m_thread.joinable()) {
		m_thread.join();
	}
	m_clientList.clear();
}

void Server::RecvLoop()
{
	fd_set read_fds;
	FD_ZERO(&m_fdClient);
	FD_SET(m_Socket, &m_fdClient);
	while (m_isOpened) {
		read_fds = m_fdClient;
		if (select(NULL, &read_fds, NULL, NULL, NULL) == -1) {
			cout << "<server>Select Failed\n";
			return;
		}
		if (!m_isOpened) {
			return;
		}
		for (int i = 0; i < read_fds.fd_count; i++) {
			//Accept
			if (read_fds.fd_array[i] == m_Socket) {
				SOCKET socket;
				if (Accept(socket)) {
					FD_SET(socket, &m_fdClient);
					ClientData c;
					c.fd = socket;
					m_clientList.push_back(c);
					if (m_clientList.size() >= MAX_PLAYER) {
						FD_CLR(m_Socket, &m_fdClient);
						Close();
						break;
					}
				}
			}
			//Recv
			else {
				int socket = read_fds.fd_array[i];
				int index = GetIndex(socket);
				char recvMsg[1024];
				int recvLen = ReadClient(socket, recvMsg, 1024);
				//Client Message
				if (recvLen != 0) {
					string strData(recvMsg, recvMsg + recvLen);
					cout << "Client" << socket << ":" << strData << endl;
					ReadCommand(index, strData);
				}
				//Client Disconnect
				else {
					FD_CLR(socket, &m_fdClient);
					closesocket(socket);
					if (m_clientList[index].name != "") {
						stringstream ss;
						ss << SG_REMOVE << SIGN_SPLIT << index << SIGN_SPLIT;
						SendAll(ss.str(), index);
					}
					m_clientList.erase(m_clientList.begin() + index);
					if (m_gameOrder != -1) {
						m_gameOrder = (m_gameOrder + m_clientList.size() - 1) % m_clientList.size();
					}
					if (!IsOpened() && m_gameOrder == -1) { //未listen中 & 非遊戲中
						Listen(m_port);
						FD_SET(m_Socket, &m_fdClient);
					}
					break;
				}
			}
		}
	}
}

void Server::ReadCommand(int clientIndex, string cmd)
{
	stringstream ss(cmd), signMsg;
	string token;
	vector<string> args;

	while (getline(ss, token, SIGN_SPLIT)) {
		args.push_back(token);
	}
	while (args.size() > 0) {
		Sign sign = (Sign)stoi(args[0]);
		switch (sign) {
		case SG_PUSH: //args: <name>
			for (int i = 0; i < m_clientList.size(); i++) {
				//名字衝突
				if (m_clientList[i].name == args[1]) {
					signMsg << SG_NAMEUSED << SIGN_SPLIT;
					WriteClient(m_clientList[clientIndex].fd, signMsg.str().c_str(), signMsg.str().size());
					break;
				}
				//重新校正名單
				else if (i == m_clientList.size() - 1) {
					m_clientList[clientIndex].name = args[1];
					signMsg << SG_NAMELIST << SIGN_SPLIT << m_clientList.size() << SIGN_SPLIT;
					for (ClientData c : m_clientList) {
						signMsg << c.name << SIGN_SPLIT;
					}
					SendAll(signMsg.str(), -1);
				}
			}
			args.erase(args.begin(), args.begin() + 2);
			break;
		case SG_START:
			FD_CLR(m_Socket, &m_fdClient);
			Close();
			signMsg << SG_START << SIGN_SPLIT;
			SendAll(signMsg.str(), clientIndex);
			args.erase(args.begin(), args.begin() + 1);
			break;
		case SG_DONE:
			m_clientList[clientIndex].isDone = true;
			args.erase(args.begin(), args.begin() + 1);
			break;
		case SG_MOVECHESS: //args: <i> <j>
			signMsg << args[0] << SIGN_SPLIT << args[1] << SIGN_SPLIT << args[2] << SIGN_SPLIT;
			SendAll(signMsg.str(), clientIndex);
			args.erase(args.begin(), args.begin() + 3);
			break;
		case SG_SETWALL: //args: <i> <j> <dir>
			signMsg << args[0] << SIGN_SPLIT << args[1] << SIGN_SPLIT << args[2] << SIGN_SPLIT << args[3] << SIGN_SPLIT;
			SendAll(signMsg.str(), clientIndex);
			args.erase(args.begin(), args.begin() + 4);
			break;
		case SG_GAMEOVER:
			m_gameOrder = -1;
			if (m_clientList.size() < 4) {
				Listen(m_port);
				FD_SET(m_Socket, &m_fdClient);
			}
			//重新校正名單
			signMsg << SG_NAMELIST << SIGN_SPLIT << m_clientList.size() << SIGN_SPLIT;
			for (ClientData c : m_clientList) {
				signMsg << c.name << SIGN_SPLIT;
			}
			SendAll(signMsg.str(), -1);
			args.erase(args.begin(), args.begin() + 1);
			break;
		case SG_ECHO: //args:<name> <msg>
			signMsg << args[0] << SIGN_SPLIT << args[1] << SIGN_SPLIT << args[2] << SIGN_SPLIT;
			SendAll(signMsg.str(), -1);
			args.erase(args.begin(), args.begin() + 3);
			break;
		}
		if (IsAllDone()) { //Next Turn
			m_gameOrder = (m_gameOrder + 1) % m_clientList.size();
			stringstream nextSign;
			nextSign << SG_NEXT << SIGN_SPLIT << m_gameOrder << SIGN_SPLIT;
			SendAll(nextSign.str(), nextSign.str().size());
		}
	}
}

bool Server::SendAll(string msg, int exceptIndex)
//參數:exceptIndex 不發送的client index  -1為全部都傳
{
	for (int i = 0; i < m_clientList.size(); i++) {
		if (i != exceptIndex) {
			if (!WriteClient(m_clientList[i].fd, msg.c_str(), msg.size())) {
				return false;
			}
		}
	}
	return true;
}

int Server::GetIndex(int fd)
{
	for (int i = 0; i < m_clientList.size(); i++) {
		if (m_clientList[i].fd == fd) {
			return i;
		}
	}
	return -1;
}

bool Server::IsAllDone()
{
	for (ClientData c : m_clientList) {
		if (!c.isDone) {
			return false;
		}
	}
	for (int i = 0; i < m_clientList.size(); i++) {
		m_clientList[i].isDone = false;
	}
	return true;
}

int Server::ReadClient(const SOCKET sock, char* data, int len)
{
	int ret_len = 0;
	if (sock == INVALID_SOCKET) {
		return 0;
	}
	try {
		ret_len = recv(sock, (char*)data, len, 0);
	}
	catch (...) {
		ret_len = SOCKET_ERROR;
		cerr << "Failed to Recv Message" << endl;
	}
	if (ret_len < 0) {
		ret_len = 0;
	}
	return ret_len;
}

bool Server::WriteClient(const SOCKET sock, const void* data, int len)
{
	if (sock == INVALID_SOCKET) {
		return false;
	}
	if (len <= 0) {
		return true;
	}
	//重傳直到達到重試上限
	for (int i = 0; i < MAX_RESEND_NUM; i++) {
		if (send(sock, (const char*)data, len, 0) == len) {
			break;
		}
		else if (i == MAX_RESEND_NUM - 1) {
			return false;
		}
		Sleep(100);
	}
	return true;
}

bool Server::IsOpenServer()
{
	return m_isOpened;
}

string Server::GetErrorMsg()
{
	string str;
	if (m_errorMsg != "") {
		str = m_errorMsg;
		m_errorMsg = "";
	}
	return str;
}

#endif