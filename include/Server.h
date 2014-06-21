/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "Packets.h"
#include "Connection.h"
#include "NetworkInterface.h"

namespace protocol
{
    struct ServerConfig
    {
        int maxClients = 16;                                    // max number of clients supported by this server.

        float connectingSendRate = 10;                          // packets to send per-second while a client slot is connecting.
        float connectedSendRate = 30;                           // packets to send per-second once a client is connected.

        float connectingTimeOut = 5.0f;                         // timeout in seconds while a client is connecting
        float connectedTimeOut = 10.0f;                         // timeout in seconds once a client is connected

        NetworkInterface * networkInterface = nullptr;          // network interface used to send and receive packets
        ChannelStructure * channelStructure = nullptr;          // defines the connection channel structure
        
        Block * block = nullptr;                                // data block sent to clients on connect. must be constant, eg. protocol is a function of this
    };

    class Server
    {
        struct ClientData
        {
            Address address;                                        // the client address that started this connection.
            double accumulator;                                     // accumulator used to determine when to send next packet.
            double lastPacketTime;                                  // time at which the last valid packet was received from the client. used for timeouts.
            uint64_t clientGuid;                                    // the client guid generated by the client and sent to us via connect request.
            uint64_t serverGuid;                                    // the server guid generated randomly on connection request unique to this client.
            ServerClientState state;                                // the current state of this client slot.
            Connection * connection;                                // connection object, once in SERVER_CLIENT_Connected state this becomes active.

            ClientData()
            {
                accumulator = 0;
                lastPacketTime = 0;
                clientGuid = 0;
                serverGuid = 0;
                state = SERVER_CLIENT_STATE_DISCONNECTED;
                connection = nullptr;
            }
        };

        const ServerConfig m_config;

        TimeBase m_timeBase;

        bool m_open = true;

        int m_numClients = 0;
        ClientData * m_clients = nullptr;

        Factory<Packet> * m_packetFactory = nullptr;

    public:

        Server( const ServerConfig & config );

        ~Server();

        void Open();

        void Close();

        bool IsOpen() const;

        void Update( const TimeBase & timeBase );

        void DisconnectClient( int clientIndex );

        ServerClientState GetClientState( int clientIndex ) const;

        Connection * GetClientConnection( int clientIndex );

        void UpdateClients();

        void UpdateSendingChallenge( int clientIndex );

        void UpdateSendingServerData( int clientIndex );

        void UpdateRequestingClientData( int clientIndex );

        void UpdateReceivingClientData( int clientIndex );

        void UpdateConnected( int clientIndex );

        void UpdateTimeouts( int clientIndex );

        void UpdateNetworkInterface();

        void UpdateReceivePackets();

        void ProcessConnectionRequestPacket( ConnectionRequestPacket * packet );

        void ProcessChallengeResponsePacket( ChallengeResponsePacket * packet );

        void ProcessReadyForConnectionPacket( ReadyForConnectionPacket * packet );

        void ProcessConnectionPacket( ConnectionPacket * packet );

        int FindClientIndex( const Address & address ) const;

        int FindClientIndex( const Address & address, uint64_t clientGuid ) const;

        int FindFreeClientSlot() const;

        void ResetClientSlot( int clientIndex );
    };
}

#endif
