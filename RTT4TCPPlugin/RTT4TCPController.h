#pragma once
#include "RTT4TCPMacroPlugin.h"

class RTT4TCPController : public RTT4TCPMacroPlugin
{
public:
	RTT4TCPController(void);
	virtual ~RTT4TCPController(void);

	virtual BOOL Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort);
	virtual void Execute(LPCSTR szCommand, double deltaX, double deltaY);
	virtual void UnInitialize(void);

protected:
	typedef struct RTTContext
	{
		SOCKET socketHandler;
		struct sockaddr_in address;

		int move;
		int angle;
		int pitch;
		int height;
		int x;
		int y;
		int z;
		int p;
		int h;
		int r;
		int speed;
	} RTTContext;
	RTTContext m_rttContext;

	virtual void TumbleExecute(int deltaX, int deltaY);
	virtual void TrackExecute(int deltaX, int deltaY);
	virtual void DollyExecute(int deltaX, int deltaY);
	void ExecuteCameraCommand(void);
};

