#include "stdafx.h"
#include "ProcessSelDlg.h"
#include <TlHelp32.h>
#include <Psapi.h>

BOOL CProcessSelDlg::s_Order[2] = {FALSE, FALSE};
INT CProcessSelDlg::s_SortColumn = 0;

CProcessSelDlg::CProcessSelDlg(void)
{
	m_pdwPidReceiver = NULL;
}

CProcessSelDlg::~CProcessSelDlg(void)
{

}

BOOL CProcessSelDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_pdwPidReceiver = (PDWORD)lParam;
	if (NULL == m_pdwPidReceiver)
	{
		return FALSE;
	}

	m_listProcess = GetDlgItem(IDC_PROCESSLIST);

	int nColIndex = 0;

	m_listProcess.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
	nColIndex = m_listProcess.InsertColumn(0, _T("����ID"), 0, 50);
	nColIndex = m_listProcess.InsertColumn(1, _T("������"), 0, 200);	

	ReFreshProcessList();

	CenterWindow();
	
	return TRUE;
}

LRESULT CProcessSelDlg::OnBnClickedOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	GetSelectedProcessID();

	return 0;
}

LRESULT CProcessSelDlg::OnNmDblClickProcesslist(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	GetSelectedProcessID();

	return 0;
}

LRESULT CProcessSelDlg::OnHdnItemClickProcesslist(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	LPNMHEADER lpHdader = (LPNMHEADER)pNMHDR;

	s_SortColumn = lpHdader->iItem;
	
	s_Order[s_SortColumn] = !s_Order[s_SortColumn];
	
	m_listProcess.SortItems(&CProcessSelDlg::CompareFunc, (LPARAM)&m_listProcess);

	return 0;
}

LRESULT CProcessSelDlg::OnBnClickedRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ReFreshProcessList();

	return 0;
}

LRESULT CProcessSelDlg::OnBnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(IDCANCEL);

	return 0;
}

void CProcessSelDlg::GetSelectedProcessID()
{
	int nIndex = m_listProcess.GetSelectedIndex();

	if (nIndex < 0)
	{
		AtlMessageBox(m_hWnd, _T("��ѡ��Ŀ�����!"), _T("��ʾ"));
		return ;
	}

	if (NULL != m_pdwPidReceiver)
	{
		*m_pdwPidReceiver = m_listProcess.GetItemData(nIndex);
	}

	EndDialog(IDOK);

	return ;
}

void CProcessSelDlg::ReFreshProcessList()
{
	m_listProcess.DeleteAllItems();

	DWORD dwIndex = 0;
	HANDLE hSnapShot = NULL;
	PROCESSENTRY32 pe32;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	hSnapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapShot == INVALID_HANDLE_VALUE)
	{
		return;
	}

	TCHAR wcsPid[8];
	::Process32First(hSnapShot, &pe32);
	do
	{
		if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == GetCurrentProcessId())
		{
			continue;
		}
		ZeroMemory(wcsPid, sizeof(wcsPid));
		_ultot_s(pe32.th32ProcessID, wcsPid, 10);
		dwIndex = m_listProcess.InsertItem(0, wcsPid, 1);
		m_listProcess.SetItemText(dwIndex, 1, pe32.szExeFile);
		m_listProcess.SetItemData(dwIndex, pe32.th32ProcessID);
	}while (::Process32Next(hSnapShot, &pe32));

	::CloseHandle(hSnapShot);

	s_Order[0] = TRUE;
	s_Order[1] = TRUE;
	s_SortColumn = 1;
	m_listProcess.SortItems(&CProcessSelDlg::CompareFunc, (LPARAM)&m_listProcess);

	return;
}

int CALLBACK CProcessSelDlg::CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CListViewCtrl* pList = (CListViewCtrl*)lParamSort;

	//���ָ���е�������
	LVFINDINFO findInfo; 
	findInfo.flags = LVFI_PARAM;

	findInfo.lParam = lParam1;
	int iItem1 = pList->FindItem(&findInfo, -1); 

	findInfo.lParam = lParam2; 
	int iItem2 = pList->FindItem(&findInfo, -1); 

	//����ָ���е�������,�õ������������
	CString strText1;
	CString strText2;

	pList->GetItemText(iItem1, s_SortColumn, strText1); 
	pList->GetItemText(iItem2, s_SortColumn, strText2);

	strText1 = strText1.MakeLower().Trim();
	strText2 = strText2.MakeLower().Trim();

	//�Ƚ���ֵ
	BOOL bRet = FALSE;

	if (s_SortColumn == 0)
	{
		bRet = _wtoi(strText1.GetString()) > _wtoi(strText2.GetString());
	}

	//�Ƚ��ַ���
	if (s_SortColumn == 1)
	{
		bRet = strText1.Compare(strText2) > 0;
	}

	if (bRet)
	{
		return s_Order[s_SortColumn] ? 1 : -1;
	}
	else
	{
		return s_Order[s_SortColumn] ? -1 : 1;
	}
}