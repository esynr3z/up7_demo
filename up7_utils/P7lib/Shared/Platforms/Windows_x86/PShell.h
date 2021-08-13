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
#pragma once

class CShell
{
public:
    ////////////////////////////////////////////////////////////////////////////
    //Execute
    static tBOOL Execute(wchar_t *i_pCmd, const wchar_t *i_pDir, tBOOL i_bWait)
    {
        STARTUPINFOW        l_tSi              = {0};
        PROCESS_INFORMATION l_tPi              = {0};
        tBOOL               l_bRes             = TRUE;
        HANDLE              l_dwSTD_Out_RD     = NULL;
        HANDLE              l_dwSTD_Out_WR     = NULL;
        HANDLE              l_dwSTD_In_RD      = NULL; 
        HANDLE              l_dwSTD_In_WR      = NULL;
        HANDLE              l_dwSTD_In_WR_Dup  = NULL;
        HANDLE              l_dwSTD_Out_RD_Dup = NULL;
        DWORD               l_dwProcess_Result = 0;
        SECURITY_ATTRIBUTES l_tsaAttr          = {0}; 


        l_tsaAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
        l_tsaAttr.bInheritHandle = TRUE; 
        l_tsaAttr.lpSecurityDescriptor = NULL; 
    
        ZeroMemory( &l_tSi, sizeof(l_tSi) );
        l_tSi.cb = sizeof(l_tSi);

        ZeroMemory( &l_tPi, sizeof(l_tPi) );

        if (    (l_bRes) 
             && (! CreatePipe(&l_dwSTD_Out_RD, &l_dwSTD_Out_WR, &l_tsaAttr, 0))
           )
        {
            l_bRes = FALSE;
        }

        if (    (l_bRes) 
             && (! CreatePipe(&l_dwSTD_In_RD, &l_dwSTD_In_WR, &l_tsaAttr, 0))
           ) 
        {
            l_bRes = FALSE;
        }
    
        if (    (l_bRes) 
             && (! DuplicateHandle(GetCurrentProcess(), 
                                   l_dwSTD_Out_RD, 
                                   GetCurrentProcess(), 
                                   &l_dwSTD_Out_RD_Dup, 
                                   0, 
                                   FALSE, 
                                   DUPLICATE_SAME_ACCESS
                                  )
                )
           )
        {
            l_bRes = FALSE;
        }
  
        if (    (l_bRes)
             && (!DuplicateHandle(GetCurrentProcess(), 
                                  l_dwSTD_In_WR,  
                                  GetCurrentProcess(), 
                                  &l_dwSTD_In_WR_Dup, 
                                  0, 
                                  FALSE, 
                                  DUPLICATE_SAME_ACCESS
                                 )
                )
           )
        {
            l_bRes = FALSE;
        }
    
        if (l_dwSTD_Out_RD)
        {
            CloseHandle(l_dwSTD_Out_RD);
            l_dwSTD_Out_RD = NULL;
        }
    
        if (l_dwSTD_In_WR)
        {
            CloseHandle(l_dwSTD_In_WR); 
            l_dwSTD_In_WR = NULL;
        }
    
        l_tSi.hStdError  = l_dwSTD_Out_WR;
        l_tSi.hStdOutput = l_dwSTD_Out_WR;
        l_tSi.hStdInput  = l_dwSTD_In_RD;
        l_tSi.dwFlags   |= STARTF_USESTDHANDLES;
    
        if (    (l_bRes)
             && (CreateProcessW(NULL, 
                                i_pCmd, 
                                NULL, 
                                NULL, 
                                TRUE, 
                                CREATE_NO_WINDOW,
                                NULL, 
                                i_pDir, 
                                &l_tSi, 
                                &l_tPi
                               )
                )
           )
        {
            WaitForInputIdle(l_tPi.hProcess, INFINITE);

            if (i_bWait)
            {
                // Wait until child process exits.
                while (WAIT_OBJECT_0 != WaitForSingleObject( l_tPi.hProcess, 50 ))
                {
                    CShell::Read(l_dwSTD_Out_RD_Dup);
                }
            }

            CShell::Read(l_dwSTD_Out_RD_Dup);

            if (GetExitCodeProcess(l_tPi.hProcess, &l_dwProcess_Result))
            {
                if (l_dwProcess_Result)
                {
                    //LOG_WARNING(L"Command execute error code = %d\n", l_dwProcess_Result);
                }
            }

            // Close process and thread handles. 
            CloseHandle( l_tPi.hProcess );
            CloseHandle( l_tPi.hThread );
        }
        else
        {
            l_bRes = FALSE;
        }

        if (l_dwSTD_In_WR_Dup)
        {
            CloseHandle( l_dwSTD_In_WR_Dup );
            l_dwSTD_In_WR_Dup = NULL;
        }

        if (l_dwSTD_Out_RD_Dup)
        {
            CloseHandle( l_dwSTD_Out_RD_Dup );
            l_dwSTD_Out_RD_Dup = NULL;
        }

        if (l_dwSTD_Out_WR)
        {
            CloseHandle( l_dwSTD_Out_WR );
            l_dwSTD_Out_WR = NULL;
        }

        if (l_dwSTD_In_RD)
        {
            CloseHandle( l_dwSTD_In_RD );
            l_dwSTD_In_RD = NULL;
        }

        return l_bRes;
    }// Execute


    ////////////////////////////////////////////////////////////////////////////
    //
    static tBOOL Read(HANDLE i_hStdOutPipe)
    {
        DWORD l_dwRealCount   = 0;
        DWORD l_dwBytesInPipe = 0;
        DWORD l_dwIteration   = 0;
        char  l_pText[1024];
    
        if (    (INVALID_HANDLE_VALUE == i_hStdOutPipe) 
             || (NULL == i_hStdOutPipe)
           )
        {
            return FALSE;
        }
    
        Sleep(500);

        l_dwBytesInPipe = 0;
        PeekNamedPipe(i_hStdOutPipe, NULL, 0, NULL, &l_dwBytesInPipe, NULL);
        while (l_dwBytesInPipe)
        {
            ReadFile(i_hStdOutPipe, 
                     l_pText, 
                     (l_dwBytesInPipe >= (LENGTH(l_pText) - 1)) ? (LENGTH(l_pText) - 1) : l_dwBytesInPipe, 
                     &l_dwRealCount, 
                     NULL
                    );

            if (l_dwRealCount)
            {
                DWORD l_dwOffset = 0;
                DWORD l_dwLength = 0;

                l_pText[l_dwRealCount] = 0;

                for (DWORD l_dwI = 0; l_dwI < l_dwRealCount; l_dwI ++)
                {
                    if (    (0xD == l_pText[l_dwI])
                         || (0xA == l_pText[l_dwI])
                         || (0x0 == l_pText[l_dwI])
                       )
                    {
                        l_pText[l_dwI] = 0;
                        if (l_dwLength)
                        {
                            OutputDebugStringA(l_pText + l_dwOffset);
                            OutputDebugStringA("\n");
                        }

                        l_dwOffset = l_dwI + 1;
                        l_dwLength = 0;
                    }
                    else
                    {
                        l_dwLength ++;
                    }
                }

                PeekNamedPipe(i_hStdOutPipe, NULL, 0, NULL, &l_dwBytesInPipe, NULL);
                l_dwIteration = 0;
            }                    
            else
            {
                Sleep(100);
                l_dwIteration++;
                if (l_dwIteration > 30)
                {
                    //LOG_ERROR(L"StrOut reading timeout !\n");
                    l_dwBytesInPipe = 0;
                }
            }
        }
    
        return TRUE;
    }// SYS_Read_StdOut

};
