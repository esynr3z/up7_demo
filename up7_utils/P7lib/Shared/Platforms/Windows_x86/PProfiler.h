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

#include "Ticks.h"


class CTickHi
{
    LARGE_INTEGER m_qwCounter;
    LARGE_INTEGER m_qwMarkIn;
    LARGE_INTEGER m_qwMarkOut;
    LARGE_INTEGER m_qwResolution;
public:
    CTickHi()
    {
        m_qwMarkIn.QuadPart = 0;
        m_qwMarkOut.QuadPart = 0;
        
        QueryPerformanceFrequency(&m_qwResolution);

        Reset();
        Start();
    }

    void Start()
    {
        QueryPerformanceCounter(&m_qwMarkIn);
    }

    void Stop()
    {
        QueryPerformanceCounter(&m_qwMarkOut);
        m_qwCounter.QuadPart += m_qwMarkOut.QuadPart - m_qwMarkIn.QuadPart;
    }

    void Reset()
    {
        m_qwCounter.QuadPart = 0;
    }

    //Second == 100 000;
    DWORD Get() 
    {
        return (DWORD)(m_qwCounter.QuadPart * 100000ULL / m_qwResolution.QuadPart);
    }
};

class CTickLow
{
    DWORD m_dwStart;
    DWORD m_dwStop;
public:
    CTickLow()
    {
        m_dwStart = 0;
        m_dwStop = 0;

        Reset();
        Start();
    }

    void Start()
    {
        m_dwStart = GetTickCount();
    }

    void Stop()
    {
        m_dwStop = GetTickCount();
    }

    void Reset()
    {
        m_dwStop = 0;
    }

    //frequency 1 000 per second
    DWORD Get() 
    {
        return CTicks::Difference(m_dwStop, m_dwStart);
    }
};

