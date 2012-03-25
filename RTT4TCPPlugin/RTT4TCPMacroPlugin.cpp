#include "StdAfx.h"
#include "RTT4TCPMacroPlugin.h"
#include "I4C3DCommon.h"
#include "Miscellaneous.h"
#include <map>
#include <string>

using namespace std;

namespace {
	const int BUFFER_SIZE = 256;

	//typedef struct MacroKey {
	//	UINT uKeyCode;		// このマクロキーが表すキーコード
	//	struct MacroKey* next;
	//} MacroKey;

	//map<string, MacroKey*> g_macroTable;
	map<string, string> g_macroTable;
};

RTT4TCPMacroPlugin::RTT4TCPMacroPlugin(void)
{
	g_macroTable.insert(map<string, string>::value_type("MACRO1", "ANIM Animation_group_01_Init START"));
	g_macroTable.insert(map<string, string>::value_type("MACRO2", "ANIM Animation_group_02_Init START"));
}


RTT4TCPMacroPlugin::~RTT4TCPMacroPlugin(void)
{
	//map<string, MacroKey*>::iterator it = g_macroTable.begin();
	//MacroKey *tmp, *next = NULL;
	//for (; it != g_macroTable.end(); ++it) {
	//	tmp = it->second;
	//	while (tmp != NULL) {
	//		next = tmp->next;
	//		delete tmp;
	//		tmp = next;
	//	}
	//}
	g_macroTable.clear();
}

BOOL RTT4TCPMacroPlugin::RegisterMacro(LPCSTR szBuffer, char* termination)
{
	//char tmpCommand[BUFFER_SIZE] = {0};
	//char macroName[BUFFER_SIZE] = {0};
	//char macroValue[BUFFER_SIZE] = {0};

	//sscanf_s(szBuffer, g_registerMacroFormat, tmpCommand, sizeof(tmpCommand), macroName, sizeof(macroName), macroValue, sizeof(macroValue), termination, sizeof(*termination));
	//if (!AnalyzeMacro(macroName, macroValue)) {
	//	char szError[BUFFER_SIZE];
	//	sprintf_s(szError, _countof(szError), "キーマクロは登録されませんでした: [マクロ名: %s] [内容: %s]", macroName, macroValue);
	//	LogDebugMessageA(Log_Error, szError);
	//	return FALSE;
	//}


	return TRUE;
}

void RTT4TCPMacroPlugin::GetMacroValue(LPCSTR macroName, LPSTR macroValue, int length)
{
	map<string, string>::iterator it = g_macroTable.find(string(macroName));
	if (it != g_macroTable.end()) {
		strcpy_s(macroValue, length, it->second.c_str());
	}
}
