#pragma once

class RTT4TCPMacroPlugin
{
public:
	RTT4TCPMacroPlugin(void);
	virtual ~RTT4TCPMacroPlugin(void);

	virtual BOOL RegisterMacro(LPCSTR szBuffer, char* termination);
	virtual void GetMacroValue(LPCSTR macroName, LPSTR macroValue, int length);

private:
	//virtual BOOL AnalyzeMacro(LPCSTR macroName, LPCSTR macroValue);
};

