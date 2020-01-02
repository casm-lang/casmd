//
//  Copyright (C) 2017-2020 CASM Organization <https://casm-lang.org>
//  All rights reserved.
//
//  Developed by: Philipp Paulweber
//                Emmanuel Pescosta
//                <https://github.com/casm-lang/casmd>
//
//  This file is part of casmd.
//
//  casmd is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  casmd is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with casmd. If not, see <http://www.gnu.org/licenses/>.
//

#include "LanguageServer.h"
#include "casmd/Version"

#include <libpass/PassManager>
#include <libstdhl/Args>
#include <libstdhl/Memory>
#include <libstdhl/String>
#include <libstdhl/net/tcp/IPv4>

/**
    @brief TODO

    TODO
*/

static constexpr const char* MODE = "mode";
static constexpr const char* MODE_LSP = "lsp";

static constexpr const char* CONN = "connection";
static constexpr const char* CONN_TCP4 = "tcp4";
static constexpr const char* CONN_STDIO = "stdio";

int main( int argc, const char* argv[] )
{
    libpass::PassManager pm;
    libstdhl::Logger log( pm.stream() );
    log.setSource(
        libstdhl::Memory::make< libstdhl::Log::Source >( argv[ 0 ], casmd::DESCRIPTION ) );

    auto flush = [&argv, &pm]( void ) {
        libstdhl::Log::ApplicationFormatter f( argv[ 0 ] );
        libstdhl::Log::OutputStreamSink c( std::cerr, f );
        pm.stream().flush( c );
    };

    std::unordered_map< std::string, std::vector< std::string > > setting;

    libstdhl::Args options( argc, argv, libstdhl::Args::DEFAULT, [&]( const char* arg ) {
        if( strcmp( arg, MODE_LSP ) == 0 )
        {
            if( setting[ MODE ].size() > 0 )
            {
                log.error(
                    "already defined mode '" + setting[ MODE ].front() +
                    "', unable to use additional provided mode '" + std::string( arg ) + "'" );
                return -1;
            }

            setting[ MODE ].emplace_back( arg );
        }
        else
        {
            log.error(
                "invalid mode '" + std::string( arg ) +
                "' found, please refer to --help for more information" );
            return 1;
        }

        return 0;
    } );

    options.add(
        't',
        "test-case-profile",
        libstdhl::Args::NONE,
        "display the unique test profile identifier",
        [&]( const char* ) {
            std::cout << casmd::PROFILE << "\n";
            return -1;
        } );

    options.add(
        'h', "help", libstdhl::Args::NONE, "display usage and synopsis", [&]( const char* ) {
            log.output(
                "\n" + std::string( casmd::DESCRIPTION ) + "\n" + log.source()->name() +
                ": usage: [options] mode\n" + "\n" + "mode: \n" +
                "  lsp                            language server protocol\n" + "\n" +
                "options: \n" + options.usage() + "\n" );

            return -1;
        } );

    options.add(
        'v', "version", libstdhl::Args::NONE, "display version information", [&]( const char* ) {
            log.output(
                "\n" + std::string( casmd::DESCRIPTION ) + "\n" + log.source()->name() +
                ": version: " + casmd::REVTAG + " [ " + __DATE__ + " " + __TIME__ + " ]\n" + "\n" +
                casmd::NOTICE );

            return -1;
        } );

    options.add(
        CONN_TCP4,
        libstdhl::Args::REQUIRED,
        "use a TCP IPv4 socket stream connection",
        [&]( const char* arg ) {
            setting[ CONN ].emplace_back( CONN_TCP4 );
            setting[ CONN_TCP4 ].emplace_back( arg );
            return 0;
        },
        "host:port" );

    options.add(
        CONN_STDIO,
        libstdhl::Args::NONE,
        "use a standard input/output stream",
        [&]( const char* arg ) {
            setting[ CONN ].emplace_back( CONN_STDIO );
            setting[ CONN_STDIO ].emplace_back( arg );
            return 0;
        } );

    if( auto ret = options.parse( log ) )
    {
        flush();

        if( ret >= 0 )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    if( setting[ MODE ].size() != 1 )
    {
        log.error( "no mode provided, please see --help for more information" );
        flush();
        return 2;
    }

    if( setting[ CONN ].size() > 1 )
    {
        log.error( "to many connection kinds provided, only use one kind" );
        flush();
        return 2;
    }

    if( setting[ CONN ].size() == 0 )
    {
        log.info( "no connection provided, using '--stdio'" );
        flush();
        setting[ CONN ].emplace_back( CONN_STDIO );
    }

    const auto& mode = setting[ MODE ].front();
    const auto& conn = setting[ CONN ].front();
    const auto& kind = setting[ conn ].front();
    const auto prefix = mode + "@" + conn + ": ";

    switch( String::value( mode ) )
    {
        case String::value( MODE_LSP ):
        {
            // language server protocol mode
            casmd::LanguageServer server( log );

            switch( String::value( conn ) )
            {
                case String::value( CONN_TCP4 ):
                {
                    auto iface = libstdhl::Network::TCP::IPv4( kind, true );
                    iface.connect();
                    log.info( "connected to '" + kind + "'" );
                    flush();

                    auto session = iface.session();
                    log.info( "starting new TCP::IPv4 session" );
                    flush();

                    while( true )
                    {
                        const auto message = session.receive();
                        const auto request = libstdhl::Network::LSP::Packet::parse( message );
                        log.info( prefix + "REQ: " + request.dump( true ) + "\n" );
                        flush();

                        try
                        {
                            request.process( server );
                        }
                        catch( const std::exception& e )
                        {
                            log.error( e.what() );
                        }

                        server.flush( [&]( const libstdhl::Network::LSP::Message& response ) {
                            log.info( prefix + "ACK: " + response.dump( true ) + "\n" );
                            flush();
                            const auto packet = libstdhl::Network::LSP::Packet( response );
                            session.send( packet.dump() );
                        } );
                    }

                    session.disconnect();
                    iface.disconnect();
                    break;
                }
                case String::value( CONN_STDIO ):
                {
                    std::ios_base::sync_with_stdio( false );
                    log.info( "starting new STDIO session" );
                    flush();

                    std::string buffer;
                    buffer.reserve( 4096 );
                    while( true )
                    {
                        buffer = "";
                        while( true )
                        {
                            std::string tmp;
                            std::getline( std::cin, tmp );
                            buffer += tmp + "\n";
                            if( String::endsWith( buffer, "\r\n\r\n" ) )
                            {
                                break;
                            }
                        }

                        libstdhl::Network::LSP::Protocol header( 0 );
                        libstdhl::Network::LSP::Message payload;

                        log.info( prefix + "REQ: HEADER: " + buffer + "\n" );
                        flush();
                        try
                        {
                            header = libstdhl::Network::LSP::Protocol::parse( buffer );
                        }
                        catch( const std::exception& e )
                        {
                            log.error( e.what() );
                            flush();
                            continue;
                        }

                        const auto length = header.length();
                        if( buffer.length() < length )
                        {
                            buffer.resize( length );
                        }
                        std::cin.read( &buffer[ 0 ], length );
                        buffer[ length ] = '\0';

                        log.info( prefix + "REQ: CONTENT: " + buffer + "\n" );
                        flush();

                        try
                        {
                            payload = libstdhl::Network::LSP::Message::parse( buffer );
                        }
                        catch( const std::exception& e )
                        {
                            log.error( e.what() );
                            flush();
                            continue;
                        }

                        libstdhl::Network::LSP::Packet request( header, payload );
                        log.info( prefix + "REQ: " + request.dump( true ) + "\n" );
                        flush();

                        try
                        {
                            request.process( server );
                        }
                        catch( const std::exception& e )
                        {
                            log.error( e.what() );
                        }

                        server.flush( [&]( const libstdhl::Network::LSP::Message& response ) {
                            const auto packet = libstdhl::Network::LSP::Packet( response );
                            std::cout << packet.dump() << std::flush;
                            log.info( prefix + "ACK: " + packet.dump( true ) + "\n" );
                            flush();
                        } );
                    }
                    break;
                }
                default:
                {
                    log.error( "unimplemented connection '" + conn + "' selected, aborting" );
                    flush();
                    return -1;
                }
            }
            break;
        }
        default:
        {
            log.error( "unimplemented mode '" + mode + "' selected, aborting" );
            flush();
            return -1;
        }
    }

    flush();
    return 0;
}

//
//  Local variables:
//  mode: c++
//  indent-tabs-mode: nil
//  c-basic-offset: 4
//  tab-width: 4
//  End:
//  vim:noexpandtab:sw=4:ts=4:
//
