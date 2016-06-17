/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include "core/Core.h"
#include "network/Address.h"
#include "protocol/Stream.h"     // todo: don't depend on protocol stuff from network!
#include "protocol/Packet.h"     // todo: don't depend on protocol stuff from network!

namespace network
{
    class Interface
    {
    public:

        virtual ~Interface() {}

        virtual void SendPacket( const Address & address, protocol::Packet * packet ) = 0;        // takes ownership of packet pointer

        virtual protocol::Packet * ReceivePacket() = 0;                                           // gives you ownership of packet pointer

        virtual void Update( const core::TimeBase & timeBase ) = 0;

        // todo: really can't be doing this stuff at the network interface level!
        virtual uint32_t GetMaxPacketSize() const = 0;
        virtual protocol::PacketFactory & GetPacketFactory() const = 0;
        virtual void SetContext( const void ** context ) = 0;
    };
}

#endif
