#include "protocol/Connection.h"
#include "protocol/ReliableMessageChannel.h"
#include "network/Simulator.h"
#include "TestMessages.h"
#include "TestPackets.h"
#include "TestChannelStructure.h"

void test_reliable_message_channel_messages()
{
    printf( "test_reliable_message_channel_messages\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );

        const void * context[protocol::MaxContexts];
        memset( context, 0, sizeof( context ) );
        context[protocol::CONTEXT_CONNECTION] = &channelStructure;

        {
            const int MaxPacketSize = 256;

            protocol::ConnectionConfig connectionConfig;
            connectionConfig.maxPacketSize = MaxPacketSize;
            connectionConfig.packetFactory = &packetFactory;
            connectionConfig.channelStructure = &channelStructure;

            protocol::Connection connection( connectionConfig );

            protocol::ReliableMessageChannel * messageChannel = static_cast<protocol::ReliableMessageChannel*>( connection.GetChannel( 0 ) );
            
            const int NumMessagesSent = 32;

            for ( int i = 0; i < NumMessagesSent; ++i )
            {
                TestMessage * message = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
                CORE_CHECK( message );
                message->sequence = i;
                messageChannel->SendMessage( message );
            }

            core::TimeBase timeBase;
            timeBase.deltaTime = 0.01f;

            uint64_t numMessagesReceived = 0;

            network::Address address( "::1" );

            network::SimulatorConfig simulatorConfig;
            simulatorConfig.packetFactory = &packetFactory;
            network::Simulator simulator( simulatorConfig );
            simulator.SetContext( context );
            simulator.AddState( network::SimulatorState( 1.0f, 1.0f, 25 ) );

            int iteration = 0;

            while ( true )
            {  
                protocol::ConnectionPacket * writePacket = connection.WritePacket();
                CORE_CHECK( writePacket );
                CORE_CHECK( writePacket->GetType() == PACKET_CONNECTION );

                simulator.SendPacket( address, writePacket );
                writePacket = nullptr;

                simulator.Update( timeBase );

                protocol::Packet * packet = simulator.ReceivePacket();

                if ( packet )
                {
                    CORE_CHECK( packet->GetType() == PACKET_CONNECTION );
                    connection.ReadPacket( static_cast<protocol::ConnectionPacket*>( packet ) );
                    packetFactory.Destroy( packet );
                    packet = nullptr;
                }

                CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_READ ) <= uint64_t( iteration + 1 ) );
                CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_WRITTEN ) == uint64_t( iteration + 1 ) );
                CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_ACKED ) <= uint64_t( iteration + 1 ) );

                while ( true )
                {
                    protocol::Message * message = messageChannel->ReceiveMessage();

                    if ( !message )
                        break;

                    CORE_CHECK( message->GetId() == (int) numMessagesReceived );
                    CORE_CHECK( message->GetType() == MESSAGE_TEST );

                    TestMessage * testMessage = static_cast<TestMessage*>( message );

                    CORE_CHECK( testMessage->sequence == numMessagesReceived );

                    ++numMessagesReceived;

                    messageFactory.Release( message );
                }

                if ( numMessagesReceived == NumMessagesSent )
                    break;

                connection.Update( timeBase );

                CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
                CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
                CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

                timeBase.time += timeBase.deltaTime;

                if ( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent )
                    break;

                iteration++;
            }
        }
    }
    core::memory::shutdown();
}

void test_reliable_message_channel_small_blocks()
{
    printf( "test_reliable_message_channel_small_blocks\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );
        
        const void * context[protocol::MaxContexts];
        memset( context, 0, sizeof( context ) );
        context[protocol::CONTEXT_CONNECTION] = &channelStructure;

        const int MaxPacketSize = 256;

        protocol::ConnectionConfig connectionConfig;
        connectionConfig.maxPacketSize = MaxPacketSize;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        protocol::Connection connection( connectionConfig );

        protocol::ReliableMessageChannel * messageChannel = static_cast<protocol::ReliableMessageChannel*>( connection.GetChannel( 0 ) );

        const protocol::ReliableMessageChannelConfig & channelConfig = channelStructure.GetConfig(); 

        const int NumMessagesSent = channelConfig.maxSmallBlockSize;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            protocol::Block block( core::memory::default_allocator(), i + 1 );
            uint8_t * data = block.GetData();
            for ( int j = 0; j < block.GetSize(); ++j )
                data[j] = ( i + j ) % 256;
            messageChannel->SendBlock( block );
        }

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        uint64_t numMessagesReceived = 0;

        int iteration = 0;

        network::Address address( "::1" );

        network::SimulatorConfig simulatorConfig;
        simulatorConfig.packetFactory = &packetFactory;
        network::Simulator simulator( simulatorConfig );
        simulator.SetContext( context );
        simulator.AddState( network::SimulatorState( 1.0f, 1.0f, 25 ) );

        while ( true )
        {
            protocol::ConnectionPacket * writePacket = connection.WritePacket();
            CORE_CHECK( writePacket );
            CORE_CHECK( writePacket->GetType() == PACKET_CONNECTION );

            simulator.SendPacket( address, writePacket );
            writePacket = nullptr;

            simulator.Update( timeBase );

            protocol::Packet * packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<protocol::ConnectionPacket*>( packet ) );
                packetFactory.Destroy( packet );
                packet = nullptr;
            }

            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_READ ) <= uint64_t( iteration + 1 ) );
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_WRITTEN ) == uint64_t( iteration + 1 ) );
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_ACKED ) <= uint64_t( iteration + 1 ) );

            while ( true )
            {
                protocol::Message * message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == (int) numMessagesReceived );
                CORE_CHECK( message->GetType() == MESSAGE_BLOCK );

                protocol::BlockMessage * blockMessage = static_cast<protocol::BlockMessage*>( message );

                protocol::Block & block = blockMessage->GetBlock();

                CORE_CHECK( block.GetSize() == int( numMessagesReceived + 1 ) );
                const uint8_t * data = block.GetData();
                for ( int i = 0; i < block.GetSize(); ++i )
                    CORE_CHECK( data[i] == ( numMessagesReceived + i ) % 256 );

                ++numMessagesReceived;

                messageFactory.Release( message );
            }

            connection.Update( timeBase );

            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == uint64_t( NumMessagesSent ) );
            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == uint64_t( numMessagesReceived ) );
            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            if ( numMessagesReceived == uint64_t( NumMessagesSent ) )
                break;

            timeBase.time += timeBase.deltaTime;

            iteration++;
        }
    }
    core::memory::shutdown();
}

void test_reliable_message_channel_large_blocks()
{
    printf( "test_reliable_message_channel_large_blocks\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );
        
        const void * context[protocol::MaxContexts];
        memset( context, 0, sizeof( context ) );
        context[protocol::CONTEXT_CONNECTION] = &channelStructure;

        const int MaxPacketSize = 256;

        protocol::ConnectionConfig connectionConfig;
        connectionConfig.maxPacketSize = MaxPacketSize;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        protocol::Connection connection( connectionConfig );

        protocol::ReliableMessageChannel * messageChannel = static_cast<protocol::ReliableMessageChannel*>( connection.GetChannel( 0 ) );

        const int NumMessagesSent = 16;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            protocol::Block block( core::memory::default_allocator(), ( i + 1 ) * 64 + i );
            uint8_t * data = block.GetData();
            for ( int j = 0; j < block.GetSize(); ++j )
                data[j] = ( i + j ) % 256;
            messageChannel->SendBlock( block );
        }

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        uint64_t numMessagesReceived = 0;

        int iteration = 0;

        network::Address address( "::1" );

        network::SimulatorConfig simulatorConfig;
        simulatorConfig.packetFactory = &packetFactory;
        network::Simulator simulator( simulatorConfig );
        simulator.SetContext( context );
        simulator.AddState( network::SimulatorState( 1.0f, 1.0f, 25 ) );

        while ( true )
        {  
            protocol::ConnectionPacket * writePacket = connection.WritePacket();
            CORE_CHECK( writePacket );
            CORE_CHECK( writePacket->GetType() == PACKET_CONNECTION );

            simulator.SendPacket( address, writePacket );

            simulator.Update( timeBase );

            protocol::Packet * packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<protocol::ConnectionPacket*>( packet ) );
                packetFactory.Destroy( packet );
                packet = nullptr;
            }
        
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_READ ) <= uint64_t( iteration + 1 ) );
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_WRITTEN ) == uint64_t( iteration + 1 ) );
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_ACKED ) <= uint64_t( iteration + 1 ) );

            while ( true )
            {
                protocol::Message * message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == (int) numMessagesReceived );
                CORE_CHECK( message->GetType() == MESSAGE_BLOCK );

                protocol::BlockMessage * blockMessage = static_cast<protocol::BlockMessage*>( message );

                protocol::Block & block = blockMessage->GetBlock();

//                printf( "received block %d (%d bytes)\n", blockMessage->GetId(), (int) block->size() );

                CORE_CHECK( block.GetSize() == int( ( numMessagesReceived + 1 ) * 64 + numMessagesReceived ) );
                const uint8_t * data = block.GetData();
                for ( int i = 0; i < block.GetSize(); ++i )
                    CORE_CHECK( data[i] == ( numMessagesReceived + i ) % 256 );

                ++numMessagesReceived;

                messageFactory.Release( message );
            }

            if ( numMessagesReceived == NumMessagesSent )
                break;

            connection.Update( timeBase );

            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            timeBase.time += timeBase.deltaTime;

            iteration++;
        }

        CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent );
    }
    core::memory::shutdown();
}

void test_reliable_message_channel_mixture()
{
    printf( "test_reliable_message_channel_mixture\n" );

    core::memory::initialize();
    {
        TestMessageFactory messageFactory( core::memory::default_allocator() );

        TestChannelStructure channelStructure( messageFactory );

        TestPacketFactory packetFactory( core::memory::default_allocator() );
        
        const void * context[protocol::MaxContexts];
        memset( context, 0, sizeof( context ) );
        context[protocol::CONTEXT_CONNECTION] = &channelStructure;

        const int MaxPacketSize = 256;

        protocol::ConnectionConfig connectionConfig;
        connectionConfig.maxPacketSize = MaxPacketSize;
        connectionConfig.packetFactory = &packetFactory;
        connectionConfig.channelStructure = &channelStructure;

        protocol::Connection connection( connectionConfig );

        protocol::ReliableMessageChannel * messageChannel = static_cast<protocol::ReliableMessageChannel*>( connection.GetChannel( 0 ) );

        const int NumMessagesSent = 64;

        for ( int i = 0; i < NumMessagesSent; ++i )
        {
            if ( rand() % 10 )
            {
                TestMessage * message = (TestMessage*) messageFactory.Create( MESSAGE_TEST );
                CORE_CHECK( message );
                CORE_CHECK( message->GetType() == MESSAGE_TEST );
                message->sequence = i;
                messageChannel->SendMessage( message );
            }
            else
            {
                protocol::Block block( core::memory::default_allocator(), ( i + 1 ) * 8 + i );
                uint8_t * data = block.GetData();
                for ( int j = 0; j < block.GetSize(); ++j )
                    data[j] = ( i + j ) % 256;
                messageChannel->SendBlock( block );
            }
        }

        core::TimeBase timeBase;
        timeBase.deltaTime = 0.01f;

        uint64_t numMessagesReceived = 0;

        int iteration = 0;

        network::Address address( "::1" );

        network::SimulatorConfig simulatorConfig;
        simulatorConfig.packetFactory = &packetFactory;
        network::Simulator simulator( simulatorConfig );
        simulator.SetContext( context );
        simulator.AddState( network::SimulatorState( 1.0f, 1.0f, 25 ) );

        while ( true )
        {  
            protocol::ConnectionPacket * writePacket = connection.WritePacket();
            CORE_CHECK( writePacket );
            CORE_CHECK( writePacket->GetType() == PACKET_CONNECTION );

            simulator.SendPacket( address, writePacket );

            simulator.Update( timeBase );

            protocol::Packet * packet = simulator.ReceivePacket();

            if ( packet )
            {
                connection.ReadPacket( static_cast<protocol::ConnectionPacket*>( packet ) );
                packetFactory.Destroy( packet );
                packet = nullptr;
            }
            
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_READ ) <= uint64_t( iteration + 1 ) );
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_WRITTEN ) == uint64_t( iteration + 1 ) );
            CORE_CHECK( connection.GetCounter( protocol::CONNECTION_COUNTER_PACKETS_ACKED ) <= uint64_t( iteration + 1 ) );

            while ( true )
            {
                protocol::Message * message = messageChannel->ReceiveMessage();

                if ( !message )
                    break;

                CORE_CHECK( message->GetId() == (int) numMessagesReceived );

                if ( message->GetType() == MESSAGE_BLOCK )
                {
                    CORE_CHECK( message->GetType() == MESSAGE_BLOCK );

                    protocol::BlockMessage * blockMessage = static_cast<protocol::BlockMessage*>( message );

                    protocol::Block & block = blockMessage->GetBlock();

    //                printf( "received block %d (%d bytes)\n", blockMessage->GetId(), (int) block->size() );

                    CORE_CHECK( block.GetSize() == int( ( numMessagesReceived + 1 ) * 8 + numMessagesReceived ) );
                    const uint8_t * data = block.GetData();
                    for ( int i = 0; i < block.GetSize(); ++i )
                        CORE_CHECK( data[i] == ( numMessagesReceived + i ) % 256 );
                }
                else
                {
                    CORE_CHECK( message->GetType() == MESSAGE_TEST );

    //                printf( "received message %d\n", message->GetId() );

                    TestMessage * testMessage = static_cast<TestMessage*>( message );

                    CORE_CHECK( testMessage->sequence == numMessagesReceived );
                }

                ++numMessagesReceived;

                messageFactory.Release( message );
            }

            if ( numMessagesReceived == NumMessagesSent )
                break;

            connection.Update( timeBase );

            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_SENT ) == NumMessagesSent );
            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == numMessagesReceived );
            CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_EARLY ) == 0 );

            timeBase.time += timeBase.deltaTime;

            iteration++;
        }

        CORE_CHECK( messageChannel->GetCounter( protocol::RELIABLE_MESSAGE_CHANNEL_COUNTER_MESSAGES_RECEIVED ) == NumMessagesSent );
    }
    core::memory::shutdown();
}
