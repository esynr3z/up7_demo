////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// 2012-2021 (c) Baical                                                        /
//                                                                             /
// This library is free software; you can redistribute it and/or               /
// modify it under the terms of the GNU Lesser General Public                  /
// License as published by the Free Software Foundation; either                /
// version 3.0 of the License, or (at your option) any later version.          /
//                                                                             /
// This library is distributed in the hope that it will be useful,             /
// but WITHOUT ANY WARRANTY; without even the implied warranty of              /
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           /
// Lesser General Public License for more details.                             /
//                                                                             /
// You should have received a copy of the GNU Lesser General Public            /
// License along with this library.                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////
//http://www.codeproject.com/Articles/207464/Exception-Handling-in-Visual-Cplusplus

#ifndef PCRASH_DUMP_H
#define PCRASH_DUMP_H

#pragma warning(disable : 4091)

#include <new.h>
#include <Shlwapi.h>
#include "DbgHelp.h"
#include <crtdbg.h>
#include "Shlobj.h"
#include "signal.h"
#include "GTypes.h"
#include "Length.h"
#include "PString.h"
#include "WString.h"
#include "AList.h"
#include "PAtomic.h"
#include "ISignal.h"


#define CH_DUMP_FILE_EXT                                  L"dmp"
#define CH_DESC_FILE_EXT                                  L"dsc"

typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)
    (HANDLE                                  i_hProcess, 
     DWORD                                   i_dwProcessId, 
     HANDLE                                  i_hFile, 
     MINIDUMP_TYPE                           i_eDumpType, 
     CONST PMINIDUMP_EXCEPTION_INFORMATION   i_pExceptionParam, 
     CONST PMINIDUMP_USER_STREAM_INFORMATION i_pUserEncoderParam, 
     CONST PMINIDUMP_CALLBACK_INFORMATION    i_pCallbackParam
    );


struct sCdContext
{
    char                *pBuffer;
    size_t               szBuffer;
    size_t               szPathMax;
    wchar_t             *pDumpFileName;
    wchar_t             *pDescFileName;

    DWORD                dwVer0;
    DWORD                dwVer1;
    DWORD                dwVer2;
    DWORD                dwVer3;

    HMODULE              hDbgDll;
    LPMINIDUMPWRITEDUMP  pfnMiniDump;
    HANDLE               hProcess;
    DWORD                dwProcessId;
};


////////////////////////////////////////////////////////////////////////////////
//CdCreateDumpTxt
static void CdCreateDumpTxt(sCdContext *i_pCdContext, const char *i_pText, UINT64 i_qwCode)
{   
    HANDLE       l_hFile     = NULL;
    SYSTEMTIME   l_sTime     = {};
    DWORD        l_dwSize    = 0;
    size_t       l_szBuffer  = 0;
    size_t       l_szOnDisc  = 0;
    UINT64       l_qwFree    = 0;

    
    ////////////////////////////////////////////////////////////////////////////
    // Create the minidump text desc file
    l_hFile = CreateFileW(i_pCdContext->pDescFileName,
                          GENERIC_WRITE | GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );

    if (    (INVALID_HANDLE_VALUE == l_hFile)
         || (NULL == l_hFile)
       )
    {
        l_hFile = NULL;
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////
    //print info
    GetLocalTime(&l_sTime);
    sprintf_s(i_pCdContext->pBuffer, 
              i_pCdContext->szBuffer, 
              "Time :%02d.%02d.%04d %02d:%02d:%0d\r\n"
              "Ver. :%d.%d.%d.%d\r\n"
              "Idx  :0x%I64X\r\n"
              "Txt  :%s\r\n",
              (DWORD)l_sTime.wDay,
              (DWORD)l_sTime.wMonth,
              (DWORD)l_sTime.wYear,
              (DWORD)l_sTime.wHour,
              (DWORD)l_sTime.wMinute,
              (DWORD)l_sTime.wSecond,
              i_pCdContext->dwVer0,
              i_pCdContext->dwVer1,
              i_pCdContext->dwVer2,
              i_pCdContext->dwVer3,
              i_qwCode,
              i_pText ? i_pText : "-"
             );

    ////////////////////////////////////////////////////////////////////////////
    //write data
    l_szBuffer = strlen(i_pCdContext->pBuffer);
    l_szOnDisc = 0;

    while (l_szBuffer > l_szOnDisc)
    {
        DWORD l_dwWritten = 0;
        if (WriteFile(l_hFile, 
                      i_pCdContext->pBuffer + l_szOnDisc, 
                      (DWORD)(l_szBuffer - l_szOnDisc),
                      &l_dwWritten,
                      NULL
                     )
            )
        {
            l_szOnDisc += l_dwWritten;
            if (0 == l_dwWritten)
            {
                break;
            }
        }
        else
        {
            break;
        }
    } 

    FlushFileBuffers(l_hFile);


l_lblExit:
    if (NULL != l_hFile)
    {
        CloseHandle(l_hFile);
        l_hFile = NULL;
    }
}//CdCreateDumpTxt


////////////////////////////////////////////////////////////////////////////////
//CdCreateDumpBin
static UINT64 CdCreateDumpBin(sCdContext *i_pCdContext, EXCEPTION_POINTERS* i_pException)
{   
    MINIDUMP_EXCEPTION_INFORMATION l_sMEI     = {};
    MINIDUMP_CALLBACK_INFORMATION  l_sMCI     = {};
    HANDLE                         l_hFile    = NULL;
    UINT64                         l_qwReturn = 0;
    
    if (NULL == i_pException)
    {
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Create the minidump file
    l_hFile = CreateFileW(i_pCdContext->pDumpFileName,
                          GENERIC_WRITE | GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                        );

    if (    (INVALID_HANDLE_VALUE == l_hFile)
         || (NULL == l_hFile)
       )
    {
        l_qwReturn = 0x0100000000000000ULL + (UINT64)GetLastError();
        l_hFile = NULL;
        goto l_lblExit;
    }
   
    l_sMEI.ThreadId          = GetCurrentThreadId();
    l_sMEI.ExceptionPointers = i_pException;
    l_sMEI.ClientPointers    = FALSE;
    l_sMCI.CallbackRoutine   = NULL;
    l_sMCI.CallbackParam     = NULL;


    ////////////////////////////////////////////////////////////////////////////
    //write damp file
    i_pCdContext->hProcess = GetCurrentProcess();
    if (FALSE == i_pCdContext->pfnMiniDump(i_pCdContext->hProcess, 
                                           i_pCdContext->dwProcessId, 
                                           l_hFile, 
                                           (MINIDUMP_TYPE)(   MiniDumpNormal 
                                                            | MiniDumpWithProcessThreadData 
                                                            | MiniDumpWithThreadInfo
                                                          ),
                                           &l_sMEI, 
                                           NULL, 
                                           &l_sMCI
                                          )
       )
    {    
        l_qwReturn = 0x0200000000000000ULL + (UINT64)GetLastError();
        goto l_lblExit;
    }

l_lblExit:
    if (NULL != l_hFile)
    {
        CloseHandle(l_hFile);
        l_hFile = NULL;
    }

    return l_qwReturn;
}//CdCreateDumpBin


////////////////////////////////////////////////////////////////////////////////
//CdWrite
static void CdWrite(void *i_pCdInfo, eCrashCode i_eCode, const void *i_pContext)
{
    sCdContext *l_pCrush = (sCdContext*)i_pCdInfo;

    if ( eCrashException == i_eCode )
    {
        if (i_pContext)
        {
            _EXCEPTION_POINTERS l_sException = *(_EXCEPTION_POINTERS*)(i_pContext);

            char l_pText[128];
            sprintf_s(l_pText, 
                      _countof(l_pText), 
                      "Exception 0x%08X", 
                      l_sException.ExceptionRecord->ExceptionCode
                     );

            tUINT64 l_qwCode = CdCreateDumpBin(l_pCrush, &l_sException);
            CdCreateDumpTxt(l_pCrush, l_pText, l_qwCode);

        }
    }
    else if (eCrashPureCall == i_eCode)
    {
        CdCreateDumpTxt(l_pCrush, (const char*)i_pContext, i_eCode);
    }
    else if (eCrashMemAlloc == i_eCode)
    {
        CdCreateDumpTxt(l_pCrush, (const char*)i_pContext, i_eCode);
    }
    else if (eCrashInvalidParameter == i_eCode)
    {
        CdCreateDumpTxt(l_pCrush, (const char*)i_pContext, i_eCode);
    }
    else if ( eCrashSignal <= i_eCode)
    {
        CdCreateDumpTxt(l_pCrush, (const char*)i_pContext, i_eCode);
    }
}//CdWrite


////////////////////////////////////////////////////////////////////////////////
//CdCleanUp
static void CdCleanUp(const wchar_t *i_pDir, unsigned int i_dwMax_Count)
{
    CBList<CWString*> l_cFiles;
    CWString          l_cDir;

    l_cDir.Set(i_pDir);
    CFSYS::Enumerate_Files(&l_cFiles, &l_cDir, L"*." CH_DESC_FILE_EXT, 0);

    //sort files - first is oldest, last is newest !
    pAList_Cell l_pStart   = NULL;
    tUINT32     l_dwDirLen = l_cDir.Length();

    while ((l_pStart = l_cFiles.Get_Next(l_pStart)))
    {
        pAList_Cell l_pMin  = l_pStart;
        pAList_Cell l_pIter = l_pStart;

        while ((l_pIter = l_cFiles.Get_Next(l_pIter)))
        {
            CWString *l_pPathM = l_cFiles.Get_Data(l_pMin);
            CWString *l_pPathI = l_cFiles.Get_Data(l_pIter);
            tXCHAR   *l_pNameM = l_pPathM->Get() + l_dwDirLen;
            tXCHAR   *l_pNameI = l_pPathI->Get() + l_dwDirLen;

            if (0 < PStrICmp(l_pNameM, l_pNameI))
            {
                l_pMin = l_pIter;
            }
        }

        if (l_pMin != l_pStart)
        {
            l_cFiles.Extract(l_pMin);
            l_cFiles.Put_After(l_cFiles.Get_Prev(l_pStart), l_pMin);
            l_pStart = l_pMin;
        }
    } //while (l_pStart = m_cFiles.Get_Next(l_pStart))

    //delete oldest files
    while (l_cFiles.Count() > i_dwMax_Count)
    {
        l_pStart = l_cFiles.Get_First();
        if (l_pStart)
        {
            CWString *l_pPath = l_cFiles.Get_Data(l_pStart);
            
            //delete description file
            CFSYS::Delete_File(l_pPath->Get());

            //delete dump file if it is
            l_pPath->Trim((tUINT32)(l_pPath->Length() - wcslen(CH_DESC_FILE_EXT)));
            l_pPath->Append(1, CH_DUMP_FILE_EXT);
            CFSYS::Delete_File(l_pPath->Get());

            l_cFiles.Del(l_pStart, TRUE);
        }
    }//while (l_cFiles.Count() > i_dwMax_Count)
}//CdCleanUp


////////////////////////////////////////////////////////////////////////////////
//CdAlloc
static void *CdAlloc(const tXCHAR *i_pFolder)
{
    wchar_t     *l_pName     = NULL;
    wchar_t     *l_pPath     = NULL;
    DWORD        l_dwUnknown = 0;
    DWORD        l_dwSize    = 0;
    SYSTEMTIME   l_sTime     = {0};
    sCdContext  *l_pCrush    = (sCdContext*)malloc(sizeof(sCdContext));
    tBOOL        l_bError    = FALSE;

    if ( !l_pCrush )
    {
        return NULL;
    }


    ////////////////////////////////////////////////////////////////////////////
    //initialize parameters
    memset(l_pCrush, 0, sizeof(sCdContext));

    l_pCrush->szBuffer      = 32768;
    l_pCrush->pBuffer       = (char*)malloc(l_pCrush->szBuffer * sizeof(char));
    l_pCrush->szPathMax     = 16384;
    l_pCrush->pDumpFileName = (wchar_t*)malloc(l_pCrush->szPathMax * sizeof(wchar_t));
    l_pCrush->pDescFileName = (wchar_t*)malloc(l_pCrush->szPathMax * sizeof(wchar_t));
    l_pCrush->hDbgDll       = LoadLibraryW(L"dbghelp.dll");
    l_pCrush->pfnMiniDump   = (LPMINIDUMPWRITEDUMP)GetProcAddress(l_pCrush->hDbgDll, 
                                                                 "MiniDumpWriteDump"
                                                                );
    l_pCrush->dwProcessId   = GetCurrentProcessId();
    l_pCrush->hProcess      = GetCurrentProcess();

    if (    (NULL == l_pCrush->pBuffer)
         || (NULL == l_pCrush->pDumpFileName)
         || (NULL == l_pCrush->pDescFileName)
         || (NULL == l_pCrush->hDbgDll)
         || (NULL == l_pCrush->pfnMiniDump)
         || (NULL == l_pCrush->hProcess)
       )
    {
        l_bError = TRUE;
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////
    //get current executable file path
    if (0 == GetModuleFileNameW(GetModuleHandleW(NULL), 
                                (wchar_t*)l_pCrush->pBuffer, 
                                (DWORD)(l_pCrush->szBuffer / sizeof(wchar_t))
                               )
       )
    {
        l_bError = TRUE;
        goto l_lblExit;
    }

    l_pPath = (wchar_t*)l_pCrush->pBuffer;

    ////////////////////////////////////////////////////////////////////////////
    //get version
    l_dwSize = GetFileVersionInfoSizeW(l_pPath, &l_dwUnknown);
    if (l_dwSize)
    {
        BYTE *l_pFileInfo = (BYTE*)malloc(l_dwSize);
        if (l_pFileInfo)
        {
            memset(l_pFileInfo, 0, l_dwSize);

            if (GetFileVersionInfoW(l_pPath, 0, l_dwSize, l_pFileInfo) )
            {
                //wchar_t l_pVerPath[256];
                VS_FIXEDFILEINFO *l_pVersion = NULL;
                UINT l_dwSize = 0;
                if (VerQueryValueW(l_pFileInfo, L"\\", (LPVOID *)&l_pVersion, &l_dwSize))
                {
                    l_pCrush->dwVer0 = l_pVersion->dwFileVersionMS >> 16;
                    l_pCrush->dwVer1 = l_pVersion->dwFileVersionMS & 0xFFFF;
                    l_pCrush->dwVer2 = l_pVersion->dwFileVersionLS >> 16;
                    l_pCrush->dwVer3 = l_pVersion->dwFileVersionLS & 0xFFFF;
                }
            }

            if (l_pFileInfo)
            {
                free(l_pFileInfo);
                l_pFileInfo = NULL;
            }
        } //if (l_pFileInfo)
    } //if (l_dwSize)

    ////////////////////////////////////////////////////////////////////////////
    //create file names
    if (    (l_pName = wcsrchr(l_pPath, L'\\'))
         || (l_pName = wcsrchr(l_pPath, L'/'))
       )
    {
        l_pName[0] = 0;
        l_pName ++;
    }

    wcscpy_s(l_pPath, l_pCrush->szBuffer / sizeof(wchar_t), i_pFolder);


    if (!CFSYS::Directory_Create(l_pPath))
    {
        l_bError = TRUE;
        goto l_lblExit;
    }

    CdCleanUp(l_pPath, 5);

    GetLocalTime(&l_sTime);


    swprintf_s(l_pCrush->pDumpFileName, 
               l_pCrush->szPathMax, 
               L"%s\\%04d_%02d_%02d__%02d_%02d_%02d." CH_DUMP_FILE_EXT,
               l_pPath,
               (DWORD)l_sTime.wYear,
               (DWORD)l_sTime.wMonth,
               (DWORD)l_sTime.wDay,
               (DWORD)l_sTime.wHour,
               (DWORD)l_sTime.wMinute,
               (DWORD)l_sTime.wSecond
              );

    swprintf_s(l_pCrush->pDescFileName, 
               l_pCrush->szPathMax, 
               L"%s\\%04d_%02d_%02d__%02d_%02d_%02d." CH_DESC_FILE_EXT,
               l_pPath,
               (DWORD)l_sTime.wYear,
               (DWORD)l_sTime.wMonth,
               (DWORD)l_sTime.wDay,
               (DWORD)l_sTime.wHour,
               (DWORD)l_sTime.wMinute,
               (DWORD)l_sTime.wSecond
              );

        
l_lblExit:
    if (    (l_bError)
         && (l_pCrush)
       )
    {
        free(l_pCrush);
        l_pCrush = NULL;
    }


    return l_pCrush;
}//CH_Install



////////////////////////////////////////////////////////////////////////////////
//CdFree
static void CdFree(void *i_pCdInfo)
{
    sCdContext *l_pCrush = (sCdContext*)i_pCdInfo;

    if ( !l_pCrush )
    {
        return;
    }

    if (l_pCrush->pBuffer)
    {
        free(l_pCrush->pBuffer);
        l_pCrush->pBuffer = NULL;
    }

    if (l_pCrush->pDumpFileName)
    {
        free(l_pCrush->pDumpFileName);
        l_pCrush->pDumpFileName = NULL;
    }

    if (l_pCrush->pDescFileName)
    {
        free(l_pCrush->pDescFileName);
        l_pCrush->pDescFileName = NULL;
    }

    if (l_pCrush->hDbgDll)
    {
        FreeLibrary(l_pCrush->hDbgDll);
        l_pCrush->hDbgDll = NULL;
    }
}//CdFree

#endif //PCRASH_DUMP_H