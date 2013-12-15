#ifndef __HANDLEAPIS_H__
#define __HANDLEAPIS_H__
#include "Detoursor.h"

namespace DetoursorHelper
{
	/*
	* Helper function of CDetoursor
	* Add many functions to CDetoursor
	*/
	BOOL AddAllFunctionsToDetoursor(CDetoursor *pDetoursor);

	VOID InitResourceForFakeFunction();

	VOID CleanResourceForFakeFunction();
}
#endif