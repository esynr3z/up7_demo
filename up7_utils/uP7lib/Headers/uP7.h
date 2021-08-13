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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                    //
//                                                 microP7                                                            //
//                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                    //
//                          Documentation is located in <uP7>/Documentation/uP7.pdf                                   //
//                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef UP7_API_H
#define UP7_API_H

#include "uP7platform.h"
#include "uP7helpers.h"
#include "uP7protocol.h"

/*! uP7 telemetry counter ID type*/
typedef uint16_t huP7TelId;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                          General uP7 structures                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! Data packet element rank/priority.
 *  Transport between CPU & host sometimes is piece of HW and usually it is quite restricted in term of size. 
 *  uP7 can generate big amount of data and transport capacity maybe not big/fast enough to proceed such amount of data.
 *  to help upper level (integrator) to decide which data can be skipped uP7 will provide with each data block the rank
 *  and it is up to integrator to decide which data has to be processed and deserved to be delivered.
*/
enum euP7packetRank
{
    euP7packetRankHighest,   /**< Maximum data priority */
    euP7packetRankHigh,      /**< High data priority    */
    euP7packetRankMedium,    /**< Medium data priority  */
    euP7packetRankLow        /**< Low data priority     */
};

/*! packet chunk structure, one data packet may consist of few chunks*/
struct stP7packetChunk
{
    void   *pData;  /**< User data */
    size_t  szData; /**< Data size */
};


/*! Data packet alignment.
 *  Transport between CPU & host sometimes is aligned and it is much more simple to transim packets if 
 *  uP7 packets are aligned in size
*/
enum euP7alignment
{
    euP7alignmentNone = 0,      /**< every packet size will be not aligned*/
    euP7alignment4    = 0x3,    /**< every packet size will be aligned on 4 bytes    */
    euP7alignment8    = 0x7,    /**< every packet size will be aligned on 8 bytes    */
    euP7alignment16   = 0xF,    /**< every packet size will be aligned on 16 bytes    */
};



/**
 * \brief callback function type to get high-resolution system timer frequency
 * @param i_pCtx [in] calling software context
 * @return frequency in hertz
*/
typedef uint64_t(*fnuP7getTimerFrequency)(void *i_pCtx);

/**
 * \brief callback function type to get high-resolution system timer value
 * @param i_pCtx [in] calling software context
 * @return timer value
*/
typedef uint64_t(*fnuP7getTimerValue)(void *i_pCtx);

/**
 * \brief callback function type for memory allocation (heap)
 * @param i_pCtx [in] calling software context
 * @param i_szMem [in] memory size in bytes to be allocated
 * @return memory pointer
*/
typedef void *(*fnuP7malloc)(void *i_pCtx, size_t i_szMem);

/**
 * \brief callback function type for memory de-allocation (heap)
 * @param i_pCtx [in] calling software context
 * @param i_pMem [in] memory pointer
*/
typedef void(*fnuP7free)(void *i_pCtx, void *i_pMem);

/**
 * \brief callback function type for OS mutex lock function
 * @param i_pCtx [in] calling software context
*/
typedef void(*fnuP7lock)(void *i_pCtx);

/**
 * \brief callback function type for OS mutex unlock function
 * @param i_pCtx [in] calling software context
*/
typedef void(*fnuP7unlock)(void *i_pCtx);

/**
 * \brief callback function type for OS thread ID function
 * @param i_pCtx [in] calling software context
*/
typedef uint32_t(*fnuP7getCurrentThreadId)(void *i_pCtx);


/**
 * \brief callback function type for sending uP7 packet to host
 * N.B. It is responsibility of integrator to transmit that packet to host ENTIRELY.
 *      if one of the chunks can't be sent - entire packet should not be sent!
 * @param i_pCtx [in] calling software context
 * @param i_eRank [in] Data rank/priority 
 * @param i_pChunks [in] chunks array
 * @param i_szChunks [in] chunks count
 * @param i_szData [in] cumulative size of data represented by chunks
 * @return true - data has been sent, false - otherwise
*/
typedef bool(*fnuP7sendPacket)(void                         *i_pCtx,  
                               enum euP7packetRank           i_eRank,  
                               const struct stP7packetChunk *i_pChunks, 
                               size_t                        i_szChunks, 
                               size_t                        i_szData);

/*! Enum describes different verbosity & trace levels */
enum euP7Level
{
    euP7Level_Trace       = 0,  /*!< minimum verbosity level, maximum information to be transmitted */
    euP7Level_Debug          ,  /*!< debug messages, info, up to critical */
    euP7Level_Info           ,  /*!< info messages, warning, up to critical */
    euP7Level_Warning        ,  /*!< warning messages, error, up to critical */
    euP7Level_Error          ,  /*!< only error & critical messages */
    euP7Level_Critical       ,  /*!< only critical messages */

    euP7Levels_Count
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         uP7::preprocessor structures                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! uP7 module descriptor structure*/
struct stuP7Module
{
    const char         *pName;
    enum euP7Level      eLevel;
    uint16_t            wId;
    uint32_t            uHash;
};


/*! uP7 trace arguments types enum*/
enum euP7arg
{
    euP7_arg_unk = 0x00,
    euP7_arg_char,
    euP7_arg_char16,
    euP7_arg_char32,
    euP7_arg_int8,
    euP7_arg_int16,
    euP7_arg_int32,
    euP7_arg_int64,
    euP7_arg_double,
    euP7_arg_pvoid,
    euP7_arg_str_ansi,
    euP7_arg_str_utf8,
    euP7_arg_str_utf16,
    euP7_arg_str_utf32,
    euP7_arg_intmax,

    euP7_args_count
};

/*! uP7 trace argument descriptor structure*/
struct stuP7arg
{
    uint8_t eType; //enum euP7arg
    uint8_t eStackSize;
};

/*! uP7 trace item descriptor structure*/
struct stuP7Trace
{
    uint16_t               wId;
    uint16_t               wArgsCount;
    const struct stuP7arg *pArgs;
};

/*! uP7 telemetry descriptor structure*/
struct stuP7telemetry
{
    const char            *pName;
    uint32_t               uHash;
    bool                   bOn;
    huP7TelId              wId;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                uP7 functions                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! uP7 configuration structure */
struct stuP7config
{
    uint32_t                uSessionId;           /**< Session ID*/  
    uint8_t                 bSessionIdCrc7;       /**< Session ID crc */ 
    void                   *pCtxTimer;            /**< Context for timer functions */
    fnuP7getTimerFrequency  cbTimerFrequency;     /**< callback to get system high-resolution timer frequency */
    fnuP7getTimerValue      cbTimerValue;         /**< callback to get system high-resolution timer value */
                                                  
    void                   *pCtxLock;             /**< Context for lock functions, set it to NULL if there is no RTOS*/
    fnuP7lock               cbLock;               /**< callback to call OS lock function, set it to NULL if there is no RTOS */
    fnuP7unlock             cbUnLock;             /**< callback to call OS unlock function, set it to NULL if there is no RTOS */
                                                  
    void                   *pCtxPacket;           /**< Context for data sending */
    fnuP7sendPacket         cbSendPacket;         /**< callback to send data packet to host */  

    void                   *pCtxThread;           /**< Context for cbGetCurrentThreadId function */
    fnuP7getCurrentThreadId cbGetCurrentThreadId; /**< callback get current thread ID, set it to NULL if there is no RTOS*/  

    enum euP7Level          eDefaultVerbosity;    /**< Default verbosity*/  

            
    /**<WARNING: content of pModules, pTraces, pTelemetry descriptors WILL be changed by uP7 core
                 due to the fact that descriptors are created by uP7 preprocessor and statically located inside
                 firmware/binary - MPU may trigger because of the attempt to modify .bss section.
    */
    struct stuP7Module     *pModules;             /**< Trace modules descriptors, generated by uP7 preprocessor*/
    size_t                  szModules;            /**< Trace modules descriptors count*/  

    struct stuP7Trace      *pTraces;              /**< Trace descriptors, generated by uP7 preprocessor*/  
    size_t                  szTraces;             /**< Trace descriptors count*/  

    struct stuP7telemetry  *pTelemetry;           /**< Telemetry descriptors, generated by uP7 preprocessor*/  
    size_t                  szTelemetry;          /**< Telemetry descriptors count*/  

    enum euP7alignment      eAlignment;           /**< transport packet size alignment*/      
};


/**
 * \brief P7 initialization function, should be called once at CPU startup
 * @param i_pConfig [in] configuration
 * @return true - uP7 has been initialized, false - failure
*/
bool uP7initialize(const struct stuP7config *i_pConfig);


/**
 * \brief P7 uninitialize function, usually it is not used. Only for testing purposes
*/
void uP7unInitialize();

/**
 * \brief uP7 process function is in charge of parsing and analyzing data from host. For example host can manage telemetry
 *        counters (on/off) or change verbosity, the commands are packed and send to CPU using IuP7stream interface. 
 *        Integrator is in charge to provide that data to uP7 using this function
 * @param i_pData [in] data from host
 * @param i_szData [in] data size in bytes
 * @return count of processed bytes
*/
size_t uP7process(const uint8_t *i_pData, size_t i_szData);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         uP7::Tracing structures & functions                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define uP7TRC(i_wID, i_hModule, i_pFormatStr, ...) uP7TrcSent(i_wID, euP7Level_Trace,    i_hModule, ##__VA_ARGS__)
#define uP7DBG(i_wID, i_hModule, i_pFormatStr, ...) uP7TrcSent(i_wID, euP7Level_Debug,    i_hModule, ##__VA_ARGS__)
#define uP7INF(i_wID, i_hModule, i_pFormatStr, ...) uP7TrcSent(i_wID, euP7Level_Info,     i_hModule, ##__VA_ARGS__)
#define uP7WRN(i_wID, i_hModule, i_pFormatStr, ...) uP7TrcSent(i_wID, euP7Level_Warning,  i_hModule, ##__VA_ARGS__)
#define uP7ERR(i_wID, i_hModule, i_pFormatStr, ...) uP7TrcSent(i_wID, euP7Level_Error,    i_hModule, ##__VA_ARGS__)
#define uP7CRT(i_wID, i_hModule, i_pFormatStr, ...) uP7TrcSent(i_wID, euP7Level_Critical, i_hModule, ##__VA_ARGS__)

/*! uP7 trace module type, used to create different modules for tracing. Each module has it's own name & verbosity */
typedef uint32_t huP7Module;

/*! uP7 invalid module ID value*/
#define uP7_MODULE_INVALID_ID ((huP7Module)~((huP7Module)0))

/**
 * \brief set trace verbosity, traces with less priority will be rejected. You may set verbosity online from Baical
          server. See documentation for details.
 * @param i_hModule [in] module, if NULL - default verbosity will be set
 * @param i_eVerbosity [in] verbosity level
*/
void uP7TrcSetVerbosity(huP7Module i_hModule, enum euP7Level i_eVerbosity);

/**
 * \brief function used to specify name for current thread, allows to have nice trace message formatting on Baical
 *        server. Call the function from the newly created thread & call uP7TrcUnregisterCurrentThread() right before  
 *        thread destroying
 * @param i_pName [in] thread name
 * @return true - successful registration, false - failure
*/
//bool uP7TrcRegisterCurrentThread(const char *i_pName);

/**
 * \brief function used to unregister current thread
 * @return true - successful registration, false - failure
*/
//bool uP7TrcUnregisterCurrentThread();


/**
 * \brief function is used to register trace module. Modules are used to group trace messages by modules, use the same
 *        verbosity level per module and to have nice formatting on Baical side - each trace will be marked with module 
 *        name. If module with such name is already registered - existing handle will be returned
 * @param i_pName [in] module name
 * @param i_eVerbosity [in] module verbosity
 * @param o_hModule [out] module handle
 * @return true - successful registration, false - failure
*/
bool uP7TrcRegisterModule(const char *i_pName, enum euP7Level i_eVerbosity, huP7Module *o_hModule);


/**
 * \brief function is used to find trace module by name. Modules are used to group trace messages by modules, use the same
 *        verbosity level per module and to have nice formatting on Baical side - each trace will be marked with module 
 *        name. If module with such name is already registered - existing handle will be returned
 * @param i_pName [in] module name
 * @param i_eVerbosity [in] module verbosity
 * @param o_hModule [out] module handle
 * @return true - successful registration, false - failure
*/
bool uP7TrcFindModule(const char *i_pName, huP7Module *o_hModule);


/**
 * \brief Function for internal usage, please use macros uP7TRC, uP7DBG, etc. instead. 
 * @param i_wId [in] message ID
          N.B: please do not change it, it is assigned automatically by uP7preProcessor tool!
 * @param i_eLevel [in] trace level
 * @param i_hModule [in] module
 * @param i_pFormat [in] format string
 * @param ... [in] variable arguments list
 * @return true - success, false - failure
*/
bool uP7TrcSent(uint16_t       i_wId,
                enum euP7Level i_eLevel, 
                huP7Module     i_hModule,
                ...);

/**
 * \brief function is used sent trace messages to host
 * @param i_wId [in] message ID
          N.B: please do not change it, it is assigned automatically by uP7preProcessor tool!
 * @param i_eLevel [in] trace level
 * @param i_hModule [in] module
 * @param i_pFormat [in] format string
 * @param i_pVa_List [in] variable arguments list
 * @return true - success, false - failure
*/
bool uP7TrcSentEmb(uint16_t       i_wId,
                   enum euP7Level i_eLevel, 
                   huP7Module     i_hModule,
                   va_list       *i_pVa_List);
                 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     uP7::Telemetry structures & functions                                          //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*! uP7 invalid counter ID value*/
#define uP7_TELEMETRY_INVALID_ID ((huP7TelId)~((huP7TelId)0))



/*! uP7 telemetry counter sample type*/
typedef UP7_PRECISE_TYPE tuP7TelVal;


/**
 * \brief function to register telemetry counter
 * @param i_pName [in] counter name
 * @param i_tMin [in] min counter value
 * @param i_tbAlarmMin [in] value below which counter value will be interpreted as alarm signal on Baical's side
 * @param i_tMax [in] max counter value
 * @param i_tAlarmMax [in] value above which counter value will be interpreted as alarm signal on Baical's side
 * @param i_bOn [in] default counter state (true - on, false - off), can be changed later in real-time from Baical
 * @param o_pID [out] counter ID
 * @return true - success, false - failure
*/
bool uP7TelCreateCounter(const char *i_pName, 
                         tuP7TelVal  i_tMin,
                         tuP7TelVal  i_tbAlarmMin,
                         tuP7TelVal  i_tMax,
                         tuP7TelVal  i_tAlarmMax,
                         bool        i_bOn,
                         huP7TelId  *o_pID 
                        );

/**
 * \brief function to sent counter sample
 * @param i_hID [in] counter ID
 * @param i_tValue [in] sample value
 * @return true - success, false - failure
*/
bool uP7TelSentSample(huP7TelId i_hID, tuP7TelVal i_tValue);

/**
 * \brief function to find counter ID by name (case sensitive)
 * @param i_pName [in] counter name
 * @param o_pID [out] counter ID
 * @return true - success, false - failure
*/
bool uP7TelFindCounter(const char *i_pName, huP7TelId *o_pID);

/**
 * \brief function to enable counter
 * @param i_hID [in] counter ID
*/
void uP7TelEnableCounter(huP7TelId i_hID, bool i_bEnable);

/**
 * \brief function to retrieve enable state of the counter
 * @param i_hID [in] counter ID
 * @return true - enabled, false - disabled
*/
bool uP7TelIsCounterEnabled(huP7TelId i_hID);

/**
 * \brief get counter's name
 * @param i_hID [in] counter ID
 * @return name
*/
const char *uP7TelGetCounterName(huP7TelId i_hID);
 
/**
 * \brief get amount of counters
 * @return count
*/
size_t uP7TelGetCountersAmount();


#endif //UP7_API_H
