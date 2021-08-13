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

#include <new>
#include "pugixml.hpp"
using namespace pugi;

#include <cassert>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////CBNodeMem/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBNodeMem
{
    struct stNode
    {
        stNode  *pNext; 
    };

    struct stPack
    {
        tUINT8 *pData;
        stPack *pNext;
    };

protected:
    stNode  *m_pNodes;
    size_t   m_lInUse;
    stPack  *m_pPacks;
    size_t   m_szNode;
    size_t   m_szPack;
    size_t   m_szInPack;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CBNodeMem(size_t i_szNode, size_t i_szInPack = 32)
        : m_pNodes(NULL)
        , m_lInUse(0)
        , m_pPacks(NULL)
        , m_szNode(i_szNode)
        , m_szPack(i_szInPack * i_szNode)
        , m_szInPack(i_szInPack)
    {
        if (i_szNode < sizeof(stNode))
        {
            m_szNode = sizeof(stNode);
            m_szPack = i_szInPack * m_szNode;
        }
    }

    ~CBNodeMem()
    {
        if (m_lInUse)
        {
        #if defined(_WIN32) || defined(_WIN64)
            wchar_t l_pError[64];
            swprintf_s(l_pError, LENGTH(l_pError), L"*** ERROR: %zu Nodes leaked", m_lInUse);
            OutputDebugStringW(l_pError);
        #elif defined(__linux__)
            printf("*** ERROR: %d Nodes leaked\n", (int)m_lInUse);
        #endif
        }

        while (m_pPacks)
        {
            stPack *l_pRelease = m_pPacks;
            m_pPacks = m_pPacks->pNext;
            free(l_pRelease);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void *Pull()
    {
        stNode *l_pReturn = NULL;

        if (!m_pNodes)
        {
            stPack *l_pNew = (stPack*)malloc(sizeof(stPack) + m_szPack);
            if (!l_pNew)
            {
                return NULL;
            }

            l_pNew->pNext = m_pPacks;
            l_pNew->pData = ((tUINT8*)l_pNew) + sizeof(stPack);
            m_pPacks = l_pNew;

            tUINT8 *l_pIter = l_pNew->pData;
            for (size_t l_szI = 0; l_szI < m_szInPack; l_szI++)
            {
                l_pReturn = (stNode *)l_pIter;
                l_pReturn->pNext = m_pNodes;
                m_pNodes = l_pReturn;

                l_pIter += m_szNode;
            }

        }

        l_pReturn = m_pNodes;
        m_pNodes = m_pNodes->pNext;
        m_lInUse ++;
        return l_pReturn;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Push(void * i_pData)
    {
        if (NULL == i_pData)
        {
            return;
        }

        assert(0 != m_lInUse);
        m_lInUse--;

        stNode *l_pItem = (stNode*)i_pData;
        l_pItem->pNext  = m_pNodes;
        m_pNodes        = l_pItem;
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////CBRoot///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBRoot
{
protected:
    tBOOL       m_bInitialized;
    CBNodeMem  *m_pMem;
    CLock      *m_pCS;
public:
    CBRoot(CBNodeMem *i_pMem, CLock *i_pCS);

    Cfg::INode  *GetChildFirst(pugi::xml_node *i_pXmlNode);
    Cfg::INode  *GetChildFirst(const tXCHAR *i_pName, pugi::xml_node *i_pXmlNode);
    Cfg::INode  *AddChildEmpty(const tXCHAR * i_pName, pugi::xml_node *i_pXmlNode);
    Cfg::eResult  DelChild(pugi::xml_node *i_pXmlNode, Cfg::INode *i_pNode);

protected:
    virtual ~CBRoot() {}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////CBNode///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBNode
    : public Cfg::INode
    , public CBRoot
    , public pugi::xml_writer
{
    tINT32 volatile m_lRefCounter;
    pugi::xml_node  m_cXmlNode;
    tUINT8         *m_pXmlText;
    size_t          m_szXmlTextMax;
    size_t          m_szXmlText;
    tBOOL           m_bXmlWriteError;
public:
    CBNode(CBNodeMem     *i_pFMC_Pool, 
           pugi::xml_node i_cXmlNode,
           CLock         *i_pCS
          );
    
    tINT32  Add_Ref()
    {
        return ATOMIC_INC(&m_lRefCounter); 
    }

    tINT32  Release()
    {
        tINT32 l_lResult = ATOMIC_DEC(&m_lRefCounter);
        // clean up internal object memory ...
        if (0 >= l_lResult)
        {
            //release object memory
            if (m_pMem)
            {
                m_pMem->Push(this);
            }
        }

        return l_lResult;
    }

    //pugi::xml_writer
    void write(const void* data, size_t size);


    Cfg::eResult  GetName(tXCHAR **o_pName);
    Cfg::eResult  SetName(const tXCHAR *i_pName);

    Cfg::eResult  Copy(Cfg::INode *i_pSource);

    Cfg::eResult  GetAttr(tUINT32 i_dwIndex, const tXCHAR *&o_rName);

    Cfg::eResult  DelAttr(const tXCHAR *i_pName);

    Cfg::eResult  GetAttrInt32(const tXCHAR *i_pName, tINT32 *o_pValue);

    Cfg::eResult  GetAttrText(const tXCHAR  *i_pName, tXCHAR **o_pValue);

    Cfg::eResult  SetAttrInt32(const tXCHAR *i_pName, tINT32 i_lValue);
    Cfg::eResult  SetAttrText(const tXCHAR  *i_pName, const tXCHAR *i_pValue);

    Cfg::eResult  GetNext(Cfg::INode **o_pNode);
    Cfg::eResult  GetNext(const tXCHAR *i_pName, Cfg::INode **o_pNode);

    Cfg::eResult  GetChildFirst(Cfg::INode **o_pNode);
    Cfg::eResult  GetChildFirst(const tXCHAR *i_pName, Cfg::INode **o_pNode);
    Cfg::eResult  AddChildEmpty(const tXCHAR  *i_pName, Cfg::INode  **o_pNode);
    Cfg::eResult  DelChild(Cfg::INode *i_pNode);
    Cfg::eResult  Del();
    Cfg::eResult  GetXml(const void *&o_rData, size_t &o_rSize);


    pugi::xml_node Get_XmlNode() { return m_cXmlNode; }

protected:
    virtual ~CBNode();
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////CBDoc///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CBDoc
    : public CBRoot
    , public Cfg::IDoc
{
private:
    pugi::xml_document m_cXmlDoc;
    CWString           m_cFileName;
public:
    CBDoc(const tXCHAR *i_pFileName);
    CBDoc(const void *i_pBuffer, size_t i_szBuffer);
    ~CBDoc();

    tBOOL Get_Initialized() 
    { 
        return m_bInitialized; 
    }

    Cfg::eResult Save();
    Cfg::eResult Save_As(const tXCHAR *i_pFile_Name);

    Cfg::eResult GetChildFirst(Cfg::INode ** o_pNode);
    Cfg::eResult GetChildFirst(const tXCHAR * i_pName, Cfg::INode ** o_pNode);
    Cfg::eResult AddChildEmpty(const tXCHAR *i_pName, Cfg::INode ** o_pNode);
    Cfg::eResult DelChild(Cfg::INode *i_pNode);

    void Release()
    {
        delete this;
    }
};

