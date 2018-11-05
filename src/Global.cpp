#include "stdafx.h"
#include "Global.h"




int _GetFileSize(LPCTSTR sPathName)
{
	HANDLE	hFile;
	hFile = ::CreateFile(sPathName, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return -1;
	const int nSize = ::GetFileSize(hFile, NULL);
	CloseHandle(hFile);
	return nSize;
}


