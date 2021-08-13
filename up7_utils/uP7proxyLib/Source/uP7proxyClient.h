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
#ifndef UP7_PROXY_CLIENT_H
#define UP7_PROXY_CLIENT_H


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IProxyClient
{
public:
    virtual IP7_Client *RegisterChannel(IP7C_Channel *i_pChannel) = 0;
    virtual void        ReleaseChannel(IP7_Client *i_pClient, uint32_t i_uID) = 0;
};

#endif //UP7_PROXY_CLIENT_H