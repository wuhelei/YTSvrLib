/*MIT License

Copyright (c) 2016 Archer Xu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "stdafx.h"
#include "GlobalServer.h"
#ifdef LIB_WINDOWS
#include <shlwapi.h>
#else
#include <execinfo.h>
#include <signal.h>
#endif // LIB_WINDOWS


static size_t convert(const char *tocode, const char *fromcode, const char *inbufp, size_t inbytesleft, char *outbufp, size_t outbytesleft)
{
	iconv_t ic;
	size_t outbytes = outbytesleft;
	size_t ret;

	ic = iconv_open(tocode, fromcode);
	if (ic == ((iconv_t) -1))
	{
		return 0;
	}

	while (inbytesleft > 0)
	{
#ifdef LIB_WINDOWS
		ret = iconv(ic, &inbufp, &inbytesleft, &outbufp, &outbytes);
#else
		char* pinbuffer = new char[inbytesleft+1];
		ZeroMemory(pinbuffer,inbytesleft+1);
		memcpy(pinbuffer,inbufp,inbytesleft);
		char* pinbufferbegin = pinbuffer;
		ret = iconv(ic, &pinbuffer, &inbytesleft, &outbufp, &outbytes);
		delete[] pinbufferbegin;
#endif // LIB_WINDOWS
		if (ret == -1)
		{
			return 0;
		}
	}

	ret = iconv_close(ic);
	if (ret == -1)
	{
		return 0;
	}

	return (outbytesleft - outbytes);
}

// UTF8字符串转换为UNICODE字符串
int utf8tounicode(const char *utf8_buf, wchar_t *unicode_buf, int max_size)
{
	ZeroMemory(unicode_buf, max_size*sizeof(wchar_t));

#ifdef LIB_WINDOWS
	return (int) convert("UCS-2LE", "UTF-8", utf8_buf, strlen(utf8_buf), (char*)unicode_buf, max_size*sizeof(wchar_t));
#else
	return (int) convert("WCHAR_T", "UTF-8", utf8_buf, strlen(utf8_buf), (char*) unicode_buf, max_size*sizeof(wchar_t));
#endif // LIB_WINDOWS
}

// UNICODE字符串转换为UTF8字符串
int unicodetoutf8(const wchar_t* unicode_buf, char* utf8_buf, int max_size)
{
	int nSrcLen = (int) (wcslen(unicode_buf) + 1);

	ZeroMemory(utf8_buf, max_size);

#ifdef LIB_WINDOWS
	return (int) convert("UTF-8", "UCS-2LE", (const char*) unicode_buf, nSrcLen * sizeof(wchar_t), utf8_buf, max_size);
#else
	return (int) convert("UTF-8", "WCHAR_T", (const char*) unicode_buf, nSrcLen * sizeof(wchar_t), utf8_buf, max_size);
#endif // LIB_WINDOWS
}

// 获取当前进程所在路径
void GetModuleFilePath(char* pszOut, int nLen)
{
	char szDir[MAX_PATH] = { 0 };
#ifdef LIB_WINDOWS
	GetModuleFileNameA(NULL, szDir, sizeof(szDir));
	char* pEnd = strrchr(szDir, '\\');
	if (pEnd)
		*pEnd = '\0';

	strncpy_s(pszOut, nLen ,szDir, nLen);
#else
	readlink("/proc/self/exe", szDir, MAX_PATH);
	char* pEnd = strrchr(szDir, '/');
	if (pEnd)
		*pEnd = '\0';

	strncpy_s(pszOut, szDir, nLen);
#endif // LIB_WINDOWS
}

// 获取当前进程名称
void GetModuleFileName(char* pszOut, int nLen)
{
	char szDir[MAX_PATH] = { 0 };

#ifdef LIB_WINDOWS
	GetModuleFileNameA(NULL, szDir, sizeof(szDir));
	char* pEnd = strrchr(szDir, '\\');

	strncpy_s(pszOut, nLen, (pEnd+1) , nLen);
#else
	readlink("/proc/self/exe", szDir, MAX_PATH);
	char* pEnd = strrchr(szDir, '/');

	strncpy_s(pszOut, (pEnd+1) , nLen);
#endif // LIB_WINDOWS
}

__time32_t GetNextWeekDayTime(__time32_t tNow ,WORD wWeekDay, WORD wHour)
{
	tm sTimeNow;
	ZeroMemory(&sTimeNow,sizeof(sTimeNow));

#ifdef LIB_WINDOWS
	_localtime32_s(&sTimeNow, &tNow);
#else
	localtime_r(&tNow, &sTimeNow);
#endif // LIB_WINDOWS

	int nDaySpan = 0;

	if (sTimeNow.tm_wday != wWeekDay)
	{//如果今天不是开始日期的话,算出相差多少天
		if (wWeekDay > sTimeNow.tm_wday)
		{
			nDaySpan = wWeekDay - sTimeNow.tm_wday;
		}
		else
		{
			nDaySpan = ((7 - sTimeNow.tm_wday) + wWeekDay);
		}
	}
	else
	{
		if (sTimeNow.tm_hour > wHour)
		{//如果刚好是这天,但是时间比开始时间大,则下次开始时间刚好是7天后了
			nDaySpan = 7;
		}
	}

	tm sTime;
	sTime.tm_year = sTimeNow.tm_year;
	sTime.tm_mon = sTimeNow.tm_mon;
	sTime.tm_mday = sTimeNow.tm_mday + nDaySpan;
	sTime.tm_hour = wHour;
	sTime.tm_min = 0;
	sTime.tm_sec = 0;

	__time32_t tNextWeekZero = _mktime32(&sTime);

	return tNextWeekZero;
}

const wchar_t* CovertUTC2String(__time32_t tTime, wchar_t* pwzOut, int nOutMaxLen, const wchar_t* pwzQuote /*= L'\''*/)
{
	ZeroMemory(pwzOut, nOutMaxLen);

	if (tTime <= 0)
	{// 不能小于等于0
		__wcsncpy_s(pwzOut, L"NULL", nOutMaxLen - 1);

		return pwzOut;
	}

	tm timenow;
#ifdef LIB_WINDOWS
	_localtime32_s(&timenow, &tTime);
#else
	localtime_r(&tTime, &timenow);
#endif // LIB_WINDOWS
	__snwprintf_s(pwzOut, nOutMaxLen, L"%s%04d-%02d-%02d %02d:%02d:%02d%s",
		pwzQuote, (timenow.tm_year + 1900), (timenow.tm_mon + 1), timenow.tm_mday, timenow.tm_hour, timenow.tm_min, timenow.tm_sec, pwzQuote);

	return pwzOut;
}

const char* CovertUTC2String(__time32_t tTime, char* pszOut, int nOutMaxLen, const char* pszQuote /*= '\''*/)
{
	ZeroMemory(pszOut, nOutMaxLen);

	if (tTime <= 0)
	{// 不能小于等于0
		__strncpy_s(pszOut, "NULL", nOutMaxLen - 1);

		return pszOut;
	}

	tm timenow;
#ifdef LIB_WINDOWS
	_localtime32_s(&timenow,&tTime);
#else
	localtime_r(&tTime, &timenow);
#endif // LIB_WINDOWS
	__snprintf_s(pszOut, nOutMaxLen, "%s%04d-%02d-%02d %02d:%02d:%02d%s",
		pszQuote, (timenow.tm_year + 1900), (timenow.tm_mon + 1), timenow.tm_mday, timenow.tm_hour, timenow.tm_min, timenow.tm_sec, pszQuote);

	return pszOut;
}

LPCSTR MakeRandomKey(LPSTR pszOut, UINT nOutMaxLen, UINT nNeedLen)
{
	if (nNeedLen >= nOutMaxLen)
	{
		return "";
	}

	const static char charset[] = { '_', '*', '^', '#', '&', '@', '$',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	const int charset_size = _countof(charset);

	for (UINT i = 0; i < nNeedLen; ++i)
	{
		int rand_charset = Random2(charset_size);//生成0-61之间的随机数
		pszOut[i] = charset[rand_charset];
	}

	return pszOut;
}

LPCWSTR MakeRandomKey(LPWSTR pszOut, UINT nOutMaxLen, UINT nNeedLen)
{
	if (nNeedLen >= nOutMaxLen)
	{
		return L"";
	}

	const static wchar_t charset[] = { L'_', L'*', L'^', L'#', L'&', L'@', L'$',
		L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h', L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p', L'q', L'r', L's', L't', L'u', L'v', L'w', L'x', L'y', L'z',
		L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H', L'I', L'J', L'K', L'L', L'M', L'N', L'O', L'P', L'Q', L'R', L'S', L'T', L'U', L'V', L'W', L'X', L'Y', L'Z',
		L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9'};

	const int charset_size = _countof(charset);

	for (UINT i = 0; i < nNeedLen; ++i)
	{
		int rand_charset = Random2(charset_size);//生成0-61之间的随机数
		pszOut[i] = charset[rand_charset];
	}

	return pszOut;
}

void AddSlashes(const wchar_t* pwzIn, wchar_t* pwzOut, int nLen)
{
	size_t nOldLen = wcslen(pwzIn);

	if ((int) nOldLen > nLen)
	{
		LOG("Error : Escape failed.out of memory.");
		return;
	}

	wchar_t* pIn = (wchar_t*) pwzIn;
	wchar_t* pCur = pwzOut;
	int nCur = 0;

	while (pIn != NULL && (*pIn) != L'\0')
	{
		if ((*pIn) == L'\'' || (*pIn) == L'\"')
		{
			(*pCur) = L'\\';
			pCur++; nCur++;
			(*pCur) = (*pIn);
			pCur++; nCur++;
		}
		else
		{
			(*pCur) = (*pIn);
			pCur++; nCur++;
		}

		if (nCur >= (nLen - 2))
		{
			break;
		}

		pIn++;
	}

	(*pCur) = L'\0';
}

void AddSlashes(const char* pszIn, char* pszOut, int nLen)
{
	size_t nOldLen = strlen(pszIn);

	if ((int) nOldLen > nLen)
	{
		LOG("Error : Escape failed.out of memory.");
		return;
	}

	char* pIn = (char*) pszIn;
	char* pCur = pszOut;
	int nCur = 0;

	while (pIn != NULL && (*pIn) != '\0')
	{
		if ((*pIn) == '\'' || (*pIn) == '\"')
		{
			(*pCur) = '\\';
			pCur++; nCur++;
			(*pCur) = (*pIn);
			pCur++; nCur++;
	}
		else
		{
			(*pCur) = (*pIn);
			pCur++; nCur++;
		}

		if (nCur >= (nLen - 2))
		{
			break;
		}

		pIn++;
}

	(*pCur) = '\0';
}

UINT GetDayFromTicks(__time32_t tTime,__time32_t tTimezone)
{
	return (UINT) ((DOUBLE) (tTime + tTimezone) / (DOUBLE) (SEC_DAY));
}

void WINAPI ParseListStr(LPCWSTR pwzList, std::vector< std::vector<int> >& vctOut)
{
	//格式为：道具1ID|数量^道具2ID|数量
	vctOut.clear();
	std::vector<std::wstring> vctKeys, vctPair;
	StrDelimiter( pwzList, L"^", vctKeys );
	vctOut.resize( vctKeys.size() );
	for( UINT i=0; i<vctKeys.size(); i++ )
	{
		vctPair.clear();
		StrDelimiter( vctKeys[i].c_str(), L"|", vctPair );
		vctOut[i].resize( vctPair.size() );
		for( UINT j=0;j<vctPair.size(); j++ )
			vctOut[i][j] = _wtol( vctPair[j].c_str() );
	}
}
void WINAPI ParseListStr(LPCSTR pszList, std::vector< std::vector<int> >& vctOut)
{
	//格式为：道具1ID|数量^道具2ID|数量
	vctOut.clear();
	std::vector<std::string> vctKeys, vctPair;
	StrDelimiter(pszList, "^", vctKeys);
	vctOut.resize(vctKeys.size());
	for (UINT i = 0; i < vctKeys.size(); i++)
	{
		vctPair.clear();
		StrDelimiter(vctKeys[i].c_str(), "|", vctPair);
		vctOut[i].resize(vctPair.size());
		for (UINT j = 0; j < vctPair.size(); j++)
			vctOut[i][j] = atol(vctPair[j].c_str());
	}
}

int StringToLowcase(LPCWSTR lpwzSrc, LPWSTR lpwzOut, int nLen)
{
	static const int nLowcase = L'a'-L'A';

	ZeroMemory(lpwzOut,nLen*sizeof(WCHAR));

	WCHAR* pSrc = (WCHAR*)lpwzSrc;
	WCHAR* pOut = (WCHAR*)lpwzOut;
	int nRealLen = 0;

	while ((*pSrc) != L'\0' && nRealLen < nLen)
	{
		if ((*pSrc) >= L'A' && (*pSrc) <= L'Z')
		{
			(*pOut) = (*pSrc) + nLowcase; 
}
		else
		{
			(*pOut) = (*pSrc);
		}

		++pSrc;
		++pOut;
		nRealLen++;
	}

	return nRealLen;
}

int StringToLowcase(LPCSTR lpszSrc, LPSTR lpszOut, int nLen)
{
	static const int nLowcase = 'a' - 'A';

	ZeroMemory(lpszOut, nLen*sizeof(CHAR));

	CHAR* pSrc = (CHAR*) lpszSrc;
	CHAR* pOut = (CHAR*) lpszOut;
	int nRealLen = 0;

	while ((*pSrc) != '\0' && nRealLen < nLen)
	{
		if ((*pSrc) >= 'A' && (*pSrc) <= 'Z')
		{
			(*pOut) = (*pSrc) + nLowcase; 
		}
		else
		{
			(*pOut) = (*pSrc);
		}

		++pSrc;
		++pOut;
		nRealLen++;
}

	return nRealLen;
}


__time32_t MakeStrTimeToUTC(LPCWSTR lpwzTime)
{
	vector<int> vctList;
	StrDelimiter(lpwzTime,L"|",vctList);
	if (vctList.size() < 6)
	{
		return 0;
	}

	tm sTime;
	sTime.tm_year = vctList[0]-1900;
	sTime.tm_mon = vctList[1]-1;
	sTime.tm_mday = vctList[2];
	sTime.tm_hour = vctList[3];
	sTime.tm_min = vctList[4];
	sTime.tm_sec = vctList[5];

	__time32_t tTime = _mktime32(&sTime);

	return tTime;
}

__time32_t MakeStrTimeToUTC(LPCSTR lpwzTime)
{
	vector<int> vctList;
	StrDelimiter(lpwzTime, "|", vctList);
	if (vctList.size() < 6)
	{
		return 0;
	}

	tm sTime;
	sTime.tm_year = vctList[0] - 1900;
	sTime.tm_mon = vctList[1] - 1;
	sTime.tm_mday = vctList[2];
	sTime.tm_hour = vctList[3];
	sTime.tm_min = vctList[4];
	sTime.tm_sec = vctList[5];

	__time32_t tTime = _mktime32(&sTime);

	return tTime;
}

__time32_t MakeStrTimeToUTC_NoYear(LPCWSTR lpwzTime, UINT nYear)
{
	vector<int> vctList;
	StrDelimiter(lpwzTime, L"|", vctList);
	if (vctList.size() < 5)
	{
		return 0;
	}

	tm sTime;
	sTime.tm_year = nYear;
	sTime.tm_mon = vctList[0] - 1;
	sTime.tm_mday = vctList[1];
	sTime.tm_hour = vctList[2];
	sTime.tm_min = vctList[3];
	sTime.tm_sec = vctList[4];

	__time32_t tTime = _mktime32(&sTime);

	return tTime;
}

__time32_t MakeStrTimeToUTC_NoYear(LPCSTR lpwzTime, UINT nYear)
{
	vector<int> vctList;
	StrDelimiter(lpwzTime, "|", vctList);
	if (vctList.size() < 5)
	{
		return 0;
	}

	tm sTime;
	sTime.tm_year = nYear;
	sTime.tm_mon = vctList[0] - 1;
	sTime.tm_mday = vctList[1];
	sTime.tm_hour = vctList[2];
	sTime.tm_min = vctList[3];
	sTime.tm_sec = vctList[4];

	__time32_t tTime = _mktime32(&sTime);

	return tTime;
}

void GetLocalIP(vector<string>& vctIPList)
{
	char szHostname[255] = { 0 };
	//获取本地主机名称
	if (gethostname(szHostname, sizeof(szHostname)) == SOCKET_ERROR)
	{
		LOG("Error in GetHostName : %d", GetLastError());
		return;
	}

	//从主机名数据库中得到对应的“主机”
	struct hostent *pHost = gethostbyname(szHostname);
	if (pHost == NULL)
	{
		LOG("Error in gethostbyname : %d", GetLastError());
		return;
	}

	//循环得出本地机器所有IP地址
	int i = 0;
	while (i < pHost->h_length && pHost->h_addr_list[i] != NULL)
	{
		struct in_addr addr;
		memcpy(&addr, pHost->h_addr_list[i], sizeof(struct in_addr));

		vctIPList.push_back(string(inet_ntoa(addr)));

		i++;
	}
}

void RemoveSpace(LPCWSTR pwzSrc, LPWSTR pwzDst, int nLen)
{
	ZeroMemory(pwzDst,nLen);

	WCHAR* pwCur = (WCHAR*)pwzSrc;

	WCHAR* pwDstCur = (WCHAR*)pwzDst;
	WCHAR* pwDstBegin = pwDstCur;

	BOOL bBegin = TRUE;

	while ((*pwCur) != L'\0')
	{
		if (bBegin == TRUE && (*pwCur) == L' ')
		{
			pwCur++;
		}
		else
		{
			bBegin = FALSE;
			(*pwDstCur) = (*pwCur);
			pwCur++;
			pwDstCur++;
		}
	}

	while (pwDstCur != pwDstBegin)
	{
		if ((*pwDstCur) == L'\0')
		{
			pwDstCur--;
		}
		else if ((*pwDstCur) == L' ')
		{
			(*pwDstCur) = L'\0';
			pwDstCur--;
		}
		else
		{
			break;
		}
	}
}

void RemoveSpace(LPCSTR pszSrc, LPSTR pszDst, int nLen)
{
	ZeroMemory(pszDst, nLen);

	char* psCur = (char*) pszSrc;

	char* psDstCur = (char*) pszDst;
	char* psDstBegin = psDstCur;

	BOOL bBegin = TRUE;

	while ((*psCur) != '\0')
	{
		if (bBegin == TRUE && (*psCur) == ' ')
		{
			psCur++;
		}
		else
		{
			bBegin = FALSE;
			(*psDstCur) = (*psCur);
			psCur++;
			psDstCur++;
		}
	}

	while (psDstCur != psDstBegin)
	{
		if ((*psDstCur) == '\0')
		{
			psDstCur--;
		}
		else if ((*psDstCur) == ' ')
		{
			(*psDstCur) = '\0';
			psDstCur--;
		}
		else
		{
			break;
		}
	}
}

int CalcTomorrowYYYYMMDD()
{
#ifdef LIB_WINDOWS
	__time32_t tNow = _time32(NULL)+86400;
	LONGLONG ll = Int32x32To64(tNow, 10000000) + 116444736000000000;
	FILETIME utcft;
	utcft.dwLowDateTime = (DWORD) ll;
	utcft.dwHighDateTime = ll >> 32;

	FILETIME ft;
	FileTimeToLocalFileTime(&utcft, &ft);

	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);
	return (st.wYear * 10000 + st.wMonth * 100 + st.wDay);
#else
	time_t tNow = time(NULL)+86400;

	tm sTimeNow;
	ZeroMemory(&sTimeNow, sizeof(sTimeNow));

	localtime_r(&tNow, &sTimeNow);

	return (((sTimeNow.tm_year + 1900) * 10000) + ((sTimeNow.tm_mon + 1) * 100) + sTimeNow.tm_mday);
#endif // LIB_WINDOWS
}

#ifdef LIB_LINUX
// linux 重新实现一个GetTickCount
DWORD GetTickCount()
{
	struct timespec time1 = {0, 0};
	clock_gettime(CLOCK_MONOTONIC,&time1);

	return ((time1.tv_sec * 1000) + ((int)(time1.tv_nsec / 1000)));
}

int _wtoi(const wchar_t* pwzSrc)
{
	return (int)wcstol(pwzSrc,NULL,0);
}

long _wtol(const wchar_t* pwzSrc)
{
	return wcstol(pwzSrc,NULL,0);
}

double _wtof(const wchar_t* pwzSrc)
{
	return (double)wcstod(pwzSrc,NULL);
}

LONGLONG _wtoll(const wchar_t* pwzSrc)
{
	return (LONGLONG)wcstoll(pwzSrc,NULL,0);
}

LONGLONG _wtoi64(const wchar_t *pwzSrc)
{
	return (LONGLONG)wcstoll(pwzSrc,NULL,0);
}

LONGLONG _atoi64(const char* _String)
{
	return (LONGLONG)atoll(_String);
}

void SetConsoleTitleA(const char* pszTitle)
{
	LOG(pszTitle);	
}

void GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	tm st;
	localtime_r(&tv.tv_sec,&st);

	lpSystemTime->wYear = st.tm_year + 1900;
	lpSystemTime->wMonth = st.tm_mon + 1;
	lpSystemTime->wDay = st.tm_mday;
	lpSystemTime->wDayOfWeek = st.tm_wday;
	lpSystemTime->wHour = st.tm_hour;
	lpSystemTime->wMinute = st.tm_min;
	lpSystemTime->wSecond = st.tm_sec;
	lpSystemTime->wMilliseconds = (WORD)(tv.tv_usec / 1000);
}
#endif // LIB_LINUX

#ifdef LIB_WINDOWS
int gettimeofday(struct timeval *tv, struct timezone *tz) 
{
	FILETIME ft;
	uint64_t tmpres = 0;
	static int tzflag = 0; 


	if(tv)   
	{    
#ifdef _WIN32_WCE
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
#else
		GetSystemTimeAsFileTime(&ft);
#endif


		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;


		/*converting file time to unix epoch*/
		tmpres /= 10;  /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long) (tmpres / 1000000UL);
		tv->tv_usec = (long) (tmpres % 1000000UL);
	}


	if (tz)
	{
		if (!tzflag)
		{
#if !TSK_UNDER_WINDOWS_RT
			_tzset();
#endif
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}
#endif // LIB_WINDOWS


#ifdef LIB_LINUX
BOOL MakeSureDirectoryPathExists(LPCSTR DirPath)
{
	char DirName[MAX_PATH] = {0};  
	strcpy(DirName,DirPath);  
	int i = 0;
	int len = strlen(DirName); 
	if(DirName[len-1] != '/')
	{
		strcat(DirName, "/");
	}

	len = strlen(DirName);  

	for(i = 1 ; i < len ; i++)  
	{
		if( DirName[i] == '/' )  
		{
			DirName[i] = '\0';  
			if( access(DirName , F_OK) != 0 )  
			{
				if( mkdir( DirName,0755 ) != 0)  
				{   
					return FALSE;   
				}  
			}  
			DirName[i] = '/';
		}  
	}

	return TRUE;
}

size_t GetFileSize(FILE* pFile)
{
	if (pFile == NULL)
	{
		return -1;
	}

	fseek(pFile,0,SEEK_END);

	size_t nSize =  ftell(pFile);

	fseek(pFile,0,SEEK_SET);

	return nSize;
}

void PrintBackTrace()
{
#define MAX_BACK_TRACE 30
	size_t nSizeTrace = 0;
	void* ayBackTrace[MAX_BACK_TRACE] = {0};
	char** pTrace = NULL;

	nSizeTrace = backtrace (ayBackTrace, MAX_BACK_TRACE);

	pTrace = backtrace_symbols (ayBackTrace, nSizeTrace);

	if (pTrace == NULL)
	{
		LOG("!!!!!!!!!!!!!!!!!!!!!!!!!!GET BACKTRACE FAILED!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		return;
	}

	LOG("--------------------------BACKTRACE SIZE : %d-------------------------",nSizeTrace);
	for (int i = 0;i < nSizeTrace;++i)
	{
		LOG("TRACE : %s",pTrace[i]);
	}
	LOG("-----------------------------BACKTRACE END---------------------------");
	free(pTrace);
	pTrace = NULL;
}

bool BlockSignal()
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGILL);
	sigaddset(&set, SIGABRT);
	sigaddset(&set, SIGFPE);
	if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0)
	{
		return false;
	}
	return true;
}

void SetConsoleCtrlHandler(signal_handle handle)
{
	signal(SIGINT, handle);
	signal(SIGTERM, handle);
	signal(SIGQUIT, handle);
	signal(SIGILL, handle);
	signal(SIGABRT, handle);
	signal(SIGFPE, handle);
	signal(SIGSEGV, handle);
	signal(SIGPIPE, SIG_IGN);
}
#endif // LIB_LINUX

void SafeTerminateProcess()
{
#ifdef LIB_WINDOWS
	GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
#else
	kill(0,SIGINT);
#endif // LIB_WINDOWS
}

// 检查是否是debug版本
bool CheckDebugVersion()
{
	char szPath[MAX_PATH] = { 0 };
	GetModuleFilePath(szPath, MAX_PATH - 1);
	char szDebugFile[MAX_PATH] = { 0 };
	_snprintf_s(szDebugFile, MAX_PATH - 1, "%s" DIRECTORY_SEPARATOR "debug", szPath);
	FILE* pFile = fopen(szDebugFile, "r");
	if (pFile == NULL)
	{
		printf_s("DEBUG MODE need create file debug\n");
		return false;
	}

	fclose(pFile);
	return true;
}

// 检查是否是零时区
bool CheckTimezoneZero()
{
#ifdef LIB_WINDOWS
	TIME_ZONE_INFORMATION sTimeZone;
	GetTimeZoneInformation(&sTimeZone);
	if (sTimeZone.Bias != 0 || sTimeZone.DaylightBias != 0)
	{
		printf_s("!!!Cur Timezone is not 0,Are you sure???Please press (y/Y) to continue.......\n");
		char cRet = (char)getchar();
		if (cRet != 'y' && cRet != 'Y')
		{
			return false;
		}
	}
#else
	time_t tCur = 0;
	struct tm tmCur;
	time(&tCur);
	localtime_r(&tCur, &tmCur);
	if (tmCur.tm_gmtoff != 0 || strcmp(tmCur.tm_zone, "UTC") != 0)
	{
		perror("!!!Cur Timezone is not 0,Are you sure???Please press (y/Y) to continue.......\n");
		char cRet = getchar();
		if (cRet != 'y' && cRet != 'Y')
		{
			return false;
		}
	}
#endif // LIB_WINDOWS
	return true;
}

int GetLocalTimeZone()
{
#ifdef LIB_WINDOWS
	TIME_ZONE_INFORMATION sLocalTimeZone;
	ZeroMemory(&sLocalTimeZone, sizeof(sLocalTimeZone));
	GetTimeZoneInformation(&sLocalTimeZone);
	return (int)(sLocalTimeZone.Bias * -60);
#else
	tm st;
	localtime_r(NULL, &st);

	return (int)-st.tm_gmtoff;
#endif
}

const char* _stristr(const char* _Src, const char* _Search)
{
	return StrStrIA(_Src,_Search);
}

__time32_t time32()
{
#ifdef LIB_WINDOWS
	return _time32(NULL);
#else
	return time(NULL);
#endif
}

longtime_t GetLongTime()
{
	timeval tv;
	gettimeofday(&tv, NULL);

	longtime_t tNow = ((longtime_t) tv.tv_sec + ((longtime_t) tv.tv_usec / 1000000.000));

	return tNow;
}