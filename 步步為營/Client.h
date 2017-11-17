#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <glut.h>
#include "WgSocket.h"
#include "Chessboard.h"
using namespace std;

#define SIGN_SPLIT (char)22
#define CHAT_MAX_CHAR 40

class Chessboard;

class Client : public WgSocket
{
private:
	string m_myName;
	bool m_isPlaying;
	vector<string> m_playerName;

	Chessboard *m_board;
	thread m_thread;
	string m_errorMsg;
	vector<string> m_chatBoxStr;
public:
	Client(Chessboard *board);
	~Client();
	void SetMyname(string name);

	bool Connect(string serverip, int port);
	void Disconnect();
	void RecvLoop();
	void ReadCommand(string cmd);

	//Send Function
	void SendStart();
	void SendDone();
	void SendMoveChess(int i, int j);
	void SendSetWall(int i, int j, int dir);
	void SendGameOver();
	void SendChat(string msg);
	
	//Get Function
	vector<string> GetPlayerName();
	bool IsPlaying();
	int GetChatLines();
	string GetChatRecord(int index);
	string GetErrorMsg();
};

Client::Client(Chessboard *board)
{
	m_board = board;
	m_errorMsg = "";
	m_isPlaying = false;
}

Client::~Client()
{
	Disconnect();
}

void Client::SetMyname(string name)
{
	m_myName = name;
}

bool Client::Connect(string serverip, int port) {
	Disconnect();
	if (!Open(serverip.c_str(), port)) {
		m_errorMsg =  "Error : Connect Failed";
		return false;
	}
	stringstream ss;
	ss << SG_PUSH << SIGN_SPLIT << m_myName << SIGN_SPLIT;
	Write(ss.str().c_str(), ss.str().size());

	m_thread = thread(&Client::RecvLoop, this);
	return true;
}

void Client::Disconnect()
{
	if (IsOpened()) {
		cout << "Client關閉" << endl;
	}
	Close();
	m_isPlaying = false;
	m_chatBoxStr.clear();
	if (m_thread.joinable())
		m_thread.join();
}

void Client::RecvLoop()
{
	while (m_Socket != INVALID_SOCKET) {
		if (WaitInputData(100)){
			char recvMsg[1024];
			int recvLen = Read(recvMsg, 1024);
			if (!IsOpened()) {
				return;
			}
			//Server Message
			if (recvLen != 0) {
				string recvStr(recvMsg, recvMsg + recvLen);
				cout << recvStr << endl;
				ReadCommand(recvStr);
			}
			//Server Disconnect
			else {
				Sleep(10); //確認是否是主動關閉
				if (IsOpened()) {
					m_errorMsg = "Error : Server Disconnect";
				}
				Close();
				m_isPlaying = false;
				return;
			}
		}
	}
}

void Client::ReadCommand(string cmd)
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
		case SG_NAMELIST: //args: <num> <name1>....
		{
			m_playerName.clear();
			int playerSize = stoi(args[1]);
			for (int i = 0; i < playerSize; i++) {
				m_playerName.push_back(args[2 + i]);
			}
			args.erase(args.begin(), args.begin() + 2 + playerSize);
		}
			break;
		case SG_NAMEUSED:
			m_errorMsg = "Error : Your Name is Already Used";
			Close();
			args.erase(args.begin(), args.begin() + 1);
			break;
		case SG_START:
			m_board->Init(m_playerName);
			m_isPlaying = true;
			SendDone();
			args.erase(args.begin(), args.begin() + 1);
			break;
		case SG_REMOVE: //args: <index>
		{
			int index = stoi(args[1]);
			m_playerName.erase(m_playerName.begin() + index);
			if (m_isPlaying && m_board->RemovePlayer(index)) {
				SendDone();
			}
			args.erase(args.begin(), args.begin() + 2);
		}
			break;
		case SG_NEXT: //args: <index>
		{
			int index = stoi(args[1]);
			if (!m_board->NextPlayer(index)) {
				SendMoveChess(-1, -1);
				SendDone();
			}
			args.erase(args.begin(), args.begin() + 2);
		}
			break;
		case SG_MOVECHESS: //args: <i> <j>
		{
			int i = stoi(args[1]), j = stoi(args[2]);
			if (i != -1 || j != -1) {
				m_board->MoveChess(i, j);
			}
			SendDone();
			args.erase(args.begin(), args.begin() + 3);
		}
			break;
		case SG_SETWALL: //args: <i> <j> <dir>
		{
			int i = stoi(args[1]), j = stoi(args[2]), dir = stoi(args[3]);
			m_board->SetWall(i, j, dir); 
			SendDone();
			args.erase(args.begin(), args.begin() + 4);
		}
			break;
		case SG_ECHO: //args:<name> <msg>
		{
			stringstream ss;
			string spaceStr;
			ss << "[" << args[1] << "]";
			int lengthPerLine = CHAT_MAX_CHAR - ss.str().size();
			for (int i = 0; i < ss.str().size(); i++) {
				spaceStr += " ";
			}
			ss << args[2].substr(0, min(lengthPerLine, args[2].size()));
			m_chatBoxStr.push_back(ss.str());
			for (int i = lengthPerLine; i < args[2].size(); i += lengthPerLine) {
				ss.str("");
				ss << spaceStr << args[2].substr(i, min(lengthPerLine, args[2].size() - i));
				m_chatBoxStr.push_back(ss.str());
			}
			args.erase(args.begin(), args.begin() + 3);
		}
			break;
		}
	}
}

//Send Function
void Client::SendStart()
{
	m_isPlaying = true;
	stringstream signMsg;
	signMsg << SG_START << SIGN_SPLIT;
	Write(signMsg.str().c_str(), signMsg.str().size());
}

void Client::SendDone()
{
	stringstream signMsg;
	signMsg << SG_DONE << SIGN_SPLIT;
	Write(signMsg.str().c_str(), signMsg.str().size());
}

void Client::SendMoveChess(int i, int j)
{
	stringstream signMsg;
	signMsg << SG_MOVECHESS << SIGN_SPLIT << i << SIGN_SPLIT << j << SIGN_SPLIT;
	Write(signMsg.str().c_str(), signMsg.str().size());
}

void Client::SendSetWall(int i, int j, int dir)
{
	stringstream signMsg;
	signMsg << SG_SETWALL << SIGN_SPLIT << i << SIGN_SPLIT << j << SIGN_SPLIT << dir << SIGN_SPLIT;
	Write(signMsg.str().c_str(), signMsg.str().size());
}

void Client::SendGameOver()
{
	stringstream signMsg;
	signMsg << SG_GAMEOVER << SIGN_SPLIT;
	Write(signMsg.str().c_str(), signMsg.str().size());
	m_isPlaying = false;
}

void Client::SendChat(string msg)
{
	stringstream signMsg;
	signMsg << SG_ECHO << SIGN_SPLIT << m_myName << SIGN_SPLIT << msg << SIGN_SPLIT;
	Write(signMsg.str().c_str(), signMsg.str().size());
}

bool Client::IsPlaying()
{ 
	return m_isPlaying;
}

vector<string> Client::GetPlayerName()
{
	return m_playerName;
}

int Client::GetChatLines()
{
	return m_chatBoxStr.size();
}

string Client::GetChatRecord(int index)
{
	int i = m_chatBoxStr.size() - 1 - index;
	if (i >= 0) {
		return m_chatBoxStr[i];
	}
	return "";
}

string Client::GetErrorMsg()
{
	string str;
	if (m_errorMsg != "") {
		str = m_errorMsg;
		m_errorMsg = "";
	}
	return str;
}

#endif