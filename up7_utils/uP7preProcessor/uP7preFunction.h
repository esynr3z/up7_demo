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
#ifndef UP7_FUNCTION_H
#define UP7_FUNCTION_H

#define UP7_FUNC_MAX_ARGUMENTS_COUNT 6
#define UP7_FUNC_UNDEFINED_INDEX     -1


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum eTraceArguments
{
    eTraceIdIndex = 0,
    eTraceFormatStringIndex,
    eTraceArgumentsMax
};
static_assert(eTraceArgumentsMax <= UP7_FUNC_MAX_ARGUMENTS_COUNT, "Arguments count error");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum eRegModArguments
{
    eRegModNameIndex = 0,
    eRegModLevelIndex,
    eRegModArgumentsMax
};
static_assert(eRegModArgumentsMax <= UP7_FUNC_MAX_ARGUMENTS_COUNT, "Arguments count error");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum eMkCounterArguments
{
    eMkCounterNameIndex = 0,
    eMkCounterMinIndex,
    eMkCounterAlarmMinIndex,
    eMkCounterMaxIndex,
    eMkCounterAlarmMaxIndex,
    eMkCounterOnIndex,
    eMkCounterArgumentsMax
};
static_assert(eMkCounterArgumentsMax <= UP7_FUNC_MAX_ARGUMENTS_COUNT, "Arguments count error");


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct stFuncDesc
{
    enum eType
    {
        eTrace,
        eRegisterModule,
        eCreateCounter
    };

    char  *pName;
    size_t szName;
    eType  eFtype;
    int    pArgs[UP7_FUNC_MAX_ARGUMENTS_COUNT];

    stFuncDesc(const tXCHAR* i_pName, stFuncDesc::eType i_eType)
    {
        size_t l_szName = (size_t)PStrLen(i_pName) + 1;
        szName = 0;
        pName  = (char*)malloc(l_szName);
        char* pIter = pName;
        if (pIter)
        {
            while (*i_pName)
            {
                //simple conversion from possibly UTF-16 to ansi char. For C/C++ source files & functions names we can afford it.
                *pIter = (char)*i_pName;
                pIter   ++;
                i_pName ++;
                szName  ++;
            }
            *pIter = 0;
        }

        eFtype = i_eType;

        for (size_t l_szI = 0; l_szI < UP7_FUNC_MAX_ARGUMENTS_COUNT; l_szI++)
        {
            pArgs[l_szI] = UP7_FUNC_UNDEFINED_INDEX;
        }
    }

    void InitDefaultArgs()
    {
        if (eFtype == eRegisterModule)
        {
            pArgs[eRegModNameIndex]  = 0;
            pArgs[eRegModLevelIndex] = 1;
        }
        else if (eFtype == eCreateCounter)
        {
            pArgs[eMkCounterNameIndex]     = 0;
            pArgs[eMkCounterMinIndex]      = 1;
            pArgs[eMkCounterAlarmMinIndex] = 2;
            pArgs[eMkCounterMaxIndex]      = 3;
            pArgs[eMkCounterAlarmMaxIndex] = 4;
            pArgs[eMkCounterOnIndex]       = 5;
        }
    }

    ~stFuncDesc()
    {
        if (pName)
        {
            free(pName);
            pName = NULL;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CBList<stFuncDesc*> CFunctionsList;


class CpreFile;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFuncRoot
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct stArg
    {
        const char *pRefStart;
        const char *pRefStop;
        char       *pNewVal;

        stArg(const char *i_pRefStart, const char *i_pRefStop)
        {
            pRefStart = i_pRefStart;
            pRefStop  = i_pRefStop;
            pNewVal   = NULL;

            while (    (    (' '  == *pRefStart)
                         || ('\t' == *pRefStart)
                         || ('\n' == *pRefStart)
                         || ('\r' == *pRefStart)
                       )
                    && (i_pRefStop >= i_pRefStart)
                  )
            {
                pRefStart++;
            }

            while (    (    (' '  == *pRefStop)
                         || ('\t' == *pRefStop)
                         || ('\n' == *pRefStop)
                         || ('\r' == *pRefStop)
                       )
                    && (i_pRefStop >= i_pRefStart)
                  )
            {
                pRefStop--;
            }
        }

        virtual ~stArg() { if (pNewVal) { free(pNewVal); }}
    };

protected:
    CpreFile      *m_pFile;  
    stFuncDesc    *m_pDesc;
    const int      m_iLine;
    char          *m_pFunction;
    eErrorCodes    m_eError;
    CBList<stArg*> m_cArgs;
    bool           m_bUpdated;

public:
    CFuncRoot(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pFunctionName);
    virtual ~CFuncRoot();
    eErrorCodes       GetError();
    int               GetLine();
    const char*       GetFilePath();
    const char*       GetFunction();
    bool              IsUpdated();
    CBList<stArg*>*   GetArgs() {return &m_cArgs;}
    stFuncDesc::eType GetType() {return m_pDesc->eFtype;}
    CpreFile*         GetFile() {return m_pFile;}
    virtual void     *GetBuffer(size_t &o_rSize) = 0;

    static char*      FindFunctionEnd(char *i_pStart, size_t i_szNameLen, int &o_rLines);

protected:
    char    *GetParameterStrValue(const stArg *i_pArg);
    char    *GetParameterRawValue(const stArg *i_pArg);
    uint32_t GetParameterUintValue(const stArg *i_pArg);
    double   GetParameterDoubleValue(const stArg *i_pArg);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFuncTrace
    : public CFuncRoot
{
    enum eWchar
    {
        eWchar16 = 16,
        eWchar32 = 32
    };

protected:
    uint16_t      m_uId;
    char         *m_pFormat;
    size_t        m_szBuffer;
    tUINT8       *m_pBuffer;
    sP7Trace_Arg *m_pVA;
    size_t        m_szVA;
    size_t        m_szTargetCpuBytes;
    eWchar        m_eWchar;

public:
    CFuncTrace(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pName);
    virtual ~CFuncTrace();
    void        SetId(uint16_t i_uId);
    uint16_t    GetId() {return m_uId;}
    void       *GetBuffer(size_t &o_rSize) { o_rSize = m_szBuffer; return m_pBuffer;}
    size_t      GetVA(const sP7Trace_Arg *&o_rVA) { o_rVA = m_pVA; return m_szVA;}
    const char *GetFormat() { return m_pFormat; }
private:
    void        ParseFormat();
    tUINT8      GetPlatformTypeSize(eTrace_Arg_Type i_eSize) const;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFuncModule
    : public CFuncRoot
{
protected:
    char           *m_pName;
    char           *m_pVerbosity;
    tUINT16         m_wId;
    tUINT32         m_uHash;
    sP7Trace_Module m_sModule;

public:
    CFuncModule(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pName);
    virtual ~CFuncModule();
    void        SetId(tUINT16 i_wId);
    tUINT16     GetId() {return m_wId;}
    void       *GetBuffer(size_t &o_rSize) { o_rSize = sizeof(m_sModule); return &m_sModule;}
    const char *GetName() { return m_pName;}
    const char *GetVerbosity() { return m_pVerbosity;}
    tUINT32     GetHash() { return m_uHash;}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFuncCounter
    : public CFuncRoot
{
protected:
    char              *m_pName;
    tUINT16            m_wId;
    tUINT32            m_uHash;
    sP7Tel_Counter_v2 *m_pCounter;
    size_t             m_szCounter;
    double             m_dbMin;
    double             m_dbMinAlarm;
    double             m_dbMax;
    double             m_dbMaxAlarm;

public:
    CFuncCounter(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pName);
    virtual ~CFuncCounter();
    void        SetId(tUINT16 i_wId);
    tUINT16     GetId()      { return m_wId;}
    const char* GetName()    { return m_pName; }
    tUINT32     GetHash()    { return m_uHash; }
    bool        GetEnabled() { return (bool)m_pCounter->bOn; }
    void*       GetBuffer(size_t &o_rSize) { o_rSize = m_szCounter; return m_pCounter;}

    bool        IsEqual(const CFuncCounter *i_pRef)
    { 
        return    (i_pRef->m_dbMin == m_dbMin)
               && (i_pRef->m_dbMax == m_dbMax)
               && (i_pRef->m_dbMinAlarm == m_dbMinAlarm)
               && (i_pRef->m_dbMaxAlarm == m_dbMaxAlarm);
    
    }
};


#endif //UP7_FUNCTION_H
