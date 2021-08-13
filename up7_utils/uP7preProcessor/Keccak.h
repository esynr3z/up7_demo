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
#ifndef KECCAK_H
#define KECCAK_H

#define KECCAK_BITRATE 1600

//typedef unsigned long long   tUINT64;
//typedef unsigned int         tUINT32;
//typedef unsigned char        tUINT8;
//typedef unsigned int         tBOOL;

#if defined(_WIN32) || defined(_WIN64)
#else
    #define _rotl64(value, shift) (((value) << (shift)) | ((value) >> (64 - (shift))))
#endif

#ifndef GET_MIN
    #define GET_MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

class CKeccak
{
private:
    size_t  m_szBits;
    size_t  m_szBitrate;
    size_t  m_szByteRate;
    tUINT64 m_pS[25];
    tUINT64 m_pRC[24];
    tUINT64 m_qwTest;
    tUINT8  m_pPadding[KECCAK_BITRATE / 8];
    size_t  m_szS_Used;

public:
    enum eBits
    {
        EBITS_224 = 224,
        EBITS_256 = 256,
        EBITS_384 = 384,
        EBITS_512 = 512
    };


    ////////////////////////////////////////////////////////////////////////////
    //CKeccak
    CKeccak(eBits i_eBits = EBITS_256)
        : m_szBits(i_eBits)
        , m_szBitrate(KECCAK_BITRATE - 2 * i_eBits)
        , m_szByteRate((KECCAK_BITRATE - 2 * i_eBits) / 8)
        , m_qwTest(0UL)
        , m_szS_Used(0)
    {
        m_pRC[0]  = 0x0000000000000001;
        m_pRC[1]  = 0x0000000000008082;
        m_pRC[2]  = 0x800000000000808A;
        m_pRC[3]  = 0x8000000080008000;
        m_pRC[4]  = 0x000000000000808B; 
        m_pRC[5]  = 0x0000000080000001;
        m_pRC[6]  = 0x8000000080008081; 
        m_pRC[7]  = 0x8000000000008009;
        m_pRC[8]  = 0x000000000000008A;
        m_pRC[9]  = 0x0000000000000088;
        m_pRC[10] = 0x0000000080008009;
        m_pRC[11] = 0x000000008000000A;
        m_pRC[12] = 0x000000008000808B; 
        m_pRC[13] = 0x800000000000008B; 
        m_pRC[14] = 0x8000000000008089;
        m_pRC[15] = 0x8000000000008003; 
        m_pRC[16] = 0x8000000000008002; 
        m_pRC[17] = 0x8000000000000080;
        m_pRC[18] = 0x000000000000800A;
        m_pRC[19] = 0x800000008000000A;
        m_pRC[20] = 0x8000000080008081;
        m_pRC[21] = 0x8000000000008080;
        m_pRC[22] = 0x0000000080000001; 
        m_pRC[23] = 0x8000000080008008;

        Clear();
    }//CKeccak


    ////////////////////////////////////////////////////////////////////////////
    //Clear
    void Clear()
    {
        memset(m_pS, 0, sizeof(m_pS));
        memset(m_pPadding, 0, sizeof(m_pPadding));
        
        m_szS_Used = 0;
    }//Clear


    ////////////////////////////////////////////////////////////////////////////
    //Get_Block_Size
    size_t Get_Block_Size() const
    { 
        return m_szByteRate; 
    }//Get_Block_Size


    ////////////////////////////////////////////////////////////////////////////
    //Get_Length
    size_t Get_Length() const
    {
        return m_szBits / 8; 
    }//Get_Length


    ////////////////////////////////////////////////////////////////////////////
    //Update
    //Function implements standard but it isn't fast, use only with Get_HashA
    tBOOL UpdateA(const tUINT8 *i_pData, size_t l_szLen)
    {
        if (l_szLen == 0)
        {
            return FALSE;
        }

        while (l_szLen)
        {
            size_t l_szTo_Take = GET_MIN(l_szLen, m_szByteRate - m_szS_Used);

            l_szLen -= l_szTo_Take;

            //printA("Step0", m_pS, 25);

            while (l_szTo_Take && m_szS_Used % 8)
            {
                m_pS[m_szS_Used / 8] ^= static_cast<tUINT64>(i_pData[0]) << (8 * (m_szS_Used % 8));

                ++m_szS_Used;
                ++i_pData;
                --l_szTo_Take;
            }


            while (l_szTo_Take && l_szTo_Take % 8 == 0)
            {
                m_pS[m_szS_Used / 8] ^= Get_Little_Endian(i_pData);
                m_szS_Used += 8;
                i_pData += 8;
                l_szTo_Take -= 8;
            }


            while (l_szTo_Take)
            {
                m_pS[m_szS_Used / 8] ^= static_cast<tUINT64>(i_pData[0]) << (8 * (m_szS_Used % 8));

                ++m_szS_Used;
                ++i_pData;
                --l_szTo_Take;
            }

            if (m_szS_Used == m_szByteRate)
            {
                keccak_f_1600();
                m_szS_Used = 0;
            }
        }//while (l_szLen)
    }//UpdateA


    ////////////////////////////////////////////////////////////////////////////
    //Get_HashA
    //Function implement standard but it isn't fast, use only with UpdateA
    size_t Get_HashA(tUINT8 *o_pHash, size_t i_szLength)
    {
        size_t l_szCount = m_szByteRate - m_szS_Used;

        if (l_szCount)
        {
            memset(m_pPadding, 0, sizeof(m_pPadding));

            m_pPadding[0] = 0x01;
            m_pPadding[l_szCount - 1] |= 0x80;

            UpdateA(m_pPadding, l_szCount);
        }

        l_szCount = m_szBits / 8;

        if (i_szLength < l_szCount)
        {
            l_szCount = i_szLength;
        }
        /*
        * We never have to run the permutation again because we only support
        * limited output lengths
        */
        for (size_t i = 0; i != l_szCount; ++i)
        {
            o_pHash[i] = Get_Byte(7 - (i % 8), m_pS[i / 8]);
        }

        Clear();

        return l_szCount;
    }//Get_Hash


    ////////////////////////////////////////////////////////////////////////////
    //UpdateB
    //Function do not implement standard exactly but is is faster than UpdateA
    //and provide the same hash quality, use only with Get_HashB
    tBOOL UpdateB(const tUINT8 *i_pData, size_t i_szLength)
    {
        if (0 == i_szLength)
        {
            return FALSE;
        }

        if (m_szS_Used)
        {
            size_t l_szS_Free = m_szByteRate - m_szS_Used;

            if (i_szLength < l_szS_Free)
            {
                l_szS_Free = i_szLength;
            }

            Xor_Line8(((tUINT8*)m_pS) + m_szS_Used, i_pData, l_szS_Free);

            m_szS_Used += l_szS_Free;
            i_pData    += l_szS_Free;
            i_szLength -= l_szS_Free;

            if (m_szS_Used >= m_szByteRate)
            {
                m_szS_Used = 0;
                keccak_f_1600();
            }
        }

        while (i_szLength >= m_szByteRate)
        {
            Xor_Line64(m_pS, (tUINT64*)i_pData, m_szByteRate / 8);
            keccak_f_1600();
            i_szLength -= m_szByteRate;
            i_pData    += m_szByteRate;
        }

        if (i_szLength)
        {
            Xor_Line8(((tUINT8*)m_pS) + m_szS_Used, i_pData, i_szLength);
            m_szS_Used = i_szLength;
        }

        return TRUE;
    }//UpdateB


    ////////////////////////////////////////////////////////////////////////////
    //Get_HashB
    //Function do not implement standard exactly but is is faster than UpdateA
    //and provide the same hash quality, use only with UpdateB
    void Get_HashB(tUINT8 *o_pHash, size_t i_szLength)
    {
        //size_t l_szPadding = m_szByteRate - m_szS_Used;

        if (m_szS_Used)
        {
            ((tUINT8*)m_pS)[m_szS_Used]       ^= 1;
            ((tUINT8*)m_pS)[m_szByteRate - 1] ^= 80;

            keccak_f_1600();
        }

        while (i_szLength)
        {
            if (i_szLength > m_szByteRate)
            {
                memcpy(o_pHash, m_pS, m_szByteRate);
                i_szLength -= m_szByteRate;
                o_pHash    += m_szByteRate;
                keccak_f_1600();
            }
            else
            {
                memcpy(o_pHash, m_pS, i_szLength);
                i_szLength = 0;
            }
        }

        Clear();
    }//Get_HashB


private:
    ////////////////////////////////////////////////////////////////////////////
    //Get_Little_Endian
    tUINT64 Get_Little_Endian(const tUINT8 *i_pData)
    {
        tUINT64 l_qwOut = 0;
        for (size_t l_szI = 0; l_szI != sizeof(l_qwOut); ++l_szI)
        {
            l_qwOut = (l_qwOut << 8) | i_pData[sizeof(l_qwOut) - 1 - l_szI];
        }

        return l_qwOut;
    }//Get_Little_Endian


    ////////////////////////////////////////////////////////////////////////////
    //Get_Byte
    tUINT8 Get_Byte(size_t i_szIndex, tUINT64 i_qwValue)
    {
        return static_cast<tUINT8>(i_qwValue >> ( (sizeof(i_qwValue) - 1 - (i_szIndex & (sizeof(i_qwValue) - 1)) ) << 3));
    }//Get_Byte


    ////////////////////////////////////////////////////////////////////////////
    //Xor_Line8
    void Xor_Line8(tUINT8 *io_pDest, const tUINT8 *i_pSrc, size_t i_szCount)
    {
        while (i_szCount--)
        {
            *(io_pDest++) ^= *(i_pSrc++);
        }
    }//Xor_Line8


    ////////////////////////////////////////////////////////////////////////////
    //Xor_Line64
    void Xor_Line64(tUINT64 *io_pDest, const tUINT64 *i_pSrc, size_t i_szCount)
    {
        while (i_szCount--)
        {
            *(io_pDest++) ^= *(i_pSrc++);
        }
    }//Xor_Line64


    ////////////////////////////////////////////////////////////////////////////
    //keccak_f_1600
    void keccak_f_1600()
    {
        for (size_t i = 0; i != 24; ++i)
        {
            const tUINT64 C0 = m_pS[0] ^ m_pS[5] ^ m_pS[10] ^ m_pS[15] ^ m_pS[20];
            const tUINT64 C1 = m_pS[1] ^ m_pS[6] ^ m_pS[11] ^ m_pS[16] ^ m_pS[21];
            const tUINT64 C2 = m_pS[2] ^ m_pS[7] ^ m_pS[12] ^ m_pS[17] ^ m_pS[22];
            const tUINT64 C3 = m_pS[3] ^ m_pS[8] ^ m_pS[13] ^ m_pS[18] ^ m_pS[23];
            const tUINT64 C4 = m_pS[4] ^ m_pS[9] ^ m_pS[14] ^ m_pS[19] ^ m_pS[24]; 

            const tUINT64 D0 = C4 ^ _rotl64(C1, 1);
            const tUINT64 D1 = C0 ^ _rotl64(C2, 1);
            const tUINT64 D2 = C1 ^ _rotl64(C3, 1);
            const tUINT64 D3 = C2 ^ _rotl64(C4, 1);
            const tUINT64 D4 = C3 ^ _rotl64(C0, 1);


            const tUINT64 B00 = m_pS[00] ^ D0;
            const tUINT64 B01 = _rotl64(m_pS[6] ^ D1, 44);
            const tUINT64 B02 = _rotl64(m_pS[12] ^ D2, 43);
            const tUINT64 B03 = _rotl64(m_pS[18] ^ D3, 21);
            const tUINT64 B04 = _rotl64(m_pS[24] ^ D4, 14);
            const tUINT64 B05 = _rotl64(m_pS[3] ^ D3, 28);
            const tUINT64 B06 = _rotl64(m_pS[9] ^ D4, 20);
            const tUINT64 B07 = _rotl64(m_pS[10] ^ D0, 3);
            const tUINT64 B08 = _rotl64(m_pS[16] ^ D1, 45);
            const tUINT64 B09 = _rotl64(m_pS[22] ^ D2, 61);
            const tUINT64 B10 = _rotl64(m_pS[1] ^ D1, 1);
            const tUINT64 B11 = _rotl64(m_pS[7] ^ D2, 6);
            const tUINT64 B12 = _rotl64(m_pS[13] ^ D3, 25);
            const tUINT64 B13 = _rotl64(m_pS[19] ^ D4, 8);
            const tUINT64 B14 = _rotl64(m_pS[20] ^ D0, 18);
            const tUINT64 B15 = _rotl64(m_pS[4] ^ D4, 27);
            const tUINT64 B16 = _rotl64(m_pS[5] ^ D0, 36);
            const tUINT64 B17 = _rotl64(m_pS[11] ^ D1, 10);
            const tUINT64 B18 = _rotl64(m_pS[17] ^ D2, 15);
            const tUINT64 B19 = _rotl64(m_pS[23] ^ D3, 56);
            const tUINT64 B20 = _rotl64(m_pS[2] ^ D2, 62);
            const tUINT64 B21 = _rotl64(m_pS[8] ^ D3, 55);
            const tUINT64 B22 = _rotl64(m_pS[14] ^ D4, 39);
            const tUINT64 B23 = _rotl64(m_pS[15] ^ D0, 41);
            const tUINT64 B24 = _rotl64(m_pS[21] ^ D1, 2);

            m_pS[0] = B00 ^ (~B01 & B02);
            m_pS[1] = B01 ^ (~B02 & B03);
            m_pS[2] = B02 ^ (~B03 & B04);
            m_pS[3] = B03 ^ (~B04 & B00);
            m_pS[4] = B04 ^ (~B00 & B01);
            m_pS[5] = B05 ^ (~B06 & B07);
            m_pS[6] = B06 ^ (~B07 & B08);
            m_pS[7] = B07 ^ (~B08 & B09);
            m_pS[8] = B08 ^ (~B09 & B05);
            m_pS[9] = B09 ^ (~B05 & B06);
            m_pS[10] = B10 ^ (~B11 & B12);
            m_pS[11] = B11 ^ (~B12 & B13);
            m_pS[12] = B12 ^ (~B13 & B14);
            m_pS[13] = B13 ^ (~B14 & B10);
            m_pS[14] = B14 ^ (~B10 & B11);
            m_pS[15] = B15 ^ (~B16 & B17);
            m_pS[16] = B16 ^ (~B17 & B18);
            m_pS[17] = B17 ^ (~B18 & B19);
            m_pS[18] = B18 ^ (~B19 & B15);
            m_pS[19] = B19 ^ (~B15 & B16);
            m_pS[20] = B20 ^ (~B21 & B22);
            m_pS[21] = B21 ^ (~B22 & B23);
            m_pS[22] = B22 ^ (~B23 & B24);
            m_pS[23] = B23 ^ (~B24 & B20);
            m_pS[24] = B24 ^ (~B20 & B21);

            m_pS[0] ^= m_pRC[i];
        }
    }//keccak_f_1600
}; //CKeccak




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                TESTS
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// 
// #define COUNT_B 500000
// 
// CKeccak   l_cKeccak(CKeccak::EBITS_256);
// char     *l_pIn = (char*)malloc(COUNT_B);
// char    **l_pOut = (char**)malloc(COUNT_B * sizeof(char*));
// tUINT64   l_qwCol = 0;
// 
// for (int l_iI = 0; l_iI < COUNT_B; l_iI++)
// {
//     l_pIn[l_iI] = (char)rand();
// 
//     l_pOut[l_iI] = (char*)malloc(CKeccak::EBITS_256 / 8);
//     if (l_pOut[l_iI])
//     {
//         memset(l_pOut[l_iI], 0, CKeccak::EBITS_256 / 8);
//     }
// }
// 
// ////////////////////////////////////////////////////////////////////////////
// 
// printf("Calculate hash B\n");
// for (int l_iI = 0; l_iI < COUNT_B; l_iI++)
// {
//     l_cKeccak.UpdateB((const tUINT8 *)l_pIn, l_iI);
//     l_cKeccak.Get_HashB((tUINT8 *)l_pOut[l_iI], l_cKeccak.Get_Length());
// 
//     if (0 == (l_iI % 10000))
//     {
//         printf("%d%,", l_iI / 10000);
//     }
// }
// 
// l_qwCol = 0;
// printf("\nCheck for collisions hash B\n");
// for (int l_iI = 0; l_iI < COUNT_B; l_iI++)
// {
//     for (int l_iJ = 0; l_iJ < COUNT_B; l_iJ++)
//     {
//         if ((l_iI != l_iJ)
//             && (0 == memcmp(l_pOut[l_iI], l_pOut[l_iJ], l_cKeccak.Get_Length()))
//             )
//         {
//             l_qwCol++;
//         }
//     }
// 
//     if (0 == (l_iI % 10000))
//     {
//         printf("%d%,", l_iI / 10000);
//     }
// }
// 
// printf("\nCollisionsB = %I64d\n", l_qwCol);
// 
// 
// ////////////////////////////////////////////////////////////////////////////
// printf("Calculate hash A\n");
// for (int l_iI = 0; l_iI < COUNT_B; l_iI++)
// {
//     l_cKeccak.UpdateA((const tUINT8 *)l_pIn, l_iI);
//     l_cKeccak.Get_HashA((tUINT8 *)l_pOut[l_iI], l_cKeccak.Get_Length());
// 
//     if (0 == (l_iI % 10000))
//     {
//         printf("%d%,", l_iI / 10000);
//     }
// }
// 
// printf("\nCheck for collisions hash A\n");
// for (int l_iI = 0; l_iI < COUNT_B; l_iI++)
// {
//     for (int l_iJ = 0; l_iJ < COUNT_B; l_iJ++)
//     {
//         if ((l_iI != l_iJ)
//             && (0 == memcmp(l_pOut[l_iI], l_pOut[l_iJ], l_cKeccak.Get_Length()))
//             )
//         {
//             l_qwCol++;
//         }
//     }
// 
//     if (0 == (l_iI % 10000))
//     {
//         printf("%d%,", l_iI / 10000);
//     }
// }
// 
// printf("\nCollisionsA = %I64d\n", l_qwCol);


#endif //KECCAK_H