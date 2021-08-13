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
//                                  microP7 proxy host library interface                                              //
//                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                    //
//               Integrator's documentation is located in <uP7>/Documentation/uP7 integrator manual.pdf               //
//                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef UP7_PROXY_API_H
#define UP7_PROXY_API_H

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "GTypes.h"

#define uP7_FILES_EXT TM("up7")

#define uP7_SHARED_TRACE     TM("up7TraceChannel")
#define uP7_SHARED_TELEMETRY TM("up7TelemetryChannel")

#define SESSION_ID_FILE_VER  0x10100000

/*! Session file header */
struct stuP7SessionFileHeader
{
    uint32_t uVersion;
    uint32_t uSessionId;
    uint64_t qwTimeStamp;
    uint32_t uCrc7 :7;
    uint32_t uFlags:25;
};


/*! Data stream interface. Used to make data connection between CPUs with uP7 on the board and host.
 *  Stream is bi-directional data pipe between CPU & Host, capable to work on multi-thread environment
*/
class IuP7Fifo
{
public:
    /**
     * \brief Write CPU outgoing raw data to fifo, this data will be processed by proxy and redirected to 
     * Baical/console/etc.
     * @param i_pData [in] data to be sent
     * @param i_szData [in] data size in bytes
     * @return true - data has been written, false - otherwise
    */
    virtual bool Write(const void *i_pData, size_t i_szData) = 0;


    /*! packet chunk structure, one data packet may consist of few chunks*/
    struct stChunk
    {
        void   *pData;  /**< User data */
        size_t  szData; /**< Data size */
    };

    /**
     * \brief Write CPU outgoing raw data to fifo, this data will be processed by proxy and redirected to 
     * Baical/console/etc.
     * @param i_pChunks [in] data chunks array
     * @param i_szChunks [in] chunks count
     * @param i_szData [in] cumulative size of data represented by chunks
     * @return true - data has been written, false - otherwise
    */
    virtual bool Write(const IuP7Fifo::stChunk *i_pChunks, size_t i_szChunks, size_t i_szData) = 0;

    /**
     * \brief return free bytes count of write buffer
     * @return bytes count
    */
    virtual size_t GetFreeBytes() = 0;


    /**
     * \brief Read data from Baical. It has to be delivered to CPU and processed using uP7process() function on CPU side
     * @param i_pBuffer [in] user buffer
     * @param i_szBuffer [in] user buffer max size in bytes
     * @param i_uTimeout [in] timeout in milliseconds
     * @return size of the written data.
    */
    virtual size_t Read(void *i_pBuffer, size_t i_szBuffer, uint32_t i_uTimeout) = 0;

    /**
     * \brief Increase object reference counter
     * @return new reference counter value
    */
    virtual int32_t Add_Ref() = 0;

    /**
     * \brief Decrease object reference counter, when it become 0 - object will be self-destroyed
     * @return new reference counter value
    */
    virtual int32_t Release() = 0;
};


/*! CPU time interface. Used to provide precise CPU time to proxy & Baical
*/
class IuP7Time
{
public:
    /**
     * \brief get CPU clock frequency
     * @return CPU clock frequency in Hz
    */
    virtual uint64_t GetFrequency() = 0;

    /**
     * \brief get CPU clock value
     * @return CPU clock value
    */
    virtual uint64_t GetTime() = 0;

    /**
     * \brief Increase object reference counter
     * @return new reference counter value
    */
    virtual int32_t Add_Ref() = 0;

    /**
     * \brief Decrease object reference counter, when it become 0 - object will be self-destroyed
     * @return new reference counter value
    */
    virtual int32_t Release() = 0;
};


/*! uP7 proxy interface. Create mapping between Bare-metal CPU and host P7 library

               +--------+ +--------+ +--------+
               |  CPU1  | |  CPU...| |  CPUN  |   Bare-Metal CPU or RTOS
               |  [uP7] | |  [uP7] | |  [uP7] |
               +---++---+ +---++---+ +---++---+          
                   || F       || F       || F
                   || I       || I       || I
 - - - - - - - - - || F - - - || F - - - || F - - - - - - - - - - - - - - - - 
                   || O       || O       || O
                   ||         ||         ||        Host code, full size OS
                   ||         ||         ||        (Linux, Windows, etc)
               +---++---------++---------++---+                                
               |   ||         ||         ||   |          IuP7proxy
               |   ++---------++---------++   | 
               |   ||                    ||   | 
               | +-++----+      +--------++-+ | 
               | | Trace |      | Telemetry | | 
               | +-++----+      +--------++-+ | 
               |   ||                    ||   | 
               | +-++--------------------++-+ | 
               | |                          | | 
               | |        P7 library        | | 
               | |                          | | 
               | +------------++------------+ | 
               |              ||              |
               |     +--------++---------+    |                            
               |     |       Sink        |    |                            
               |     | * Network (Baical)|    |                            
               |     | * FileBin         |    |                            
               |     | * FileTxt         |    |                            
               |     | * Console         |    |                            
               |     | * Syslog          |    |                            
               |     | * Auto            |    |                            
               |     | * Null            |    |                            
               |     +-------------------+    |                            
               |                              |
               +------------------------------+                                
*/
class IuP7proxy
{
public:

    /**
     * \brief Register new CPU and connect it to P7
     * @param i_bCpuId [in] CPU ID
     * @param i_bBigEndian [in] Big-endian CPU flag
     * @param i_qwFreq [in] target CPU clock frequency in Hz. This clock is used for trace & telemetry timestamp
     * @param i_pName [in] channel name, will be used to display in Baical server
     * @param i_szFifoLen [in] FIFOs size in bytes (IuP7stream  *&o_iStream). Min value 16384 bytes.
     * @param i_bFifoBiDirectional [in] flag to specify that communication with CPU is bidirectional and proxy can use 
     * it to send control data to CPU like disable telemetry counter, change verbosity, etc.
     * If fifo is bidirections - set to "true", otherwise - "false"
     * @param o_iFifo [out] FIFO object to be used for CPU communication. 
     *                N.B.: Please do not forget to call o_iFifo->Release() right after UnRegisterCpu();
     * @return true - success, false - error
    */
    virtual bool RegisterCpu(uint8_t       i_bCpuId, 
                             bool          i_bBigEndian, 
                             uint64_t      i_qwFreq, 
                             const tXCHAR *i_pName,
                             size_t        i_szFifoLen,
                             bool          i_bFifoBiDirectional,
                             IuP7Fifo    *&o_iFifo
                            ) = 0;

    /**
     * \brief Register new CPU and connect it to P7
     * @param i_bCpuId [in] CPU ID
     * @param i_bBigEndian [in] Big-endian CPU flag
     * @param i_iTime [in] target CPU clock 
     * @param i_pName [in] channel name, will be used to display in Baical server
     * @param i_szFifoLen [in] FIFOs size in bytes (IuP7stream  *&o_iStream). Min value 16384 bytes.
     * @param i_bFifoBiDirectional [in] flag to specify that communication with CPU is bidirectional and proxy can use 
     * it to send control data to CPU like disable telemetry counter, change verbosity, etc.
     * If fifo is bidirections - set to "true", otherwise - "false"
     * @param o_iFifo [out] FIFO object to be used for CPU communication. 
     *                N.B.: Please do not forget to call o_iFifo->Release() right after UnRegisterCpu();
     * @return true - success, false - error
    */
    virtual bool RegisterCpu(uint8_t       i_bCpuId, 
                             bool          i_bBigEndian, 
                             IuP7Time     *i_iTime, 
                             const tXCHAR *i_pName,
                             size_t        i_szFifoLen,
                             bool          i_bFifoBiDirectional,
                             IuP7Fifo    *&o_iFifo
                            ) = 0;


    /**
     * \brief Unregister CPU and release stream object
     * @param i_bCpuId [in] CPU ID
     * @return true - success, false - error
    */
    virtual bool UnRegisterCpu(uint8_t i_bCpuId) = 0;

    /**
     * \brief Increase object reference counter
     * @return new reference counter value
    */
    virtual int32_t Add_Ref() = 0;

    /**
     * \brief Decrease object reference counter, when it become 0 - object will be self-destroyed
     * @return new reference counter value
    */
    virtual int32_t Release() = 0;
};

/**
    * \brief Create instance of uP7 proxy object. 
    * @param i_pP7Args [in] P7 arguments, see P7 documentation for details. May be NULL
    * @param i_puP7Dir [in] directory where uP7 description files are located, previously created by uP7preProcessor tool
    * @return new reference counter value
*/
extern "C" P7_EXPORT IuP7proxy* __cdecl uP7createProxy(const tXCHAR  *i_pP7Args, 
                                                       const tXCHAR  *i_puP7Dir
                                                      );


#endif //UP7_PROXY_API_H
