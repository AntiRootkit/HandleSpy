// HandleSpy.cpp : main source file for HandleSpy.exe
//

#include "stdafx.h"

#include "DetectDlg.h"
#include "aboutdlg.h"
#include "MainFrm.h"
#include "Inject.h"

#include "SymbolHandler.h"

TCHAR* g_SingleInstacneMutexName = _T("HandleSpy_{2F1B687C-A7B8-45ba-AFC4-32900FA16559}_SingleInstanceMutexName");

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	// ����������
	HANDLE hMutex = ::CreateMutex(NULL, FALSE, g_SingleInstacneMutexName);
	// ���������
	if (::GetLastError() == ERROR_ALREADY_EXISTS) 
	{
		AtlMessageBox(
			NULL, 
			_T("ֻ����һ��HandleSpyʵ�����У�"), 
			_T("HandleSpy"));
		::CloseHandle(hMutex);
		 hMutex = NULL;
		 return FALSE;
	}

	if (FALSE == EnableDebugPrivilege())
	{
		AtlMessageBox(
			NULL, 
			_T("����Ȩ�޻�ȡʧ�ܣ��������̿����޷�����ʹ�øù��ߣ�"), 
			_T("��ʾ"));
	}

	//��ʼ��GDI+
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


	//��ʼ�����Ź�����
	CONST TCHAR* lpConfigFileName = _T("Config.ini");
	TCHAR szExePath[_MAX_PATH];
	::ZeroMemory(szExePath, sizeof(szExePath));
	::GetModuleFileName(NULL, szExePath, _countof(szExePath));
	LPTSTR lpName = ::PathFindFileName(szExePath);
	_tcscpy_s(lpName, _tcslen(lpConfigFileName)+1, lpConfigFileName);
	CSymbolHandler::GetInstance()->Init(szExePath);
	
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	GdiplusShutdown(gdiplusToken);

	return nRet;
}
