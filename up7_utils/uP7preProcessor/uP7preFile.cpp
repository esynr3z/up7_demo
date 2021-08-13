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

#define MAX_FILE_SIZE 0x2000000ull

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CpreFile::CpreFile(CpreManager *i_pManager, const tXCHAR *i_pName, eErrorCodes &o_rError)
    : m_pManager(i_pManager)
    , m_pOsPath(NormalizePath(i_pName))
    , m_pDbPath(NULL)
    , m_pData(NULL)
    , m_szData(0)
    , m_bModification(eModified)
    , m_eError(eErrorNo)
    , m_bUpdated(FALSE)
    , m_bIsVirtual(FALSE)
    , m_pVirtMod(NULL)
    , m_pVirtTel(NULL)
{
    memset(m_pHash, 0, sizeof(m_pHash));
    CPFile l_cFile;



#ifdef UTF8_ENCODING
    m_pDbPath = const_cast<char*>(m_pOsPath);    
#else
    size_t l_szPath = PStrLen(m_pOsPath) * 5;
    m_pDbPath = (char*)malloc(l_szPath);
    if (m_pDbPath)
    {
        int l_iCount = Convert_UTF16_To_UTF8((const tWCHAR *)m_pOsPath, 
                                              m_pDbPath, 
                                             (tUINT32)l_szPath
                                            );
        if (l_iCount < 0) 
        {
            m_pDbPath[0] = 0;
        }
    }
#endif                             

    if (l_cFile.Open(m_pOsPath,   IFile::EOPEN
                                | IFile::ESHARE_READ 
                                | IFile::EACCESS_READ 
                                | IFile::EACCESS_WRITE
                    )
        )
    {
        tUINT64 l_qwFileSize = l_cFile.Get_Size();
         
        if (l_qwFileSize < MAX_FILE_SIZE)
        {
            m_szData = (size_t)l_qwFileSize;
            m_pData  = (tUINT8*)malloc((size_t)m_szData + 1);
            if ((size_t)m_szData == l_cFile.Read(m_pData, (size_t)m_szData))
            {
                m_pData[(size_t)m_szData] = 0;

                CKeccak l_cHash;
                l_cHash.UpdateB(m_pData, (size_t)m_szData);
                l_cHash.Get_HashB(m_pHash, sizeof(m_pHash));

                Parse();
            }
            else
            {
                OSPRINT(TM("ERROR: Can't read file {%s}\n"), i_pName);
                m_eError = eErrorFileRead;
            }
        }
        else
        {
            OSPRINT(TM("ERROR: file {%s} is too big, 32MB max\n"), i_pName);
            m_eError = eErrorFileTooBig;
        }

        l_cFile.Close(FALSE);
    }
    else
    {
        OSPRINT(TM("ERROR: can't open file {%s}\n"), i_pName);
        m_eError = eErrorFileOpen;
    }

    o_rError = m_eError;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CpreFile::CpreFile(CpreManager *i_pManager, const tXCHAR *i_pName, Cfg::INode *i_pNode, eErrorCodes &o_rError)
    : m_pManager(i_pManager)
    , m_pOsPath(NormalizePath(i_pName))
    , m_pDbPath(NULL)
    , m_pData(NULL)
    , m_szData(0)
    , m_bModification(eReadOnly)
    , m_eError(eErrorNo)
    , m_bUpdated(FALSE)
    , m_bIsVirtual(TRUE)
    , m_pVirtMod(NULL)
    , m_pVirtTel(NULL)
{
    memset(m_pHash, 0, sizeof(m_pHash));

#ifdef UTF8_ENCODING
    m_pDbPath = const_cast<char*>(m_pOsPath);    
#else
    size_t l_szPath = PStrLen(m_pOsPath) * 5;
    m_pDbPath = (char*)malloc(l_szPath);
    if (m_pDbPath)
    {
        int l_iCount = Convert_UTF16_To_UTF8((const tWCHAR *)m_pOsPath, m_pDbPath, (tUINT32)l_szPath);
        if (l_iCount < 0) 
        {
            m_pDbPath[0] = 0;
        }
    }
#endif                

    m_pVirtMod = new stFuncDesc(TM("uP7TrcRegisterModule"), stFuncDesc::eRegisterModule);
    m_pVirtTel = new stFuncDesc(TM("uP7TelCreateCounter"), stFuncDesc::eCreateCounter);

    size_t l_szBuffer = 8192;
    char  *l_pBuffer  = (char*)malloc(l_szBuffer * sizeof(char));

    m_pVirtMod->InitDefaultArgs();
    m_pVirtTel->InitDefaultArgs();

    if (l_pBuffer)
    {
        Cfg::INode *l_pModule = NULL;

        i_pNode->GetChildFirst(XML_NODE_OPTIONS_DESC_MODULE, &l_pModule);

        while (l_pModule)
        {
            tXCHAR *l_pName = NULL;
            tXCHAR *l_pVerb = NULL;
            l_pModule->GetAttrText(XML_NODE_OPTIONS_DESC_MODULE_NAME, &l_pName);
            l_pModule->GetAttrText(XML_NODE_OPTIONS_DESC_MODULE_VERB, &l_pVerb);

            if (    (l_pName)
                 && (l_pVerb)
               )
            {
                CUtf8 l_cU8Name(l_pName);
                CUtf8 l_cU8Verb(l_pVerb);

                sprintf(l_pBuffer, "%s(\"%s\", %s, stub)", m_pVirtMod->pName, l_cU8Name.text(), l_cU8Verb.text());

                m_cFunctions.Push_Last(new CFuncModule(this, m_pVirtMod, l_pBuffer, l_pBuffer+strlen(l_pBuffer) - 1, 0, "Static"));
            }

            Cfg::INode *l_pNext = NULL;
            l_pModule->GetNext(XML_NODE_OPTIONS_DESC_MODULE, &l_pNext);
            l_pModule->Release();
            l_pModule = l_pNext;
        }

        Cfg::INode *l_pCounter = NULL;

        i_pNode->GetChildFirst(XML_NODE_OPTIONS_DESC_COUNTER, &l_pCounter);

        while (l_pCounter)
        {
            tXCHAR *l_pName     = NULL;
            tXCHAR *l_pMin      = NULL;
            tXCHAR *l_pAlarmMin = NULL;
            tXCHAR *l_pMax      = NULL;
            tXCHAR *l_pAlarmMax = NULL;
            tXCHAR *l_pEnabled  = NULL;
            l_pCounter->GetAttrText(XML_NODE_OPTIONS_DESC_COUNTER_NAME,      &l_pName);
            l_pCounter->GetAttrText(XML_NODE_OPTIONS_DESC_COUNTER_MIN,       &l_pMin);
            l_pCounter->GetAttrText(XML_NODE_OPTIONS_DESC_COUNTER_ALARM_MIN, &l_pAlarmMin);
            l_pCounter->GetAttrText(XML_NODE_OPTIONS_DESC_COUNTER_MAX,       &l_pMax);
            l_pCounter->GetAttrText(XML_NODE_OPTIONS_DESC_COUNTER_ALARM_MAX, &l_pAlarmMax);
            l_pCounter->GetAttrText(XML_NODE_OPTIONS_DESC_COUNTER_ENABLED,   &l_pEnabled);

            if (    (l_pName)
                 && (l_pMin)
                 && (l_pAlarmMin)
                 && (l_pMax)
                 && (l_pAlarmMax)
                 && (l_pEnabled)
               )
            {
                CUtf8 l_cName    (l_pName    );
                CUtf8 l_cMin     (l_pMin     );
                CUtf8 l_cAlarmMin(l_pAlarmMin);
                CUtf8 l_cMax     (l_pMax     );
                CUtf8 l_cAlarmMax(l_pAlarmMax);
                CUtf8 l_cEnabled (l_pEnabled );

                sprintf(l_pBuffer, "%s(\"%s\", %s, %s, %s, %s, %s, stub)", 
                        m_pVirtTel->pName, 
                        l_cName.text(), 
                        l_cMin.text(), 
                        l_cAlarmMin.text(), 
                        l_cMax.text(), 
                        l_cAlarmMax.text(), 
                        l_cEnabled.text());

                m_cFunctions.Push_Last(new CFuncCounter(this, m_pVirtTel, l_pBuffer, l_pBuffer + strlen(l_pBuffer) - 1, 0, "Static"));
            }

            Cfg::INode *l_pNext = NULL;
            l_pCounter->GetNext(XML_NODE_OPTIONS_DESC_COUNTER, &l_pNext);
            l_pCounter->Release();
            l_pCounter = l_pNext;
        }

        free(l_pBuffer);
    }
    else
    {
        OSPRINT(TM("ERROR: Can't allocate memory {%s}\n"), i_pName);
        m_eError = eErrorMemAlloc;
    }

    o_rError = m_eError;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CpreFile::~CpreFile()
{
    if (    ((void*)m_pDbPath != (void*)m_pOsPath)
         && (m_pDbPath)
       )
    {
        free(m_pDbPath);
        m_pDbPath = NULL;
    }

    if (m_pOsPath)
    {
        PStrFreeDub(m_pOsPath);
        m_pOsPath = NULL;
    }


    if (m_pData)
    {
        free(m_pData);
        m_pData = NULL;
    }

    if (m_pVirtMod)
    {
        delete m_pVirtMod;
        m_pVirtMod = NULL;
    }

    if (m_pVirtTel)
    {
        delete m_pVirtTel;
        m_pVirtTel = NULL;
    }

    m_cFunctions.Clear(TRUE);
    m_cBlocks.Clear(TRUE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tBOOL CpreFile::Save()
{
    if (eErrorNo != m_eError)
    {
        return FALSE;
    }

    if (m_bIsVirtual)
    {
        return TRUE;
    }

    if (!m_bUpdated)
    {
        return TRUE;
    }

    if (eReadOnly == m_bModification)
    {
        OSPRINT(TM("WARNING: Read-only file has been modified {%s}\n"), m_pOsPath);
    }

    size_t      l_szDst     = m_szData + 4096;
    tUINT8     *l_pDst      = (tUINT8*)malloc(l_szDst);
    const char *l_pSrcStart = (const char*)m_pData;
    size_t      l_szOffs    = 0;
    if (!l_pDst)
    {
        m_eError = eErrorMemAlloc;
        return FALSE;
    }


    pAList_Cell l_pItemEl = NULL;
    while (    ((l_pItemEl = m_cFunctions.Get_Next(l_pItemEl)))
            && (eErrorNo == m_eError)
          )
    {
        CFuncRoot *l_pFunc = m_cFunctions.Get_Data(l_pItemEl);
        if (l_pFunc->IsUpdated())
        {
            CBList<CFuncRoot::stArg*>* l_pArgs = l_pFunc->GetArgs();
            pAList_Cell l_pArgEl = NULL;
            while ((l_pArgEl = l_pArgs->Get_Next(l_pArgEl)))
            {
                CFuncRoot::stArg* l_pArg = l_pArgs->Get_Data(l_pArgEl);
                if (    (l_pArg)
                     && (l_pArg->pNewVal)
                   )
                {
                    size_t l_szNewval = strlen(l_pArg->pNewVal);
                    if (l_pArg->pRefStart > l_pSrcStart)
                    {
                        size_t l_szNewBlock = l_pArg->pRefStart - l_pSrcStart;

                        if ((l_szDst - l_szOffs) < (l_szNewBlock + l_szNewval))
                        {
                            size_t  l_szNewDst = l_szDst + l_szNewBlock + l_szNewval + 4096;
                            tUINT8 *l_pNewDst  = (tUINT8*)realloc(l_pDst, l_szNewDst);
                            if (l_pNewDst)
                            {
                                l_pDst  = l_pNewDst;
                                l_szDst = l_szNewDst;
                            }
                            else
                            {
                                m_eError = eErrorMemAlloc;
                                break;
                            }
                        }

                        memcpy(l_pDst + l_szOffs, l_pSrcStart, l_szNewBlock);
                        l_szOffs += l_szNewBlock;
                        memcpy(l_pDst + l_szOffs, l_pArg->pNewVal, l_szNewval);
                        l_szOffs += l_szNewval;
                        l_pSrcStart = l_pArg->pRefStop + 1;
                    }
                }
            }
        }
    }


    if (eErrorNo == m_eError)
    {
        if (l_pSrcStart < ((const char*)m_pData + m_szData))
        {
            size_t l_szNewBlock = (const char*)m_pData + m_szData - l_pSrcStart;

            if ((l_szDst - l_szOffs) < l_szNewBlock)
            {
                size_t  l_szNewDst = l_szDst + l_szNewBlock;
                tUINT8 *l_pNewDst  = (tUINT8*)realloc(l_pDst, l_szNewDst);
                if (l_pNewDst)
                {
                    l_pDst  = l_pNewDst;
                    l_szDst = l_szNewDst;
                }
                else
                {
                    m_eError = eErrorMemAlloc;
                }
            }

            if (eErrorNo == m_eError)
            {
                memcpy(l_pDst + l_szOffs, l_pSrcStart, l_szNewBlock);
                l_szOffs += l_szNewBlock;
                l_pSrcStart = NULL;
            }
        }
    }


    if (eErrorNo == m_eError)
    {
        CPFile l_cFile;
        if (l_cFile.Open(m_pOsPath,   IFile::ECREATE
                                    | IFile::ESHARE_READ 
                                    | IFile::EACCESS_WRITE
                        )
            )
        {
            if (l_szOffs == l_cFile.Write(l_pDst, l_szOffs, TRUE))
            {
                CKeccak l_cHash;
                l_cHash.UpdateB(l_pDst, (size_t)l_szOffs);
                l_cHash.Get_HashB(m_pHash, sizeof(m_pHash));
            }
            else
            {
                OSPRINT(TM("ERROR: Can't re-write file {%s}\n"), m_pOsPath);
                m_eError = eErrorFileWrite;
            }
        }
        else
        {
            OSPRINT(TM("ERROR: can't open file {%s}\n"), m_pOsPath);
            m_eError = eErrorFileOpen;
        }
    }


    free(l_pDst);

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
eErrorCodes CpreFile::GetError()
{
    if (eErrorNo != m_eError)
    {
        return m_eError;
    }

    //cleanup & retrieving error if any
    pAList_Cell l_pEl = NULL;
    while ((l_pEl = m_cFunctions.Get_Next(l_pEl)))
    {
        CFuncRoot  *l_pFunc  = m_cFunctions.Get_Data(l_pEl);
        eErrorCodes l_eError = l_pFunc->GetError();
        if (eErrorNo != l_eError)
        {
            return l_eError;
        }
    }

    return eErrorNo;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CpreFile::PrintErrors()
{
    OSPRINT(TM("ERROR(s): at file {%s}\n"), m_pOsPath);

    if (eErrorNo != m_eError)
    {
        printf(" * %s\n", g_pErrorsText[m_eError]);
    }

    pAList_Cell l_pEl = NULL;
    while ((l_pEl = m_cFunctions.Get_Next(l_pEl)))
    {
        CFuncRoot  *l_pFunc  = m_cFunctions.Get_Data(l_pEl);
        eErrorCodes l_eError = l_pFunc->GetError();
        if (eErrorNo != l_eError)
        {
            printf(" * Line:%d, %s\n", l_pFunc->GetLine(), g_pErrorsText[l_eError]);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tBOOL CpreFile::Parse()
{
    enum class eComment
    {
        eNone,
        eLine,
        eMultiLine
    };

    tBOOL              l_bReturn    = TRUE;
    int                l_iLine      = 1;
    char              *l_pIt        = (char*)m_pData;    
    eComment           l_eComment   = eComment::eNone;
    CFunctionsList    &l_rFunctions = m_pManager->GetFunctions();
    pAList_Cell        l_pEl        = NULL;
    const char        *l_pKeyWords[]= {"class", "enum", "namespace", "struct"};

    while (*l_pIt)
    {
        if (0xA == *l_pIt)
        {
            if (eComment::eLine == l_eComment)
            {
                l_eComment = eComment::eNone;
            }

            l_iLine++;
        }
        else if ('/' == *l_pIt)     
        {
            if (    (eComment::eMultiLine == l_eComment)
                 && ((char*)m_pData != l_pIt) //not the beginning of the file
                 && ('*' == *(l_pIt-1)) //prev char was *
               )
            {
                l_eComment = eComment::eNone;
            }
            else if (    (eComment::eNone == l_eComment)
                      && ('/' == *(l_pIt+1)) 
                    )
            {
                l_eComment = eComment::eLine;
            }
            else if (    (eComment::eNone == l_eComment)
                      && ('*' == *(l_pIt+1)) 
                    )
            {
                l_eComment = eComment::eMultiLine;
            }
        }
        else if (eComment::eNone == l_eComment)
        {
            if ('\"' == *l_pIt)     
            {
                l_pIt = GetStringEnd(l_pIt);
                if (!l_pIt)
                {
                    OSPRINT(TM("ERROR: file {%s}:%d Can't find end of the string\n"), m_pOsPath, l_iLine);
                    m_eError = eErrorFileRead;
                }
            }
            else if ('{' == *l_pIt)     
            {
                pAList_Cell l_pEl = m_cBlocks.Get_Last();
                if (l_pEl)
                {
                    stBlock *l_pBlock = m_cBlocks.Get_Data(l_pEl);
                    if (l_pBlock)
                    {
                        l_pBlock->iBrakets ++;
                    }
                }
            }
            else if ('}' == *l_pIt)     
            {
                pAList_Cell l_pEl = m_cBlocks.Get_Last();
                if (l_pEl)
                {
                    stBlock *l_pBlock = m_cBlocks.Get_Data(l_pEl);
                    if (l_pBlock)
                    {
                        l_pBlock->iBrakets --;
                        if (!l_pBlock->iBrakets)
                        {
                            m_cBlocks.Del(l_pEl, TRUE);
                        }
                    }
                }
            }
            else if ('(' == *l_pIt) //probably start of the function ....    
            {
                stBlock *l_pBlock = m_cBlocks.Get_Data(m_cBlocks.Get_Last());
                if (    (    (l_pBlock)
                          && (!l_pBlock->bFunction)
                        )
                     || (!l_pBlock)
                   )
                {
                    size_t      l_szLen     = 0;
                    const char *l_pFuncName = GetFunctionName((const char*)m_pData, l_pIt, l_szLen);
                    m_cBlocks.Push_Last(new stBlock(TRUE, l_pFuncName, l_szLen));
                }
            }
            else if (';' == *l_pIt) //probably end of the function declaration
            {
                stBlock *l_pBlock = m_cBlocks.Get_Data(m_cBlocks.Get_Last());
                if (    (l_pBlock)
                     && (l_pBlock->bFunction)
                     && (!l_pBlock->iBrakets)
                   )
                {
                    m_cBlocks.Del(m_cBlocks.Get_Last(), TRUE);
                }
            }
            else if (IsKeyWord((char*)m_pData, l_pIt, l_pKeyWords, LENGTH(l_pKeyWords))) //probably start of the function ....    
            {
                m_cBlocks.Push_Last(new stBlock(FALSE, NULL, 0));
            }
            else
            {
                l_pEl = NULL;
                while ((l_pEl = l_rFunctions.Get_Next(l_pEl)))
                {
                    stFuncDesc *l_pFunc = l_rFunctions.Get_Data(l_pEl);
                    if (0 == strncmp(l_pIt, l_pFunc->pName, l_pFunc->szName))
                    {                                               
                        char *l_pStart   = l_pIt;
                        int   l_iCurLine = l_iLine;


                        l_pIt = CFuncRoot::FindFunctionEnd(l_pIt, l_pFunc->szName, l_iLine);

                        if (l_pIt)
                        {
                            stBlock *l_pBlock = m_cBlocks.Get_Data(m_cBlocks.Get_Last());
                            const char *l_pFuncionName = "Unknown";
                            
                            if ((l_pBlock) && (l_pBlock->bFunction))
                            {
                                l_pFuncionName = l_pBlock->pName;
                            }

                            if (stFuncDesc::eTrace == l_pFunc->eFtype)
                            {
                                m_cFunctions.Push_Last(new CFuncTrace(this, l_pFunc, l_pStart, l_pIt, l_iCurLine, l_pFuncionName));
                            }
                            else if (stFuncDesc::eRegisterModule == l_pFunc->eFtype)
                            {
                                m_cFunctions.Push_Last(new CFuncModule(this, l_pFunc, l_pStart, l_pIt, l_iCurLine, l_pFuncionName));
                            }
                            else if (stFuncDesc::eCreateCounter == l_pFunc->eFtype)
                            {
                                m_cFunctions.Push_Last(new CFuncCounter(this, l_pFunc, l_pStart, l_pIt, l_iCurLine, l_pFuncionName));
                            }
                            else
                            {
                                OSPRINT(TM("ERROR: file {%s}:%d Unknown function\n"), m_pOsPath, l_iCurLine);
                                m_eError = eErrorFileOpen;
                            }
                        }
                        else
                        {
                            OSPRINT(TM("ERROR: file {%s}:%d Can't parse function arguments\n"), m_pOsPath, l_iCurLine);
                            m_eError = eErrorNotFunction;
                        }

 
                        break;
                    }
                }
            }
        }

        l_pIt++;
    }


    //cleanup & retrieving error if any
    l_pEl = NULL;
    while ((l_pEl = m_cFunctions.Get_Next(l_pEl)))
    {
        CFuncRoot  *l_pFunc  = m_cFunctions.Get_Data(l_pEl);
        eErrorCodes l_eError = l_pFunc->GetError();
        if (eErrorNotFunction == l_eError)
        {
            pAList_Cell l_pElPrev = m_cFunctions.Get_Prev(l_pEl);
            m_cFunctions.Del(l_pEl, TRUE);
            l_pEl = l_pElPrev;
        }
    }


    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tBOOL CpreFile::IsKeyWord(const char* i_pHead, const char *i_pCurPos, const char **i_ppKeywords, size_t i_szKeywords)
{
    if (    (!i_pHead)
         || (!i_pCurPos)
         || (!i_ppKeywords)
         || (!i_szKeywords)
       )
    {
        return FALSE;
    }

    tBOOL l_bReturn = FALSE;

    for (size_t l_szI = 0; l_szI < i_szKeywords; l_szI++)
    {
        if (0 == strcmp(i_pCurPos, i_ppKeywords[l_szI]))
        {
            if (    (    (i_pHead == i_pCurPos)
                      || (' ' == (*(i_pCurPos-1)))
                      || ('\t' == (*(i_pCurPos-1)))
                      || ('\n' == (*(i_pCurPos-1)))
                      || ('\r' == (*(i_pCurPos-1)))
                    )
                 && (    (' '  == (*(i_pCurPos+1)))
                      || ('\t' == (*(i_pCurPos+1)))
                      || ('\n' == (*(i_pCurPos+1)))
                      || ('\r' == (*(i_pCurPos+1)))
                      || ('{' == (*(i_pCurPos+1)))
                    )
               )

            {
                l_bReturn = TRUE;
            }

            break;
        }
    }

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* CpreFile::GetFunctionName(const char* i_pHead, const char *i_pCurPos, size_t &o_rLength)
{
    static const char *l_pNameAlphabet  = "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    i_pCurPos--;

    while (i_pHead < i_pCurPos)
    {
        if (    (' '  == *i_pCurPos)
             || ('\t' == *i_pCurPos)
             || ('\n' == *i_pCurPos)
             || ('\r' == *i_pCurPos)
           )
        {
            i_pCurPos --;
        }
        else
        {
            break;
        }
    }

    o_rLength = 0;
    while (    (i_pHead < i_pCurPos)
            && (NULL != strchr(l_pNameAlphabet, *i_pCurPos))
          )
    {
        i_pCurPos --;
        o_rLength ++;
    }

    if (i_pHead != i_pCurPos)
    {
        i_pCurPos++;
    }


    return i_pCurPos;
}
