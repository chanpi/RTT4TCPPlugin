#pragma once

#include "stdafx.h"

class RTT4TCPAccessor
{
public:
	RTT4TCPAccessor(void);
	~RTT4TCPAccessor(void);

	SOCKET InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort);
	BOOL SetConnectingSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress);
};

