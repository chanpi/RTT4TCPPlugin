// RTT4TCPPlugin.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "RTT4TCPPlugin.h"
#include "RTT4TCPController.h"
#include "Miscellaneous.h"
#include "I4C3DCommon.h"
#include <WinSock2.h>
#include <ShellAPI.h>

#include <cstdlib>	// 必要

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define new  ::new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#endif

#define MAX_LOADSTRING 100

const int BUFFER_SIZE = 256;
static const PCSTR COMMAND_INIT	= "init";
static const PCSTR COMMAND_EXIT	= "exit";
static const PCSTR COMMAND_REGISTERMACRO = "registermacro";

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名
static USHORT g_uPort = 0;
static USHORT g_uRTTPort = 0;

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static int AnalyzeMessage(I4C3DUDPPacket* pPacket, LPSTR szCommand, SIZE_T size, double* pDeltaX, double* pDeltaY, char cTermination);

static SOCKET InitializeController(HWND hWnd, USHORT uPort);
static void UnInitializeController(SOCKET socketHandler);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


#if DEBUG || _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

 	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RTT4TCPPLUGIN, szWindowClass, MAX_LOADSTRING);

	if (!ExecuteOnce(szTitle)) {
		return EXIT_SUCCESS;
	}

	MyRegisterClass(hInstance);

	int argc = 0;
	LPTSTR *argv = NULL;
	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argc != 3) {
		MessageBox(NULL, _T("[ERROR] 引数が足りません[例: RTTPlugin.exe 10005 3333]。<RTT4TCPPlugin>"), szTitle, MB_OK | MB_ICONERROR);
		LocalFree(argv);
		CleanupMutex();
		return EXIT_FAILURE;
	}
	g_uPort = static_cast<USHORT>(_wtoi(argv[1]));
	OutputDebugString(argv[1]);
	g_uRTTPort = static_cast<USHORT>(_wtoi(argv[2]));
	OutputDebugString(argv[2]);
	LocalFree(argv);

	static WSAData wsaData;
	WORD wVersion;
	int nResult;

	wVersion = MAKEWORD(2,2);
	nResult = WSAStartup(wVersion, &wsaData);
	if (nResult != 0) {
		MessageBox(NULL, _T("[ERROR] Initialize Winsock."), szTitle, MB_OK | MB_ICONERROR);
		CleanupMutex();
		return EXIT_FAILURE;
	}
	if (wsaData.wVersion != wVersion) {
		MessageBox(NULL, _T("[ERROR] Winsock バージョン."), szTitle, MB_OK | MB_ICONERROR);
		WSACleanup();
		CleanupMutex();
		return EXIT_FAILURE;
	}

	LOG_LEVEL logLevel = Log_Error;
#if _DEBUG || DEBUG
	logLevel = Log_Debug;
#else
	logLevel = Log_Error;
#endif
	if (!LogFileOpenW("RTT4TCP", logLevel)) {
		ReportError(_T("RTT4TCPのログは出力されません。"));
	}
	LogDebugMessage(Log_Debug, _T("RTT4TCP log file opened."));

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		WSACleanup();
		CleanupMutex();
		LogFileCloseW();
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RTT4TCPPLUGIN));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();
	CleanupMutex();
	LogFileCloseW();
	LogDebugMessage(Log_Debug, _T("RTT4TCP log file closed."));
	return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
//  コメント:
//
//    この関数および使い方は、'RegisterClassEx' 関数が追加された
//    Windows 95 より前の Win32 システムと互換させる場合にのみ必要です。
//    アプリケーションが、関連付けられた
//    正しい形式の小さいアイコンを取得できるようにするには、
//    この関数を呼び出してください。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RTT4TCPPLUGIN));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RTT4TCPPLUGIN);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	static RTT4TCPController controller;
	static SOCKET i4c3dSocketHandler = INVALID_SOCKET;
	static char cTermination = '?';

	I4C3DUDPPacket packet = {0};
	int nBytes = 0;
	char szCommand[32] = {0};
	double deltaX = 0;
	double deltaY = 0;
	TCHAR szError[BUFFER_SIZE] = {0};

	switch (message)
	{
	case WM_CREATE:
		i4c3dSocketHandler = InitializeController(hWnd, g_uPort);
		break;
	
	case MY_WINSOCKSELECT:
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			nBytes = recv(i4c3dSocketHandler, (char*)&packet, sizeof(packet), 0);
			if (nBytes == SOCKET_ERROR) {
				_stprintf_s(szError, _countof(szError), _T("recv() : %d <RTT4TCPPlugin>"), WSAGetLastError());
				LogDebugMessage(Log_Error, szError);
				//ReportError(szError);
				break;

			}

			int scanCount = AnalyzeMessage(&packet, szCommand, _countof(szCommand), &deltaX, &deltaY, cTermination);
			if (scanCount == 3) {
				controller.Execute(szCommand, deltaX, deltaY);
				Sleep(1);

			} else if (scanCount == 1) {
				if (_strcmpi(szCommand, COMMAND_INIT) == 0) {
					if (!controller.Initialize(packet.szCommand, &cTermination, g_uRTTPort)) {
						_stprintf_s(szError, _countof(szError), _T("RTT4TCPコントローラの初期化に失敗しています。"));
						ReportError(szError);
					}

				} else if (_strcmpi(szCommand, COMMAND_EXIT) == 0) {
					controller.UnInitialize();
					OutputDebugString(_T("exit\n"));
					DestroyWindow(hWnd);

				} else if (_strcmpi(szCommand, COMMAND_REGISTERMACRO) == 0) {	// マクロ登録
					if (!controller.RegisterMacro(packet.szCommand, &cTermination)) {
						_stprintf_s(szError, _countof(szError), _T("RTT4TCPコントローラのマクロの登録に失敗しています。"));
						ReportError(szError);
					}
				} else {	// マクロ実行
					controller.Execute(szCommand, 0, 0);
				}
			}
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;

	case MY_I4C3DREBOOT:
		UnInitializeController(i4c3dSocketHandler);
		i4c3dSocketHandler = InitializeController(hWnd, g_uPort);
		break;

	case MY_I4C3DDESTROY:
	case WM_CLOSE:
	case WM_DESTROY:
		UnInitializeController(i4c3dSocketHandler);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

SOCKET InitializeController(HWND hWnd, USHORT uPort)
{
	SOCKET socketHandler;
	SOCKADDR_IN address;
	TCHAR szError[BUFFER_SIZE];
	int nResult = 0;

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		//ReportError(szError);
		LogDebugMessage(Log_Error, szError);
		return INVALID_SOCKET;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(uPort);
	address.sin_addr.S_un.S_addr = INADDR_ANY;

	nResult = bind(socketHandler, (const SOCKADDR*)&address, sizeof(address));
	if (nResult == SOCKET_ERROR) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] bind() : %d"), WSAGetLastError());
		//ReportError(szError);
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		return INVALID_SOCKET;
	}

	if (WSAAsyncSelect(socketHandler, hWnd, MY_WINSOCKSELECT, FD_READ) == SOCKET_ERROR) {
		TCHAR* szError = _T("ソケットイベント通知設定に失敗しました。<RTTPlugin::InitializeController>");
		//ReportError(szError);
		LogDebugMessage(Log_Error, szError);
	}

	return socketHandler;
}

void UnInitializeController(SOCKET socketHandler)
{
	closesocket(socketHandler);
}

//SOCKET InitializeRTTOriginalController(HWND hWnd, USHORT uPort)
//{
//	SOCKET socketHandler;
//	SOCKADDR_IN address;
//	TCHAR szError[BUFFER_SIZE];
//	int nResult = 0;
//
//	socketHandler = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (socketHandler == INVALID_SOCKET) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
//		LogDebugMessage(Log_Error, szError);
//		return INVALID_SOCKET;
//	}
//
//	address.sin_family = AF_INET;
//	address.sin_port = htons(uPort);
//	address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
//
//	nResult = connect(socketHandler, (const sockaddr*)&address, sizeof(address));
//	if (nResult == SOCKET_ERROR) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] connect() : %d"), WSAGetLastError());
//		LogDebugMessage(Log_Error, szError);
//		closesocket(socketHandler);
//		return INVALID_SOCKET;
//	}
//
//	if (WSAAsyncSelect(socketHandler, hWnd, MY_WINSOCKSELECT_RTT, FD_READ) == SOCKET_ERROR) {
//		TCHAR* szError = _T("ソケットイベント通知設定に失敗しました。<RTTPlugin::InitializeController>");
//		//ReportError(szError);
//		LogDebugMessage(Log_Error, szError);
//	}
//
//	return socketHandler;
//}

int AnalyzeMessage(I4C3DUDPPacket* pPacket, LPSTR szCommand, SIZE_T size, double* pDeltaX, double* pDeltaY, char cTermination)
{
	static char szFormat[BUFFER_SIZE] = {0};
	int scanCount = 0;
	double deltaX = 0., deltaY = 0.;

	if (szFormat[0] == '\0') {
		sprintf_s(szFormat, sizeof(szFormat), "%%s %%lf %%lf%c", cTermination);
	}
	
	scanCount = sscanf_s(pPacket->szCommand, szFormat, szCommand, size, &deltaX, &deltaY);

	if (3 <= scanCount) {
		*pDeltaX = deltaX;
		*pDeltaY = deltaY;
	}
	return scanCount;
}
