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
#include "uP7preCommon.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ParseFunctions(Cfg::INode *i_pOpions, CFunctionsList &o_rFunctions)
{
    bool        l_bReturn = true;
    Cfg::INode *l_pFunc   = NULL;
    Cfg::INode *l_pGroup  = NULL;

    if (    (!i_pOpions)
         || (Cfg::eOk != i_pOpions->GetChildFirst(XML_NODE_OPTIONS_FUNC, &l_pGroup))
       )
    {
        return false;
    }

    l_pGroup->GetChildFirst(&l_pFunc);

    while (l_pFunc)
    {
        tXCHAR *l_pName = NULL;
        if (Cfg::eOk != l_pFunc->GetName(&l_pName))
        {
            l_bReturn = false;
            break;
        }

        if (0 == PStrCmp(l_pName, XML_NODE_OPTIONS_FUNC_TRACE))
        {
            tXCHAR *l_pName           = NULL;
            tINT32  l_iIdIndex        = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iFormatStrIndex = UP7_FUNC_UNDEFINED_INDEX;
            if (    (Cfg::eOk == l_pFunc->GetAttrText(XML_NARG_OPTIONS_FUNC_NAME, &l_pName))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_TRACE_ID, &l_iIdIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_TRACE_FORMAT, &l_iFormatStrIndex))
               )
            {
                stFuncDesc *l_pFunction = new stFuncDesc(l_pName, stFuncDesc::eTrace);
                l_pFunction->pArgs[eTraceIdIndex]           = l_iIdIndex;
                l_pFunction->pArgs[eTraceFormatStringIndex] = l_iFormatStrIndex;
                o_rFunctions.Push_Last(l_pFunction);
            }
            else
            {
                l_bReturn = false;
                break;
            }
        }
        else if (0 == PStrCmp(l_pName, XML_NODE_OPTIONS_FUNC_REGMOD))
        {
            tXCHAR *l_pName       = NULL;
            tINT32  l_iNameIndex  = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iLevelIndex = UP7_FUNC_UNDEFINED_INDEX;
            if (    (Cfg::eOk == l_pFunc->GetAttrText(XML_NARG_OPTIONS_FUNC_NAME, &l_pName))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_REGMOD_NAME, &l_iNameIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_REGMOD_LEVEL, &l_iLevelIndex))
               )
            {
                stFuncDesc *l_pFunction = new stFuncDesc(l_pName, stFuncDesc::eRegisterModule);
                l_pFunction->pArgs[eRegModNameIndex]  = l_iNameIndex;
                l_pFunction->pArgs[eRegModLevelIndex] = l_iLevelIndex;
                o_rFunctions.Push_Last(l_pFunction);
            }
            else
            {
                l_bReturn = false;
                break;
            }
        }
        if (0 == PStrCmp(l_pName, XML_NODE_OPTIONS_FUNC_MKCOUNTER))
        {
            tXCHAR *l_pName      = NULL;
            tINT32  l_iNameIndex = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iMinIndex  = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iAMinIndex = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iMaxIndex  = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iAMaxIndex = UP7_FUNC_UNDEFINED_INDEX;
            tINT32  l_iOnIndex   = UP7_FUNC_UNDEFINED_INDEX;
            if (    (Cfg::eOk == l_pFunc->GetAttrText(XML_NARG_OPTIONS_FUNC_NAME, &l_pName))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_MKCOUNTER_NAME, &l_iNameIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_MKCOUNTER_MIN , &l_iMinIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_MKCOUNTER_AMIN, &l_iAMinIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_MKCOUNTER_MAX , &l_iMaxIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_MKCOUNTER_AMAX, &l_iAMaxIndex))
                 && (Cfg::eOk == l_pFunc->GetAttrInt32(XML_NARG_OPTIONS_FUNC_MKCOUNTER_ON  , &l_iOnIndex))
               )
            {
                stFuncDesc *l_pFunction = new stFuncDesc(l_pName, stFuncDesc::eCreateCounter);
                l_pFunction->pArgs[eMkCounterNameIndex]     = l_iNameIndex;
                l_pFunction->pArgs[eMkCounterMinIndex]      = l_iMinIndex;
                l_pFunction->pArgs[eMkCounterAlarmMinIndex] = l_iAMinIndex;
                l_pFunction->pArgs[eMkCounterMaxIndex]      = l_iMaxIndex;
                l_pFunction->pArgs[eMkCounterAlarmMaxIndex] = l_iAMaxIndex;
                l_pFunction->pArgs[eMkCounterOnIndex]       = l_iOnIndex;
                o_rFunctions.Push_Last(l_pFunction);
            }
            else
            {
                l_bReturn = false;
                break;
            }
        }

        Cfg::INode *l_pNext = NULL;
        l_pFunc->GetNext(&l_pNext);
        l_pFunc->Release();
        l_pFunc = l_pNext;
    }
    

    SAFE_RELEASE(l_pFunc);
    SAFE_RELEASE(l_pGroup);


    if (    (l_bReturn)
         && (0 == o_rFunctions.Count()) 
       )
    {
        l_bReturn = false;
    }

    return l_bReturn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32) || defined(_WIN64)
int wmain(int i_iArgC, tXCHAR *i_pArgV[])
#elif defined(__linux__)
int main(int i_iArgC, tXCHAR *i_pArgV[])
#endif
{
    CBList<CWString*> l_cFiles;
    Cfg::IDoc        *l_iDoc     = NULL;
    Cfg::INode       *l_pRoot    = NULL;
    Cfg::INode       *l_pOptions = NULL;
    Cfg::INode       *l_pProcess = NULL;
    Cfg::INode       *l_pDescrip = NULL;
    Cfg::INode       *l_pPattern = NULL;
    Cfg::INode       *l_pRoFiles = NULL;
    Cfg::INode       *l_pFiles   = NULL;
    Cfg::INode       *l_pCPU     = NULL;
    int               l_iReturn  = eErrorNo;
    pAList_Cell       l_pEl      = NULL;
    CWString          l_cDir;
    CpreManager       l_cManager;

    if (4 > i_iArgC)
    {
        printf("uP7preProcessor <config.xml> <sources files dir> <output dir>\n");
        printf("Not all arguments are specified\n");
        printf("Please refer to documentation for further details\n");
        l_iReturn = eErrorArguments;
        goto l_lblExit;
    }

    if (!CFSYS::Directory_Exists(i_pArgV[DIR_SRC_INDEX]))
    {
        printf("Source folder doesn't exists\n");
        l_iReturn = eErrorArguments;
        goto l_lblExit;
    }

    if (!CFSYS::Directory_Exists(i_pArgV[DIR_OUT_INDEX]))
    {
        printf("Source folder doesn't exists\n");
        l_iReturn = eErrorArguments;
        goto l_lblExit;
    }

    l_cManager.SetSourcesDir(i_pArgV[DIR_SRC_INDEX]);
    l_cManager.SetOutputDir(i_pArgV[DIR_OUT_INDEX]);

    l_iDoc = IBDoc_Load(i_pArgV[CFG_FILE_INDEX]);
    if (    (!l_iDoc)
         || (!l_iDoc->Get_Initialized())
         || (Cfg::eOk != l_iDoc->GetChildFirst(XML_NODE_MAIN, &l_pRoot))
         || (Cfg::eOk != l_pRoot->GetChildFirst(XML_NODE_OPTIONS, &l_pOptions))
         || (Cfg::eOk != l_pOptions->GetChildFirst(XML_NODE_OPTIONS_PROCESS, &l_pProcess))
       )
    {
        printf("uP7preProcessor config file opening/parsing error\n");
        l_iReturn = eErrorXmlParsing;
        goto l_lblExit;
    }

   
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Scan project properties
    l_pOptions->GetChildFirst(XML_NODE_OPTIONS_PROJECT, &l_pCPU);
    if (l_pCPU)
    {
        tXCHAR *l_pName = NULL;
        if (    (Cfg::eOk == l_pCPU->GetAttrText(XML_ATTE_OPTIONS_PROJECT_NAME, &l_pName))
             && (l_pName)
             && (PStrLen(l_pName) >= 1)
           )
        {
            l_cManager.SetName(l_pName);
        }
        else
        {
            printf("uP7preProcessor config file opening/parsing error: project name isn't specified\n");
            l_iReturn = eErrorXmlParsing;
        }


        tXCHAR *l_pBits = NULL;
        l_pCPU->GetAttrText(XML_ATTE_OPTIONS_PROJECT_BITS, &l_pBits);
        if (    (l_pBits)
             && (0 == PStrICmp(l_pBits, TM("x64")))
           )
        {
            l_cManager.SetTargetCpuBitsCount(64);
        }

        int32_t l_uWchar = 0;
        l_pCPU->GetAttrInt32(XML_ATTE_OPTIONS_PROJECT_WCHAR, &l_uWchar);
        if (    (l_uWchar)
             && (    (16 == l_uWchar) 
                  || (32 == l_uWchar)
                )
           )
        {
            l_cManager.SetTargetCpuWCharBitsCount(l_uWchar);
        }
        else
        {
            printf("uP7preProcessor config file opening/parsing error: unknown Wchar size\n");
            l_iReturn = eErrorXmlParsing;
        }

        tXCHAR *l_pIdsHeader = NULL;
        l_pCPU->GetAttrText(XML_ATTE_OPTIONS_PROJECT_IDS, &l_pIdsHeader);
        if (    (l_pIdsHeader)
             && (0 == PStrICmp(l_pIdsHeader, TM("true")))
           )
        {
            l_cManager.EnableIDsHeader();
        }

        l_pCPU->Release();
        l_pCPU = NULL;
    }

    if (eErrorNo != l_iReturn)
    {
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Scanning patterns & enumerate files
    l_pProcess->GetChildFirst(XML_NODE_OPTIONS_PROCESS_PATTERN, &l_pPattern);
    while (l_pPattern)
    {
        tXCHAR *l_pMask = NULL;
        l_pPattern->GetAttrText(XML_NARG_OPTIONS_PROCESS_MASK, &l_pMask);

        if (l_pMask)
        {
            l_cDir.Set(i_pArgV[DIR_SRC_INDEX]);
            CFSYS::Enumerate_Files(&l_cFiles, &l_cDir, l_pMask);
        }

        Cfg::INode *l_pNext = NULL;
        l_pPattern->GetNext(&l_pNext);
        l_pPattern->Release();
        l_pPattern = l_pNext;
    }
   
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Scanning functions
    if (!ParseFunctions(l_pOptions, l_cManager.GetFunctions()))
    {
        printf("Error: can't parse functions section\n");
        l_iReturn = eErrorXmlParsing;
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Add files to manager
    l_pEl = NULL;
    while ((l_pEl = l_cFiles.Get_Next(l_pEl)))
    {
        l_cManager.AddFile(l_cFiles.Get_Data(l_pEl)->Get());
    }

    if (Cfg::eOk == l_pOptions->GetChildFirst(XML_NODE_OPTIONS_DESC, &l_pDescrip))
    {
        l_cManager.AddFile(i_pArgV[CFG_FILE_INDEX], l_pDescrip);
        l_pDescrip->Release();
        l_pDescrip = NULL;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Scan saved files & hashes
    l_pRoot->GetChildFirst(XML_NODE_FILES, &l_pFiles);
    if (l_pFiles)
    {
        Cfg::INode *l_pFile = NULL;

        l_pFiles->GetChildFirst(XML_NODE_FILES_ITEM, &l_pFile);


        while (l_pFile)
        {
            tXCHAR *l_pFileName = NULL;
            tXCHAR *l_pFileHash = NULL;
            l_pFile->GetAttrText(XML_NARG_FILES_ITEM_RELATIVE_PATH, &l_pFileName);
            l_pFile->GetAttrText(XML_NARG_FILES_ITEM_HASH, &l_pFileHash);

            if (    (l_pFileName)
                 && (l_pFileHash)
               )
            {
                if (!l_cManager.CheckFileHash(l_pFileName, l_pFileHash))
                {
                    l_iReturn = eErrorMemAlloc;
                    break;
                }
            }

            l_pFile->Del();
            l_pFile->Release();
            l_pFile = NULL;
            l_pFiles->GetChildFirst(XML_NODE_FILES_ITEM, &l_pFile);
        }
    }

    if (eErrorNo != l_iReturn)
    {
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Scan read only files
    l_pOptions->GetChildFirst(XML_NODE_OPTIONS_READONLY, &l_pRoFiles);
    if (l_pRoFiles)
    {
        Cfg::INode *l_pFile = NULL;

        l_pRoFiles->GetChildFirst(XML_NODE_OPTIONS_READONLY_FILE, &l_pFile);


        while (l_pFile)
        {
            tXCHAR *l_pFileName = NULL;
            l_pFile->GetAttrText(XML_NARG_OPTIONS_READONLY_RALTIVE_PATH, &l_pFileName);

            if (l_pFileName)
            {
                if (!l_cManager.SetReadOnlyFile(l_pFileName))
                {
                    l_iReturn = eErrorMemAlloc;
                    break;
                }
            }

            Cfg::INode *l_pNext = NULL;
            l_pFile->GetNext(&l_pNext);
            l_pFile->Release();
            l_pFile = l_pNext;
        }

        SAFE_RELEASE(l_pRoFiles);
    }

    if (eErrorNo != l_iReturn)
    {
        goto l_lblExit;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Scan excluded files
    l_pOptions->GetChildFirst(XML_NODE_OPTIONS_EXCLUDED, &l_pRoFiles);
    if (l_pRoFiles)
    {
        Cfg::INode *l_pFile = NULL;

        l_pRoFiles->GetChildFirst(XML_NODE_OPTIONS_EXCLUDED_FILE, &l_pFile);


        while (l_pFile)
        {
            tXCHAR *l_pFileName = NULL;
            l_pFile->GetAttrText(XML_NARG_OPTIONS_EXCLUDED_RALTIVE_PATH, &l_pFileName);

            if (l_pFileName)
            {
                if (!l_cManager.SetExcludedFile(l_pFileName))
                {
                    l_iReturn = eErrorMemAlloc;
                    break;
                }
            }

            Cfg::INode *l_pNext = NULL;
            l_pFile->GetNext(&l_pNext);
            l_pFile->Release();
            l_pFile = l_pNext;
        }

        SAFE_RELEASE(l_pRoFiles);
    }

    if (eErrorNo != l_iReturn)
    {
        goto l_lblExit;
    }

    l_iReturn = l_cManager.Process();

    //save files & hashes
    if (eErrorNo == l_iReturn)
    {
        if (l_cManager.SaveHashes(l_pFiles))
        {
            if (Cfg::eOk != l_iDoc->Save())
            {
                printf("Error: can't save configuration file! But source files have been modified!\n");
                l_iReturn = eErrorFileWrite;
            }
        }
        else
        {
            printf("Error: can't save files hashes!\n");
            l_iReturn = eErrorFileWrite;
        }
    }

l_lblExit:
    SAFE_RELEASE(l_pCPU);
    SAFE_RELEASE(l_pPattern);
    SAFE_RELEASE(l_pProcess);
    SAFE_RELEASE(l_pFiles);
    SAFE_RELEASE(l_pRoFiles);
    SAFE_RELEASE(l_pOptions);
    SAFE_RELEASE(l_pRoot);
    SAFE_RELEASE(l_iDoc);

    l_cFiles.Clear(TRUE);

    return -l_iReturn;
}
