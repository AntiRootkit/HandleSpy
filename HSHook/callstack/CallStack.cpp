#include "CallStack.h"

extern const std::wstring GetModuleIndexString(void *pModBase, BOOL bPDB);

typedef struct __FRAME
{
	struct __FRAME * pOutFrame;
	DWORD dwret;
}FRAME, *PFRAME;

std::vector<MOD_INFO> CCallStack::s_vecModInfo;

/*
* ��ȡPEB�е�ģ���б�����ĳһ����ַ������ģ��
*/
BOOL CCallStack::GetModuleInfoFromAddr(DWORD addr, RAW_MOD_INFO& modinfo)
{
	PPEB pPeb = NULL;

	/* ��ȡPEB��ַ*/
	__asm
	{
		push eax
		mov eax, fs:[0x30]
		mov pPeb, eax
		pop eax
	} 

	/* ��PEB�л�ȡPEB_LDR_DATA�ṹ��ַ */
	PPEB_LDR_DATA pLdr = pPeb->Ldr;

	/* ��PEB_LDR_DATA�ṹ�л�ȡInLoadOrderModuleList.Flink
	* �ýṹΪLIST_ENTRY
	* ѡȡ��LIST��InLoadOrderModuleList
	*/
	PLIST_ENTRY pListEntryOfInLoadOrderModuleList = pLdr->InLoadOrderModuleList.Flink;

	PLIST_ENTRY pCur = pListEntryOfInLoadOrderModuleList;

	PLDR_MODULE pLdrMod = NULL;

	BOOL bFound = FALSE;

	/* ǰ�����ģ���б� */
	do 
	{
		/* ��LIST_ENTRY�ṹ���뵽LDR_MODULE�ṹ */
		pLdrMod = CONTAINING_RECORD(pCur, LDR_MODULE, InLoadOrderModuleList);

		/* �жϸõ�ַ�Ƿ��ڵ�ǰģ�� */
		if (addr > (DWORD)(pLdrMod->BaseAddress) && 
			addr < ((DWORD)(pLdrMod->BaseAddress) + pLdrMod->SizeOfImage))
		{
			/* �ҵ�Ŀ��ģ�飬ȡֵ������ѭ��*/
			modinfo.dwModuleBase = (DWORD)pLdrMod->BaseAddress;
			modinfo.dwImageSize = (DWORD)pLdrMod->SizeOfImage;
			modinfo.dwTimeStamp = (DWORD)pLdrMod->TimeDateStamp;
			modinfo.pModPath = pLdrMod->FullDllName.Buffer;

			bFound = TRUE;
			break;
		}

		/* ����������һ��LSIT_ENTRY */
		pCur = pCur->Flink;
	} while (pCur != pListEntryOfInLoadOrderModuleList);

	return bFound;
}


/*
* ��ȡ��ǰ�������õ���������ö�ջ
*
*/
DWORD CCallStack::GetCurrentCallStack(const PCALL_STACK pcs, DWORD dwMaxFrameCount)
{
	DWORD dwBytesWritten = 0;
	
	if (NULL == pcs)
	{
		return 0;
	}
	ZeroMemory(pcs, sizeof(CALL_STACK));

	PTEB pTeb = NULL;

	/* ��ȡTEB��ַ*/
	__asm
	{
		push eax
		mov eax, fs:[0x18]
		mov pTeb, eax
		pop eax
	}

	DWORD dwStackBase = (DWORD)(pTeb->NtTib.StackBase);
	DWORD dwStackLimit = (DWORD)(pTeb->NtTib.StackLimit);

 	DWORD dwEbp = 0;
	PFRAME pFrame = NULL;

	_asm mov dwEbp, ebp;

	DWORD dwFrameCount = 0;
	PSTACK_FRAME pStackFrame = pcs->frame;

	pFrame = (PFRAME)dwEbp;

	do 
	{
		/*
		 * �����ж��õ���pFrame��ֵ�Ƿ���ջ�ڣ�����ͨ��ջ�����߽����ж�
		 * pFrame��ֵһ��Ҫ����ջ����͵�ַ����С��ջ�ף���ߵ�ַ����ȥһ��DWORD�ĵ�ַ
		 * ��Ϊ���pFrame��ֵ����ջ����ߵ�ַ��ȥһ��DWORD�ĵ�ַʱȥ����pFrame->dwRet��Ȼ����av�쳣
		 */
		if ((DWORD)pFrame > (dwStackBase - sizeof(DWORD)) || (DWORD)pFrame < dwStackLimit)
		{
			break;
		}

		if (IsBadReadPtr((PVOID)(pFrame->pOutFrame), sizeof(DWORD)) || NULL == pFrame->pOutFrame ||
			IsBadReadPtr((PVOID)(pFrame->pOutFrame->pOutFrame), sizeof(DWORD)) || NULL == pFrame->pOutFrame->pOutFrame ||
			IsBadReadPtr((PVOID)(pFrame->pOutFrame->dwret), sizeof(DWORD)) || NULL == pFrame->pOutFrame->dwret)
		{
			break;
		}

		pFrame = pFrame->pOutFrame;
		
		//if (IsBadReadPtr((PVOID)(pFrame), sizeof(DWORD)) || NULL == pFrame || 
		//	IsBadReadPtr((PVOID)(pFrame->dwret), sizeof(DWORD)) || NULL == pFrame->dwret)
		//{
		//	break;
		//}
		
		if (dwFrameCount >= dwMaxFrameCount)
		{
			break;
		}

		pStackFrame->dwAddr = pFrame->dwret;	/* ��¼���ص�ַ */
		pStackFrame->iIndex = -1;				/* ��¼ģ����� */

		dwFrameCount += 1;
		dwBytesWritten += sizeof(STACK_FRAME);
		
		RAW_MOD_INFO rawmi;
		if (TRUE == GetModuleInfoFromAddr(pStackFrame->dwAddr, rawmi))
		{
			/* �����Ƿ��Ѿ���¼��ľ�� */
			for (int i=0; i<(int)s_vecModInfo.size(); i++)
			{
				if (rawmi.dwModuleBase == s_vecModInfo[i].dwModuleBase && 
					rawmi.dwTimeStamp == s_vecModInfo[i].dwTimeStamp)
				{
					pStackFrame->iIndex = i;
					break;
				}
			}

			/* û�м�¼��ģ�� */
			if (-1 == pStackFrame->iIndex)
			{
				if (s_vecModInfo.size() < MAX_MODULE_COUNT)
				{
					/* ���û�г������ģ���¼������¼��ģ�� */
					MOD_INFO modinfo;
					modinfo.dwModuleBase = rawmi.dwModuleBase;
					modinfo.dwImageSize = rawmi.dwImageSize;
					modinfo.dwTimeStamp = rawmi.dwTimeStamp;
					std::wstring wstrPdbSig = GetModuleIndexString((void*)(modinfo.dwModuleBase), TRUE);

					wcscpy_s(modinfo.wcszPdbSig, _countof(modinfo.wcszPdbSig), wstrPdbSig.c_str());

					if (NULL != rawmi.pModPath)
					{
						wcscpy_s(modinfo.wcszModPath, _countof(modinfo.wcszModPath), rawmi.pModPath);
					}
					else
					{
						wcscpy_s(modinfo.wcszModPath, _countof(modinfo.wcszModPath), L"UnknowModule");
					}

					pStackFrame->iIndex = s_vecModInfo.size();
					s_vecModInfo.push_back(modinfo);
				}
			}
		}

		pStackFrame += 1;

	} while (true);

	if (dwFrameCount > 0)
	{	
		pcs->nFrameCount = dwFrameCount;
		pcs->dwTimeStamp = GetTimeStamp();
		pcs->dwReserve = 0xabababab;
		dwBytesWritten = dwBytesWritten + sizeof(CALL_STACK) - sizeof(STACK_FRAME);
	}
	
	return dwBytesWritten;
}


DWORD CCallStack::GetTimeStamp()
{
	DWORD dwTimeStamp = 0;
	UINT uPeriod = 1;
	timeBeginPeriod(uPeriod);
	dwTimeStamp = timeGetTime();
	timeEndPeriod(uPeriod);

	return dwTimeStamp;
}

std::vector<MOD_INFO>& CCallStack::GetModInfoVector()
{
	return s_vecModInfo;
}

void CCallStack::ClearModInfo()
{
	s_vecModInfo.clear();
}


//int GetCallStack(void *_ebp, void *buf, int nMaxLevel)
//{
//	unsigned char *pbBuf = (unsigned char *)buf;
//	DWORD dwStackHigh, dwStackLow;
//	__asm
//	{
//		mov edx,fs:[4];
//		mov dwStackHigh,edx;
//		mov edx,fs:[8];
//		mov dwStackLow,edx;
//	}		
//
//	PDWORD p_frame = (PDWORD)_ebp;
//	int i = 0;
//	while (i < nMaxLevel)
//	{
//		if (p_frame > (DWORD*)dwStackHigh || p_frame < (DWORD*)dwStackLow)
//			break;	
//		DWORD ret = p_frame[1];
//		if (ret == 0)
//			break;
//
//		BYTE index;
//		if (!GetModuleInfoByAddr((void*)ret, &index))
//			index = MAX_MODULE_COUNT - 1;//unknown module!
//
//		*(DWORD*)pbBuf = ret;
//		pbBuf += 4;
//		*(BYTE*)pbBuf = index;
//		pbBuf += 1;
//		i++;
//
//		PDWORD p_prevFrame = p_frame;
//		p_frame = (PDWORD)p_frame[0];
//
//		if (p_frame > (DWORD*)dwStackHigh || p_frame < (DWORD*)dwStackLow)
//		{
//			p_prevFrame += 7;
//			p_frame =  (PDWORD)p_prevFrame[0];
//			if (p_frame > (DWORD*)dwStackHigh || p_frame < (DWORD*)dwStackLow)
//			{
//				break;
//			}
//		}
//
//		if ((DWORD)p_frame & 3)    // Frame pointer must be aligned on a
//			break;                  // DWORD boundary.  Bail if not so.
//
//		if (p_frame <= p_prevFrame)
//			break;
//	}
//	return i;
//}