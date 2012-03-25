#include "StdAfx.h"
#include "RTT4TCPController.h"
#include "RTT4TCPAccessor.h"
#include "I4C3DCommon.h"
#include "Miscellaneous.h"
#include <math.h>

const int BUFFER_SIZE = 256;

static char g_cTermination = '?';

const PCSTR COMMAND_TUMBLE	= "tumble";
const PCSTR COMMAND_TRACK	= "track";
const PCSTR COMMAND_DOLLY	= "dolly";
const PCSTR INITIALIZE_MESSAGE	= "SUBSCRIBE CAMERA";

const double M_PI = asin(1.0) * 4.0;

inline void SendCommand()
{
}

RTT4TCPController::RTT4TCPController(void)
{
}


RTT4TCPController::~RTT4TCPController(void)
{
}

BOOL RTT4TCPController::Initialize(LPCSTR szBuffer, char* termination, USHORT uRTTPort)
{
	char tmpCommand[BUFFER_SIZE] = {0};
	char szModKeys[BUFFER_SIZE] = {0};
	double tumble, track, dolly;

	sscanf_s(szBuffer, g_initCommandFormat, tmpCommand,	sizeof(tmpCommand), szModKeys, sizeof(szModKeys), &tumble, &track, &dolly, termination, sizeof(*termination));
	//sscanf_s(szBuffer, g_initCommandFormatRTT4TCP, tmpCommand, sizeof(tmpCommand), &m_rttContext.move, &m_rttContext.angle, &m_rttContext.pitch, &m_rttContext.height, termination, sizeof(*termination));
	m_rttContext.move = 7;
	m_rttContext.angle = 1;
	m_rttContext.pitch = 1;
	m_rttContext.height = 2;
	m_rttContext.speed = 1;

	g_cTermination = *termination;

	// TCPソケットの作成
	RTT4TCPAccessor accessor;
	m_rttContext.socketHandler = accessor.InitializeTCPSocket(&m_rttContext.address, "127.0.0.1", TRUE, uRTTPort);
	if (m_rttContext.socketHandler == INVALID_SOCKET) {
		LogDebugMessage(Log_Error, _T("InitializeSocket <RTT4TCPController::Initialize>"));
		return FALSE;
	}
	if (!accessor.SetConnectingSocket(m_rttContext.socketHandler, &m_rttContext.address)) {
		return FALSE;
	}

	// CAMERA情報取得のためのメッセージ送信
	send(m_rttContext.socketHandler, INITIALIZE_MESSAGE, strlen(INITIALIZE_MESSAGE), 0);

	return TRUE;
}

void RTT4TCPController::UnInitialize(void)
{
	if (m_rttContext.socketHandler != INVALID_SOCKET) {
		closesocket(m_rttContext.socketHandler);
		m_rttContext.socketHandler = INVALID_SOCKET;
	}
}

void RTT4TCPController::Execute(LPCSTR szCommand, double deltaX, double deltaY)
{
	if (_strcmpi(szCommand, COMMAND_TUMBLE) == 0) {
		//LogDebugMessage(Log_Debug, _T("Tumble execute"));
		TumbleExecute((INT)deltaX, (INT)deltaY);

	} else if (_strcmpi(szCommand, COMMAND_TRACK) == 0) {
		//LogDebugMessage(Log_Debug, _T("Track execute"));
		TrackExecute((INT)deltaX, (INT)deltaY);

	} else if (_strcmpi(szCommand, COMMAND_DOLLY) == 0) {
		//LogDebugMessage(Log_Debug, _T("Dolly execute"));
		DollyExecute((INT)deltaX, (INT)deltaY);

	} else {
		char szMacroValue[BUFFER_SIZE];
		GetMacroValue(szCommand, szMacroValue, _countof(szMacroValue));

		send(m_rttContext.socketHandler, szMacroValue, strlen(szMacroValue), 0);
	}
}

void RTT4TCPController::ExecuteCameraCommand(void)
{
	char message[BUFFER_SIZE];
	sprintf_s(message, _countof(message), "POSORIENT PHR CAMERA %d %d %d %d %d %d;",
		m_rttContext.x, m_rttContext.y, m_rttContext.z,
		m_rttContext.p, m_rttContext.h, m_rttContext.r);
	
	send(m_rttContext.socketHandler, message, strlen(message), 0);
}

void RTT4TCPController::TumbleExecute(int deltaX, int deltaY)
{
	// カメラ向き上下：pitch
	// カメラ向き左右：head

	// カメラ右
	if (deltaX > 0) {
		m_rttContext.h -= (int)m_rttContext.angle * m_rttContext.speed;
	// カメラ左
	} else if (deltaX < 0) {
		m_rttContext.h += (int)m_rttContext.angle * m_rttContext.speed;
	}

	// カメラ下
	if (deltaY > 0) {
		m_rttContext.p += (int)m_rttContext.pitch * m_rttContext.speed;
            //if (root.p > 88)
            //{
            //    root.p = 88;
            //    root.r = 0;
            //}
	// カメラ上
	} else if (deltaY < 0) {
		m_rttContext.p -= (int)m_rttContext.pitch * m_rttContext.speed;
            //if (root.p < -88)
            //{
            //    root.p = -88;
            //    root.r = 0;
            //}
	}

	ExecuteCameraCommand();
}

void RTT4TCPController::TrackExecute(int /*deltaX*/, int deltaY)
{
	// z
	if (deltaY > 0) {	// 上昇
		m_rttContext.z += (int)m_rttContext.height * m_rttContext.speed;
	} else {			// 下降
		m_rttContext.z -= (int)m_rttContext.height * m_rttContext.speed;
	}
	ExecuteCameraCommand();
}

void RTT4TCPController::DollyExecute(int /*deltaX*/, int deltaY)
{
	// xy
	if (deltaY > 0) {	// 前進
		m_rttContext.x += (int)(m_rttContext.move * m_rttContext.speed * sin(M_PI / 180 * m_rttContext.h));
		m_rttContext.y -= (int)(m_rttContext.move * m_rttContext.speed * cos(M_PI / 180 * m_rttContext.h));
	} else {			// 後退
		m_rttContext.x -= (int)(m_rttContext.move * m_rttContext.speed * sin(M_PI / 180 * m_rttContext.h));
		m_rttContext.y += (int)(m_rttContext.move * m_rttContext.speed * cos(M_PI / 180 * m_rttContext.h));
	}
	ExecuteCameraCommand();
}
