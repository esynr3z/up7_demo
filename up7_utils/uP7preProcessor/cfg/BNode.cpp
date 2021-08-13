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
#include "BNode.h"

extern "C" 
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
P7_EXPORT Cfg::IDoc* __cdecl IBDoc_Load(const tXCHAR *i_pFile_Name)
{
    CBDoc *l_pResult = new CBDoc(i_pFile_Name);

    if (    (l_pResult)
         && (!l_pResult->Get_Initialized())
       )
    {
        delete l_pResult;
        l_pResult = NULL;
    }

    return l_pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
P7_EXPORT Cfg::IDoc* __cdecl IBDoc_Load_Buffer(const void *i_pBuffer, size_t i_szBuffer)
{
    CBDoc *l_pResult = new CBDoc(i_pBuffer, i_szBuffer);

    if (    (l_pResult)
         && (!l_pResult->Get_Initialized())
       )
    {
        delete l_pResult;
        l_pResult = NULL;
    }

    return l_pResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
P7_EXPORT Cfg::IDoc* __cdecl IBDoc_Create()
{
    CBDoc *l_pResult = new CBDoc(NULL);

    if (    (l_pResult)
         && (!l_pResult->Get_Initialized())
       )
    {
        delete l_pResult;
        l_pResult = NULL;
    }

    return l_pResult;
}
}//extern "C"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////CBRoot///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBRoot::CBRoot(CBNodeMem *i_pMem, CLock *i_pCS)
    : m_bInitialized(TRUE)
    , m_pMem(i_pMem)
    , m_pCS(i_pCS)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::INode * CBRoot::GetChildFirst(pugi::xml_node *i_pXmlNode)
{
    CBNode        *l_pReturn = NULL;
    CLock          l_cLock(m_pCS);
    pugi::xml_node l_cXmlNode = *i_pXmlNode;

    if (NULL == i_pXmlNode)
    {
        return l_pReturn;
    }


    while (1)
    {
        if (true == l_cXmlNode.empty())
        {
            break;
        }
        else if (pugi::node_element == l_cXmlNode.type())
        {
            void *l_pNew = m_pMem->Pull();
            if (l_pNew)
            {
                l_pReturn = new (l_pNew) CBNode(m_pMem, l_cXmlNode, m_pCS);
            }
            break;
        }
        else
        {
            l_cXmlNode = l_cXmlNode.next_sibling();
        }
    }

    return static_cast<Cfg::INode*>(l_pReturn);
}//GetChildFirst


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::INode * CBRoot::GetChildFirst(const tXCHAR *i_pName, pugi::xml_node *i_pXmlNode)
{
    CBNode        *l_pReturn  = NULL;
    pugi::xml_node l_cXmlNode = *i_pXmlNode;
    CLock          l_cLock(m_pCS);

    if (    (!i_pXmlNode)
         || (!i_pName) 
       )
    {
        return l_pReturn;
    }

    while (1)
    {
        if (true == l_cXmlNode.empty())
        {
            break;
        }
        else if (    (pugi::node_element == l_cXmlNode.type())
                  && (0 == PStrICmp(i_pName, l_cXmlNode.name()))
                )
        {
            void *l_pNew = m_pMem->Pull();
            if (l_pNew)
            {
                l_pReturn = new (l_pNew) CBNode(m_pMem, l_cXmlNode, m_pCS);
            }
            break;
        }
        else
        {
            l_cXmlNode = l_cXmlNode.next_sibling();
        }
    }

    return static_cast<Cfg::INode*>(l_pReturn);
}//GetChildFirst


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::INode *CBRoot::AddChildEmpty(const tXCHAR * i_pName, pugi::xml_node *i_pXmlNode)
{
    CBNode *l_pReturn = NULL;
    CLock   l_cLock(m_pCS);

    if (    (!i_pXmlNode)
         || (!i_pName)
       )
    {
        return l_pReturn;
    }

    pugi::xml_node l_cXmlNode = i_pXmlNode->append_child(i_pName);
    if (false == l_cXmlNode.empty())
    {
        void *l_pNew = m_pMem->Pull();
        if (l_pNew)
        {
            l_pReturn = new (l_pNew) CBNode(m_pMem, l_cXmlNode, m_pCS);
        }
    }

    return static_cast<CBNode*>(l_pReturn);
}//AddChildEmpty


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBRoot::DelChild(pugi::xml_node *i_pXmlParent, Cfg::INode *i_pNodeChild)
{
    CLock l_cLock(m_pCS);

    if (    (!i_pXmlParent) 
         || (!i_pNodeChild) 
       )
    {
        return Cfg::eErrorInternal;
    }

    return (true == i_pXmlParent->remove_child(static_cast<CBNode*>(i_pNodeChild)->Get_XmlNode()) 
           ) ? Cfg::eOk : Cfg::eErrorInternal;
}//DelChild

            

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////CBRoot///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBNode::CBNode(CBNodeMem *i_pMem, pugi::xml_node i_cXmlNode, CLock *i_pCS)
    : CBRoot(i_pMem, i_pCS)
    , m_lRefCounter(1)
    , m_cXmlNode(i_cXmlNode)
    , m_pXmlText(NULL)
    , m_szXmlTextMax(0)
    , m_szXmlText(0)
    , m_bXmlWriteError(FALSE)
{
    CLock l_cLock(m_pCS);

    if (    (!m_pMem)
         || (m_cXmlNode.empty()) 
       )
    {
        m_bInitialized = FALSE;
    }
}//CBNode


 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBNode::write(const void* data, size_t size)
{
    CLock l_cLock(m_pCS);

    if (m_bXmlWriteError)
    {
        return;
    }

    if ((m_szXmlText + size) > m_szXmlTextMax)
    {
        size_t l_szNew    = (m_szXmlTextMax + size + 8191) & ~8191u;
        void  *l_pNewData = realloc(m_pXmlText, l_szNew);
        if (!l_pNewData) 
        {
            m_bXmlWriteError = TRUE;
            return;
        }

        m_szXmlTextMax = l_szNew;
        m_pXmlText     = (tUINT8*)l_pNewData;
    }

    memcpy(m_pXmlText + m_szXmlText, data, size);
    m_szXmlText += size;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetXml(const void *&o_rData, size_t &o_rSize)
{
    CLock l_cLock(m_pCS);
    m_szXmlText = 0;
    m_bXmlWriteError = FALSE;
    m_cXmlNode.print(*this);
    if (m_bXmlWriteError)
    {
        return Cfg::eErrorNoBuffer;
    }

    o_rData = m_pXmlText;
    o_rSize = m_szXmlText;

    return Cfg::eOk;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetName(tXCHAR **o_pName)
{
    CLock l_cLock(m_pCS);

    if (    (!m_bInitialized) 
         || (!o_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    *o_pName = (tXCHAR*)m_cXmlNode.name();

    return Cfg::eOk;
}//GetName


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::SetName(const tXCHAR *i_pName)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized)
         || (!i_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    return ( true == m_cXmlNode.set_name(i_pName) ) ? Cfg::eOk : Cfg::eErrorInternal;
}//SetName


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::Copy(Cfg::INode *i_pSource)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized) 
         || (!i_pSource) 
       )
    {
        return Cfg::eErrorInternal;
    }

    CBNode        *l_pSource  = static_cast<CBNode*>(i_pSource);
    pugi::xml_node l_cXmlNode = m_cXmlNode.append_copy(l_pSource->m_cXmlNode);

    return (l_cXmlNode.empty()) ? Cfg::eErrorInternal : Cfg::eOk;
}//Copy
    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetAttr(tUINT32 i_dwIndex, const tXCHAR *&o_rName)
{
    CLock       l_cLock(m_pCS);
    Cfg::eResult l_eReturn = Cfg::eErrorInternal;

    o_rName = NULL;

    if (!m_bInitialized)
    {
        return l_eReturn;
    }

    pugi::xml_attribute l_pXmlAttr = m_cXmlNode.first_attribute();

    while (1)
    {
        if ( l_pXmlAttr.empty() )
        {
            break;
        }
        else if (!i_dwIndex)
        {
            o_rName   = (const tXCHAR*)l_pXmlAttr.name();
            l_eReturn = Cfg::eOk;
            break;
        }
        else
        {
            l_pXmlAttr = l_pXmlAttr.next_attribute();
        }

        i_dwIndex--;
    }

    return l_eReturn;
}//GetAttr


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::DelAttr(const tXCHAR *i_pName)
{
    CLock l_cLock(m_pCS);

    if (    (!m_bInitialized)
         || (!i_pName) 
         || (!m_cXmlNode.remove_attribute(i_pName))
       )
    {
        return Cfg::eErrorInternal;
    }

    return Cfg::eOk;
}//DelAttr

       
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetAttrInt32(const tXCHAR *i_pName, tINT32 *o_pValue)
{
    CLock               l_cLock(m_pCS);
    Cfg::eResult         l_eReturn  = Cfg::eErrorInternal;
    pugi::xml_attribute l_pXmlAttr = m_cXmlNode.first_attribute();

    if (    (!m_bInitialized)
         || (!i_pName) 
         || (!o_pValue) 
       )
    {
        return l_eReturn;
    }

    while (1)
    {
        if ( l_pXmlAttr.empty() )
        {
            break;
        }
        else if ( 0 == PStrICmp(i_pName, l_pXmlAttr.name()) )
        {
            *o_pValue = l_pXmlAttr.as_uint();
            l_eReturn = Cfg::eOk;
            break;
        }
        else
        {
            l_pXmlAttr = l_pXmlAttr.next_attribute();
        }
    }

    return l_eReturn;
}//GetAttrInt32


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetAttrText(const tXCHAR * i_pName, tXCHAR **o_pValue)
{
    CLock               l_cLock(m_pCS);
    Cfg::eResult        l_eReturn  = Cfg::eErrorInternal;
    pugi::xml_attribute l_pXmlAttr = m_cXmlNode.first_attribute();

    if (    (!m_bInitialized)
         || (!i_pName)
         || (!o_pValue) 
       )
    {
        return l_eReturn;
    }


    while (1)
    {
        if ( l_pXmlAttr.empty() )
        {
            break;
        }
        else if ( 0 == PStrICmp(i_pName, l_pXmlAttr.name()) )
        {
            *o_pValue = (tXCHAR*)l_pXmlAttr.value();
            l_eReturn = Cfg::eOk;
            break;
        }
        else
        {
            l_pXmlAttr = l_pXmlAttr.next_attribute();
        }
    }

    return l_eReturn;
}//GetAttrText


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::SetAttrInt32(const tXCHAR *i_pName, tINT32 i_lValue)
{
    CLock               l_cLock(m_pCS);
    Cfg::eResult        l_eReturn  = Cfg::eErrorInternal;
    pugi::xml_attribute l_pXmlAttr = m_cXmlNode.first_attribute();

    if (    (!m_bInitialized)
         || (!i_pName) 
       )
    {
        return l_eReturn;
    }

    while (1)
    {
        if (l_pXmlAttr.empty())
        {
            break;
        }
        else if ( 0 == PStrICmp(i_pName, l_pXmlAttr.name()) )
        {
            l_pXmlAttr.set_value(i_lValue);
            l_eReturn = Cfg::eOk;
            break;
        }
        else
        {
            l_pXmlAttr = l_pXmlAttr.next_attribute();
        }
    }

    if (Cfg::eOk != l_eReturn)
    {
        l_pXmlAttr = m_cXmlNode.append_attribute(i_pName);
        if (false == l_pXmlAttr.empty())
        {
            l_pXmlAttr.set_value(i_lValue);
            l_eReturn = Cfg::eOk;
        }
    }

    return l_eReturn;
}//SetAttrInt32


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::SetAttrText(const tXCHAR *i_pName, const tXCHAR *i_pValue)
{
    CLock l_cLock(m_pCS);
    Cfg::eResult         l_eReturn  = Cfg::eErrorInternal;
    pugi::xml_attribute l_pXmlAttr = m_cXmlNode.first_attribute();

    if (    (!m_bInitialized)
         || (!i_pName) 
         || (!i_pValue) 
       )
    {
        return l_eReturn;
    }


    while (1)
    {
        if ( l_pXmlAttr.empty() )
        {
            break;
        }
        else if ( 0 == PStrICmp(i_pName, l_pXmlAttr.name()) )
        {
            l_pXmlAttr.set_value(i_pValue);
            l_eReturn = Cfg::eOk;
            break;
        }
        else
        {
            l_pXmlAttr = l_pXmlAttr.next_attribute();
        }
    }

    if (Cfg::eOk != l_eReturn)
    {
        l_pXmlAttr = m_cXmlNode.append_attribute(i_pName);
        if (false == l_pXmlAttr.empty())
        {
            l_pXmlAttr.set_value(i_pValue);
            l_eReturn = Cfg::eOk;
        }
    }

    return l_eReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetNext(Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized) 
         || (NULL == o_pNode) 
       )
    {
        return Cfg::eErrorInternal;
    }

    CBNode        *l_pReturn  = NULL;
    pugi::xml_node l_cXmlNode = m_cXmlNode.next_sibling();

    while (1)
    {
        if (true == l_cXmlNode.empty())
        {
            break;
        }
        else if ( pugi::node_element == l_cXmlNode.type() ) 
        {
            void *l_pNew = m_pMem->Pull();
            if (l_pNew)
            {
                l_pReturn = new (l_pNew) CBNode(m_pMem, l_cXmlNode, m_pCS);
            }
            break;
        }
        else
        {
            l_cXmlNode = l_cXmlNode.next_sibling();
        }
    }

    *o_pNode = static_cast<Cfg::INode*>(l_pReturn);

    return Cfg::eOk;
}//GetNext


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetNext(const tXCHAR *i_pName, Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);

    if (    (!m_bInitialized)
         || (!o_pNode) 
         || (!i_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    CBNode        *l_pResult  = NULL;
    pugi::xml_node l_cXmlNode = m_cXmlNode.next_sibling();

    while (1)
    {
        if (true == l_cXmlNode.empty())
        {
            break;
        }
        else if (    (pugi::node_element == l_cXmlNode.type() ) 
                  && (0 == PStrICmp(i_pName, l_cXmlNode.name()))
                )
        {
            void *l_pNode_Memory = m_pMem->Pull();
            if (l_pNode_Memory)
            {
                l_pResult = new (l_pNode_Memory) CBNode(m_pMem, l_cXmlNode, m_pCS);
            }
            break;
        }
        else
        {
            l_cXmlNode = l_cXmlNode.next_sibling();
        }
    }

    *o_pNode = static_cast<Cfg::INode*>(l_pResult);

    return Cfg::eOk;
}//GetNext


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetChildFirst(Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized)
         || (!o_pNode) 
       )
    {
        return Cfg::eErrorInternal;
    }

    pugi::xml_node l_pNode = m_cXmlNode.first_child();
    *o_pNode = CBRoot::GetChildFirst(&l_pNode);
   
    return (*o_pNode) ? Cfg::eOk : Cfg::eErrorInternal;
}//GetChildFirst


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::GetChildFirst(const tXCHAR * i_pName, Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);

    if (    (!m_bInitialized)
         || (!o_pNode) 
         || (!i_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    pugi::xml_node l_pNode = m_cXmlNode.first_child();
    *o_pNode = CBRoot::GetChildFirst(i_pName, &l_pNode);

    return (*o_pNode) ? Cfg::eOk : Cfg::eErrorInternal;
}//GetChildFirst


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::AddChildEmpty(const tXCHAR * i_pName, Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (FALSE == m_bInitialized)
         || (NULL == o_pNode)
         || (NULL == i_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    *o_pNode = CBRoot::AddChildEmpty(i_pName, &m_cXmlNode);

    return (*o_pNode) ? Cfg::eOk : Cfg::eErrorInternal;
}//AddChildEmpty


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::DelChild(Cfg::INode *i_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized)
         || (!i_pNode) 
       )
    {
        return Cfg::eErrorInternal;
    }

    return CBRoot::DelChild(&m_cXmlNode, i_pNode);
}//DelChild


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBNode::Del()
{
    CLock l_cLock(m_pCS);
    if (!m_bInitialized) 
    {
        return Cfg::eErrorInternal;
    }

    pugi::xml_node l_cXmlNode = m_cXmlNode.parent();
    if (l_cXmlNode.empty())
    {
        return Cfg::eErrorInternal;
    }

    l_cXmlNode.remove_child(m_cXmlNode);

    return Cfg::eOk;
}//Del

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBNode::~CBNode()
{
    CLock l_cLock(m_pCS);
    if (m_pXmlText)
    {
        free(m_pXmlText);
        m_pXmlText = NULL;
        m_szXmlText = 0;
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////CBDoc///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBDoc::CBDoc(const tXCHAR *i_pFileName)
    : CBRoot(NULL, NULL)
    , m_cXmlDoc()
{
    m_cFileName.Set(i_pFileName);

    if (m_bInitialized)
    {
        size_t l_dwMemSize = sizeof(CBNode);

        if (sizeof(CBDoc) > l_dwMemSize)
        {
            l_dwMemSize = sizeof(CBDoc);
        }

        m_pMem = new CBNodeMem(l_dwMemSize);
        if (NULL == m_pMem)
        {
            m_bInitialized = FALSE;
        }
    }

    if (m_bInitialized)
    {
        m_pCS = new CLock();
        if (!m_pCS)
        {
            m_bInitialized = FALSE;
        }
    }

    if (m_bInitialized)
    {
        if (m_cFileName.Length())
        {
            if (!m_cXmlDoc.load_file(m_cFileName.Get(), pugi::parse_full))
            {
                m_bInitialized = FALSE;
            }
        }
        else
        {
            if (!m_cXmlDoc.load(TM(""), pugi::parse_full))
            {
                m_bInitialized = FALSE;
            }
        }
    }
}//CBDoc


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBDoc::CBDoc(const void *i_pBuffer, size_t i_szBuffer)
    : CBRoot(NULL, NULL)
    , m_cXmlDoc()
{
    m_cFileName.Set(TM(""));

    if (m_bInitialized)
    {
        size_t l_dwMemSize = sizeof(CBNode);

        if (sizeof(CBDoc) > l_dwMemSize)
        {
            l_dwMemSize = sizeof(CBDoc);
        }

        m_pMem = new CBNodeMem(l_dwMemSize);
        if (NULL == m_pMem)
        {
            m_bInitialized = FALSE;
        }
    }

    if (m_bInitialized)
    {
        m_pCS = new CLock();
        if (!m_pCS)
        {
            m_bInitialized = FALSE;
        }
    }

    if (m_bInitialized)
    {
        if (!m_cXmlDoc.load_buffer(i_pBuffer, i_szBuffer, pugi::parse_full))
        {
            m_bInitialized = FALSE;
        }
    }
}//CBDoc


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBDoc::~CBDoc()
{
    if (m_pMem)
    {
        delete m_pMem;
        m_pMem = NULL;
    }

    if (m_pCS)
    {
        delete m_pCS;
        m_pCS = NULL;
    }

}//~CBDoc


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBDoc::Save()
{
    CLock l_cLock(m_pCS);
    if (!m_bInitialized)
    {
        return Cfg::eErrorInternal;
    }

    if (0 >= m_cFileName.Length())
    {
        return Cfg::eErrorNotSupported;
    }

    return Save_As(m_cFileName.Get());
}//Save


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBDoc::Save_As(const tXCHAR *i_pFile_Name)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized) 
         || (!i_pFile_Name) 
       )
    {
        return Cfg::eErrorInternal;
    }

    if (0 >= m_cFileName.Length())
    {
        m_cFileName.Set(i_pFile_Name);
    }

#if defined(UTF8_ENCODING)
    xml_encoding l_xmlEnc = pugi::encoding_utf8;
#else
    xml_encoding l_xmlEnc = pugi::encoding_utf16_le;
#endif

    return (true == m_cXmlDoc.save_file(i_pFile_Name, 
                                        TM("    "), 
                                        pugi::format_default | pugi::format_write_bom, 
                                        l_xmlEnc
                                       )
           ) ? Cfg::eOk : Cfg::eErrorInternal;

}//Save_As

 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBDoc::GetChildFirst(Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized)
         || (!o_pNode)
       )
    {
        return Cfg::eErrorInternal;
    }

    pugi::xml_node l_pNode = m_cXmlDoc.first_child();
    *o_pNode = CBRoot::GetChildFirst(&l_pNode);

    return (*o_pNode) ? Cfg::eOk : Cfg::eErrorInternal;
}//GetChildFirst


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBDoc::GetChildFirst(const tXCHAR *i_pName, Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized)
         || (!o_pNode) 
         || (!i_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    pugi::xml_node l_pNode = m_cXmlDoc.first_child();
    *o_pNode = CBRoot::GetChildFirst(i_pName, &l_pNode);

    return (*o_pNode) ? Cfg::eOk : Cfg::eErrorInternal;
}//GetChildFirst


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBDoc::AddChildEmpty(const tXCHAR *i_pName, Cfg::INode **o_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized) 
         || (!o_pNode) 
         || (!i_pName) 
       )
    {
        return Cfg::eErrorInternal;
    }

    *o_pNode = CBRoot::AddChildEmpty(i_pName, static_cast<pugi::xml_node*>(&m_cXmlDoc));

    return (*o_pNode) ? Cfg::eOk : Cfg::eErrorInternal;
}//AddChildEmpty


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Cfg::eResult CBDoc::DelChild(Cfg::INode *i_pNode)
{
    CLock l_cLock(m_pCS);
    if (    (!m_bInitialized) 
         || (!i_pNode) 
       )
    {
        return Cfg::eErrorInternal;
    }

    return CBRoot::DelChild(static_cast<pugi::xml_node*>(&m_cXmlDoc), i_pNode);
}//DelChild
