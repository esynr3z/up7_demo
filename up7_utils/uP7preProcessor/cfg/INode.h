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
#ifndef CFG_INODE_H
#define CFG_INODE_H

namespace Cfg
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> result codes</summary>
    enum eResult
    {
        /// <summary> Success result code </summary>
        eOk = 0,

        /// <summary> general error code </summary>
        eError,
        /// <summary> arguments error </summary>
        eErrorWrongInput,
        /// <summary> no available memory </summary>
        eErrorNoBuffer,
        /// <summary> internal error </summary>
        eErrorInternal,
        /// <summary> function isn't supported </summary>
        eErrorNotSupported,
        /// <summary> function isn't implemented </summary>
        eErrorNotImplemented,
        /// <summary> access to function of file is blocked by the system </summary>
        eErrorBlocked,
        /// <summary> functionality is not active for the time being </summary>
        eErrorNotActive,
        /// <summary> not appropriate plugin for the stream </summary>
        eErrorMissmatch,
        /// <summary> stream is closed </summary>
        eErrorClosed,
        /// <summary> timeout </summary>
        eErrorTimeout,
        /// <summary> can't write to file </summary>
        eErrorFileWrite
    };


    /// <summary> Interface provide access to Baical properties storage tree. </summary>
    class INode
    {
    public:
        /// <summary> Get node name </summary>
        /// <param name="o_pName"> pointer to variable contains node name </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetName(tXCHAR **o_pName) = 0;

        /// <summary> Set node name </summary>
        /// <param name="i_pName"> pointer to variable contains node name </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  SetName(const tXCHAR *i_pName) = 0;

        /// <summary> Copy nodes tree recursivly from provided source </summary>
        /// <param name="i_pSource"> source node for copy </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  Copy(Cfg::INode *i_pSource) = 0;

        /// <summary> Get node attribute name by index, index range is [0 .. N) </summary>
        /// <param name="i_dwIndex"> attribute index </param>
        /// <param name="o_rName"> attribute name, output value</param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetAttr(tUINT32 i_dwIndex, const tXCHAR *&o_rName) = 0;

        /// <summary> Delete node attribute name </summary>
        /// <param name="i_pName"> attribute name</param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  DelAttr(const tXCHAR *i_pName) = 0;

        /// <summary> Get attribute int value </summary>
        /// <param name="i_pName"> attribute name</param>
        /// <param name="o_pValue"> attribute value, output</param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetAttrInt32(const tXCHAR *i_pName, tINT32 *o_pValue) = 0;

        /// <summary> Get attribute text value </summary>
        /// <param name="i_pName"> attribute name</param>
        /// <param name="o_pValue"> attribute value, output</param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetAttrText(const tXCHAR  *i_pName, tXCHAR **o_pValue) = 0;

        /// <summary> Set attribute long value </summary>
        /// <param name="i_pName"> attribute name </param>
        /// <param name="i_lValue"> attribute value </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  SetAttrInt32(const tXCHAR *i_pName, tINT32 i_lValue)= 0;

        /// <summary> Set attribute text value </summary>
        /// <param name="i_pName"> attribute name </param>
        /// <param name="i_pValue"> attribute value </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  SetAttrText(const tXCHAR *i_pName, const tXCHAR *i_pValue) = 0;

        /// <summary> Get next node </summary>
        /// <param name="o_pIXNode"> next node, output </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetNext(Cfg::INode **o_pIXNode) = 0;

        /// <summary> Get next node using node name </summary>
        /// <param name="i_pName"> node name </param>
        /// <param name="o_pIXNode"> next node, output </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetNext(const tXCHAR *i_pName, Cfg::INode **o_pIXNode) = 0;

        /// <summary> Get first child </summary>
        /// <param name="o_pIXNode"> child node, output </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetChildFirst(Cfg::INode **o_pIXNode) = 0;

        /// <summary> Get first child using name </summary>
        /// <param name="i_pName"> node name </param>
        /// <param name="o_pIXNode"> child node, output </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetChildFirst(const tXCHAR *i_pName, Cfg::INode **o_pIXNode) = 0;

        /// <summary> Add empty child node </summary>
        /// <param name="i_pName"> node name </param>
        /// <param name="o_pIXNode"> child node, output </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  AddChildEmpty(const tXCHAR *i_pName, Cfg::INode **o_pIXNode) = 0;

        /// <summary> Delete child node </summary>
        /// <param name="i_pIXNode"> child node </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  DelChild(Cfg::INode *i_pIXNode) = 0;

        /// <summary> Delete current node </summary>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  Del() = 0;

        /// <summary> Get XML text in current encoding </summary>
        /// <param name="o_rData"> XML data  </param>
        /// <param name="o_rSize"> size in bytes </param>
        /// <returns>Bk::eResult::eOk in case of success</returns>  
        virtual Cfg::eResult  GetXml(const void *&o_rData, size_t &o_rSize) = 0;


        /// <summary> increase node's reference counter </summary>
        /// <returns> new reference counter value </returns>  
        virtual tINT32       Add_Ref() = 0;

        /// <summary> decrease node's reference counter </summary>
        /// <returns> new reference counter value </returns>  
        virtual tINT32       Release() = 0;
    };

    class IDoc
    {
    public:                 
        virtual tBOOL        Get_Initialized() = 0;
        virtual Cfg::eResult Save() = 0;
        virtual Cfg::eResult Save_As(const tXCHAR *i_pFile_Name) = 0;

        virtual Cfg::eResult GetChildFirst(Cfg::INode **o_pIXNode) = 0;
        virtual Cfg::eResult GetChildFirst(const tXCHAR  *i_pName, Cfg::INode **o_pIXNode) = 0;
        virtual Cfg::eResult AddChildEmpty(const tXCHAR  *i_pName, Cfg::INode **o_pIXNode) = 0;
        virtual Cfg::eResult DelChild(Cfg::INode *i_pIXNode) = 0;
        virtual void         Release() = 0;
    };
}


extern "C" P7_EXPORT Cfg::IDoc* __cdecl IBDoc_Load(const tXCHAR *i_pFile_Name);
extern "C" P7_EXPORT Cfg::IDoc* __cdecl IBDoc_Load_Buffer(const void *i_pBuffer, size_t i_szBuffer);
extern "C" P7_EXPORT Cfg::IDoc* __cdecl IBDoc_Create();

#endif //CFG_INODE_H