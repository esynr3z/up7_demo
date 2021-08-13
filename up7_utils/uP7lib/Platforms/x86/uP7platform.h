////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                    //
//                                                 microP7 platform header                                            //
//                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef UP7_PLATFORM_H
#define UP7_PLATFORM_H


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//                   N.B.: Please include here platform specific headers, global variables if any                     //
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#include <stdarg.h> 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>


/*! uP7 maximum memory which can be allocated by stack variables.
 * To increase performance uP7 trying to minimize amount lock/unlock calls and allocating most of the data on the stack
*/
#define UP7_MAX_STACK_SIZE_IN_BYTES 1024

/*! uP7 maximum size of trace item in bytes. Recommended value is less or equal to half of UP7_MAX_STACK_SIZE_IN_BYTES value*/
#define UP7_MAX_TRACE_SIZE_IN_BYTES 512


/*! if target CPU support double floating point - activate this macro */
#define UP7_FLOATING_POINT

/*! uP7 telemetry sample value type. Next types can be used: 
 *  - 0 = double
 *  - 1 = float
 *  - 2 = int64_t
 *  - 3 = int32_t
 *  Type should be selected depending on the target CPU where it will be used and performance/memory constrains.
 *  For example if CPU support double type integrator wants to get max. telemetry samples precision - double it a good
 *  candidate for telemetry sample. 
 *  If floating point isn't supported or int32_t is enough for telemetry needs - int32_t may be used, etc.
*/
#define UP7_PRECISE_TYPE_INDEX 0


#if   UP7_PRECISE_TYPE_INDEX == 0
    #if defined(UP7_FLOATING_POINT)
        typedef struct stuP7telF8Hdr stuP7telHdr;
        #define UP7_PRECISE_TYPE double
    #endif
#elif UP7_PRECISE_TYPE_INDEX == 1
    #if defined(UP7_FLOATING_POINT)
        typedef struct stuP7telF4Hdr stuP7telHdr;
        #define UP7_PRECISE_TYPE float
    #endif
#elif UP7_PRECISE_TYPE_INDEX == 2
    typedef struct stuP7telI8Hdr stuP7telHdr;
    #define UP7_PRECISE_TYPE int64_t
#elif UP7_PRECISE_TYPE_INDEX == 3
    typedef struct stuP7telI4Hdr stuP7telHdr;
    #define UP7_PRECISE_TYPE int32_t
#else
    char assert_unknown_telemetry_type[-1];
#endif


/*! Activate this define in case if uP7 should use locks to protect memory from multi-threading or ISR access */
//#define UP7_LOCK_AVAILABLE



#endif //UP7_PLATFORM_H