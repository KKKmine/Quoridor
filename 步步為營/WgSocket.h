#ifndef WGSOCKET_H
#define WGSOCKET_H

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
using namespace std;

const int MAX_RESEND_NUM = 10;

class WgSocket
{
protected:
	SOCKET m_Socket;
public:
	static bool Initialize();
	static void Terminate();
	static bool IsLocalHost(const char* hostname);
	//static void GetHostIP(const char* hostname, char* ipStr);

	WgSocket();
	WgSocket(SOCKET socket);
	~WgSocket();
	bool SetNoDelay();

	bool Listen(int port);
	bool Accept(SOCKET &socket);
	bool Open(const char* hostname, int port);
	void Close();
	bool WaitInputData(int seconds);
	int Read(char* data, int len);
	bool Write(const void* data, int len);
	bool IsOpened() const;
};

static bool m_InitFlag = false;

bool WgSocket::Initialize()
//說明：初始化Socket物件
//傳回：失敗傳回false
{
	if (!m_InitFlag) {
		WSAData wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
			return false;
		}
		m_InitFlag = true;
	}
	return true;
}

void WgSocket::Terminate()
//說明：結束Socket物件
{
	if (m_InitFlag) {
		WSACleanup();
		m_InitFlag = false;
	}
}

bool WgSocket::IsLocalHost(const char* hostname)
//說明：檢查是否為localhost呼叫
//輸入：hostname = Server位址
//傳回：是否為localhost呼叫
{
	if (hostname == NULL ||
		*hostname == 0 ||
		stricmp(hostname, "localhost") == 0 ||
		strcmp(hostname, "127.0.0.1") == 0) {
		return true;
	}
	return false;
}
/*
void WgSocket::GetHostIP(const char* hostname, char* ipStr)
//說明：取得指定host的ip
//輸入：hostname = host位址
//輸出：IP string
{
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (IsLocalHost(hostname)) {
		char name[128];
		gethostname(name, sizeof name);
		hostname = name;
	}
	getaddrinfo(hostname, NULL, &hints, &servinfo);
	for (p = servinfo; p != NULL; p = p->ai_next) {
		void *addr;
		char *ipver;

		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		inet_ntop(p->ai_family, addr, ipStr, sizeof ipStr);
	}
	freeaddrinfo(servinfo);
}
*/

WgSocket::WgSocket()
//說明：建構Socket物件
{
	m_Socket = INVALID_SOCKET;
}

WgSocket::WgSocket(SOCKET socket)
{
	m_Socket = socket;
}

WgSocket::~WgSocket()
//說明：解構Socket物件
{
	Close();
}

bool WgSocket::SetNoDelay()
//說明：設定不延遲傳送 (停掉Nagle Algorithm)
//傳回：設定失敗傳回false
{
	if (!IsOpened()) {
		return false;
	}
	int on = 1;
	if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) != 0) {
		return false;
	}
	return true;
}

bool WgSocket::Listen(int port)
//說明：接聽某個Port
//輸入：port = 接聽Port
//傳回：失敗傳回false
{
	Close();
	if (!Initialize()) {
		return false;
	}
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = INADDR_ANY;
	sock_addr.sin_port = htons(port);
	//建立socket
	try {
		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	}
	catch (...) {
		m_Socket = INVALID_SOCKET;
		return false;
	}
	if (m_Socket == INVALID_SOCKET) {
		return false;
	}
	//Bind socket
	int on = 1;
	setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	int rc;
	try {
		rc = ::bind(m_Socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	}
	catch (...) {
		rc = SOCKET_ERROR;
	}
	if (rc == SOCKET_ERROR) {
		Close();
		return false;
	}
	//Listen socket
	try {
		rc = listen(m_Socket, SOMAXCONN);
	}
	catch (...) {
		rc = SOCKET_ERROR;
	}
	if (rc == SOCKET_ERROR) {
		Close();
		return false;
	}
	return true;
}

bool WgSocket::Accept(SOCKET &socket)
//說明：等待接收連線
//輸出：連線socket
//傳回：失敗傳回false
{
	socket = INVALID_SOCKET;
	if (!IsOpened()) {
		return false;
	}
	struct sockaddr_in from;
	int fromlen = sizeof(from);
	try {
		socket = accept(m_Socket, (struct sockaddr*)&from, &fromlen);
	}
	catch (...) {
		socket = INVALID_SOCKET;
		return false;
	}
	if (socket == INVALID_SOCKET) {
		return false;
	}
	return true;
}

bool WgSocket::Open(const char* hostname, int port)
//說明：開啟與Server的連線
//輸入：hostname,port = Server位址與通訊埠
//傳回：失敗傳回false
{
	Close();
	if (!Initialize()) {
		return false;
	}
	struct sockaddr_in sock_addr;
	//解出socket address
	if (IsLocalHost(hostname)) {
		hostname = "127.0.0.1";
	}
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	struct hostent *hostinfo = gethostbyname(hostname);
	if (hostinfo == NULL) {
		return false;
	}
	sock_addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	//建立socket
	try {
		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	}
	catch (...) {
		cerr << "建立Socket失敗\n";
		m_Socket = INVALID_SOCKET;
		return false;
	}
	if (m_Socket == INVALID_SOCKET) {
		return false;
	}
	//開始連線
	/*if (!connect(m_Socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) >= 0) {
		Close();
		return false;
	}*/
	try {
		if (connect(m_Socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) >= 0)
			return true;
		else {
			return false;
		}
	}
	catch (...) {
		cerr << "連線失敗\n";
		return false;
	}
	return true;
}

void WgSocket::Close()
//說明：關閉與Server的連線
{
	if (!IsOpened()) {
		return;
	}
	shutdown(m_Socket, SD_SEND);
	closesocket(m_Socket);

	m_Socket = INVALID_SOCKET;
}

bool WgSocket::WaitInputData(int seconds)
//說明：等待對方送來資料
//輸入：seconds = 等待秒數
//傳回：沒有資料傳回false
{
	if (!IsOpened()) {
		return false;
	}
	fd_set socket_set;
	FD_ZERO(&socket_set);
	FD_SET(m_Socket, &socket_set);
	
	struct timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	
	if (select(FD_SETSIZE, &socket_set, NULL, NULL, &timeout) <= 0) {
		return false;
	}
	return true;
}

int WgSocket::Read(char* data, int len)
//說明：讀取資料
//輸入：data, len = 資料緩衝區與大小
//輸出：data = 讀取的資料
//傳回：失敗傳回false
//備註：本函數會一直等到有讀取資料或結束連線時才傳回
{
	int ret_len = 0;
	if (!IsOpened()) {
		return 0;
	}
	try{
		ret_len = recv(m_Socket, (char*)data, len, 0);
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

bool WgSocket::Write(const void* data, int len)
//說明：送出資料
//輸入：data, len = 資料緩衝區與大小
//傳回：失敗傳回false
{
	if (!IsOpened()) {
		return false;
	}
	if (len <= 0) {
		return true;
	}
	//重傳直到達到重試上限
	for (int i = 0; i < MAX_RESEND_NUM; i++) {
		if (send(m_Socket, (const char*)data, len, 0) == len) {
			break;
		}
		else if (i == MAX_RESEND_NUM - 1) {
			return false;
		}
		Sleep(100);
	}
	return true;
}

bool WgSocket::IsOpened() const
//說明：檢測Socket是否已開啟
//傳回：檢測結果
{
	return m_Socket != INVALID_SOCKET;
}

#endif