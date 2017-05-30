//
//  Copyright (c) 2017 CASM Organization
//  All rights reserved.
//
//  Developed by: Philipp Paulweber
//                Emmanuel Pescosta
//                https://github.com/casm-lang/casmd
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

#include "license.h"
#include "version.h"

#include "libpass.h"
#include "libstdhl.h"

#include "../stdhl/cpp/file/TextDocument.h"
#include "../stdhl/cpp/network/Lsp.h"
#include "../stdhl/cpp/network/udp/IPv4.h"

#include "libcasm-fe.h"
#include "libcasm-ir.h"
#include "libcasm-tc.h"

/**
    @brief TODO

    TODO
*/

static const std::string DESCRIPTION
    = "Corinthian Abstract State Machine (CASM) Language "
      "Server/Service Daemon\n";

using namespace libstdhl;
using namespace Network;
using namespace LSP;

class DiagnosticFormatter : public libstdhl::Log::StringFormatter
{
  public:
    DiagnosticFormatter( const std::string& name )
    : m_name( name )
    {
    }

    std::string visit( libstdhl::Log::Data& item ) override
    {
        std::string msg = m_name + ": ";

        msg += item.level().accept( *this ) + ": ";

        u1 first = true;

        for( auto i : item.items() )
        {
            if( i->id() == libstdhl::Log::Item::ID::LOCATION )
            {
                continue;
            }

            msg += ( first ? "" : ", " ) + i->accept( *this );

            first = false;
        }

        // #ifndef NDEBUG
        //         // auto tsp = item.timestamp().accept( *this );
        //         auto src = item.source()->accept( *this );
        //         auto cat = item.category()->accept( *this );
        //         msg += " [" + /*tsp +*/ src + ", " + cat + "]";
        // #endif

        std::string result = msg;

        for( auto i : item.items() )
        {
            if( i->id() == libstdhl::Log::Item::ID::LOCATION )
            {
                result += "\n" + i->accept( *this );

                const auto& location
                    = static_cast< const libstdhl::Log::LocationItem& >( *i );

                addDiagnostic( item.level(), location, msg );
            }
        }

        return result;
    }

    const std::vector< Diagnostic >& diagnostics( void ) const
    {
        return m_diagnostics;
    }

  private:
    void addDiagnostic( const libstdhl::Log::Level& level,
        const libstdhl::Log::LocationItem& location, const std::string& msg )
    {

        Position start( location.range().begin().line() - 1,
            location.range().begin().column() - 1 );
        Position end( location.range().end().line() - 1,
            location.range().end().column() );
        Range range( start, end );

        Diagnostic diagnostic( range, msg );

        switch( level.id() )
        {
            case libstdhl::Log::Level::ID::ERROR:
            {
                diagnostic.setSeverity( DiagnosticSeverity::Error );
                break;
            }
            case libstdhl::Log::Level::ID::WARNING:
            {
                diagnostic.setSeverity( DiagnosticSeverity::Warning );
                break;
            }
            case libstdhl::Log::Level::ID::INFORMATIONAL:
            {
                diagnostic.setSeverity( DiagnosticSeverity::Information );
                break;
            }
            case libstdhl::Log::Level::ID::NOTICE:
            {
                diagnostic.setSeverity( DiagnosticSeverity::Hint );
                break;
            }
            default:
            {
                break;
            }
        }

        const auto hash = std::hash< std::string >()( msg );
        if( m_cache.find( hash ) == m_cache.end() )
        {
            m_cache.insert( hash );
            m_diagnostics.emplace_back( diagnostic );
        }
    }

  private:
    std::string m_name;
    std::unordered_set< std::size_t > m_cache;
    std::vector< Diagnostic > m_diagnostics;
};

class LanguageServer final : public ServerInterface
{
  public:
    LanguageServer( Logger& log )
    : ServerInterface()
    , m_log( log )
    {
        m_log.info( "started LSP" );
    }

    InitializeResult initialize( const InitializeParams& params ) override
    {
        m_log.info( __FUNCTION__ );

        // TextDocumentSyncOptions tdso;
        // tdso.setChange( TextDocumentSyncKind::Full );
        // tdso.setOpenClose( true );

        ServerCapabilities sc;
        sc.setTextDocumentSync( TextDocumentSyncKind::Full );
        sc.setCodeActionProvider( true );

        InitializeResult res( sc );
        m_log.debug( res.dump() );
        return res;
    }

    void initialized( void ) noexcept override
    {
        m_log.info( __FUNCTION__ );
    }

    void shutdown( void ) override
    {
        m_log.info( __FUNCTION__ );
    }

    void exit( void ) noexcept override
    {
        m_log.info( __FUNCTION__ );
    }

    void textDocument_didOpen(
        const DidOpenTextDocumentParams& params ) noexcept override
    {
        m_log.info( __FUNCTION__ );

        const auto& fileuri = params.textDocument().uri();
        const auto& filestr = params.textDocument().text();
        const auto& fileext = params.textDocument().languageId();
        const auto& filerev = params.textDocument().version();

        auto result = m_files.emplace( fileuri.toString(),
            libstdhl::File::TextDocument{ fileuri, fileext } );
        result.first->second.setData( filestr );

        textDocument_analyze( result.first->second );
    }

    void textDocument_didChange(
        const DidChangeTextDocumentParams& params ) noexcept override
    {
        m_log.info( __FUNCTION__ );

        const auto& fileuri = params.textDocument().uri();
        const auto& filerev = params.textDocument().version();

        m_files.emplace( fileuri.toString(),
            libstdhl::File::TextDocument{ fileuri, "casm" } );
        auto result = m_files.find( fileuri.toString() );
        if( result == m_files.end() )
        {
            m_log.error(
                "unable to find text document '" + fileuri.toString() + "'" );
            return;
        }

        result->second.setData( params[ "contentChanges" ][ 0 ][ "text" ] );

        textDocument_analyze( result->second );
    }

    CodeActionResult textDocument_codeAction(
        const CodeActionParams& params ) override
    {
        m_log.info( __FUNCTION__ );

        CodeActionResult res;
        m_log.debug( res.dump() );
        return res;
    }

  private:
    void textDocument_analyze( const libstdhl::File::TextDocument& file )
    {
        libpass::PassResult pr;
        pr.setResult< libpass::LoadFilePass >(
            libstdhl::make< libpass::LoadFilePass::Data >( file ) );

        libpass::PassManager pm;
        pm.setDefaultResult( pr );
        pm.add< libcasm_fe::SourceToAstPass >();
        pm.add< libcasm_fe::AttributionPass >();
        pm.add< libcasm_fe::SymbolResolverPass >();
        pm.add< libcasm_fe::TypeInferencePass >();
        pm.add< libcasm_fe::ConsistencyCheckPass >();
        pm.setDefaultPass< libcasm_fe::ConsistencyCheckPass >();
        // pm.setDefaultPass< libcasm_fe::TypeInferencePass >();

        try
        {
            pm.run();
        }
        catch( const std::exception& e )
        {
            m_log.error( "pass manager triggered an exception: '"
                         + std::string( e.what() )
                         + "'" );
        }

        DiagnosticFormatter f( "casmd" );
        libstdhl::Log::OutputStreamSink c( std::cerr, f );
        pm.stream().flush( c );

        PublishDiagnosticsParams res( file.path(), f.diagnostics() );

        textDocument_publishDiagnostics( res );
    }

  private:
    Logger& m_log;
    std::unordered_map< std::string, libstdhl::File::TextDocument > m_files;
};

static constexpr const char* MODE = "mode";
static constexpr const char* MODE_LSP = "lsp";

static constexpr const char* CONN = "connection";
static constexpr const char* CONN_UDP4 = "udp4";
static constexpr const char* CONN_STDIO = "stdio";

int main( int argc, const char* argv[] )
{
    libpass::PassManager pm;
    libstdhl::Logger log( pm.stream() );
    log.setSource(
        libstdhl::make< libstdhl::Log::Source >( argv[ 0 ], DESCRIPTION ) );

    auto flush = [&argv, &pm]( void ) {
        libstdhl::Log::ApplicationFormatter f( argv[ 0 ] );
        libstdhl::Log::OutputStreamSink c( std::cerr, f );
        pm.stream().flush( c );
    };

    std::unordered_map< std::string, std::vector< std::string > > setting;

    libstdhl::Args options(
        argc, argv, libstdhl::Args::DEFAULT, [&]( const char* arg ) {

            if( strcmp( arg, MODE_LSP ) == 0 )
            {
                if( setting[ MODE ].size() > 0 )
                {
                    log.error( "already defined mode '"
                               + setting[ MODE ].front()
                               + "', unable to use additional provided mode '"
                               + std::string( arg )
                               + "'" );
                    return -1;
                }

                setting[ MODE ].emplace_back( arg );
            }
            else
            {
                log.error(
                    "invalid mode '" + std::string( arg )
                    + "' found, please refer to --help for more information" );
                return 1;
            }

            return 0;
        } );

    options.add( 't', "test-case-profile", libstdhl::Args::NONE,
        "display the unique test profile identifier", [&]( const char* ) {

            std::cout << libcasm_tc::Profile::get(
                             libcasm_tc::Profile::LANGUAGE_SERVER )
                      << "\n";

            return -1;
        } );

    options
        .add( 'h', "help", libstdhl::Args::NONE, "display usage and synopsis",
            [&]( const char* ) {

                log.output( "\n" + DESCRIPTION + "\n" + log.source()->name()
                        + ": usage: [options] mode\n"
                        + "\n"
                        + "mode: \n"
                        + "  lsp                            language server protocol\n"
                        + "\n"
                        + "options: \n"
                        + options.usage()
                        + "\n" );

                return -1;
            } );

    options.add( 'v', "version", libstdhl::Args::NONE,
        "display version information", [&]( const char* ) {

            log.output( "\n" + DESCRIPTION + "\n" + log.source()->name()
                        + ": version: "
                        + VERSION
                        + " [ "
                        + __DATE__
                        + " "
                        + __TIME__
                        + " ]\n"
                        + "\n"
                        + LICENSE );

            return -1;
        } );

    options.add( CONN_UDP4, libstdhl::Args::REQUIRED,
        "use a UDP IPv4 socket stream connection",
        [&]( const char* arg ) {
            setting[ CONN ].emplace_back( CONN_UDP4 );
            setting[ CONN_UDP4 ].emplace_back( arg );
            return 0;
        },
        "host:port" );

    options.add( CONN_STDIO, libstdhl::Args::NONE,
        "use a standard input/output stream", [&]( const char* arg ) {
            setting[ CONN ].emplace_back( CONN_STDIO );
            setting[ CONN_STDIO ].emplace_back( arg );
            return 0;
        } );

    // for( auto& p : libpass::PassRegistry::registeredPasses() )
    // {
    //     libpass::PassInfo& pi = *p.second;

    //     if( pi.argChar() or pi.argString() )
    //     {
    //         options.add( pi.argChar(), pi.argString(),
    //         libstdhl::Args::NONE,
    //             pi.description(), pi.argAction() );
    //     }
    // }

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
            LanguageServer server( log );

            switch( String::value( conn ) )
            {
                case String::value( CONN_UDP4 ):
                {
                    auto iface = libstdhl::Network::UDP::IPv4( kind );

                    iface.connect();

                    while( true )
                    {
                        // try
                        // {
                        std::string in;
                        const auto client = iface.receive( in );

                        if( in.size() == 0 )
                        {
                            continue;
                        }

                        log.debug( prefix + in + "\n" );
                        flush();

                        const auto request
                            = libstdhl::Network::LSP::Packet( in );
                        log.info(
                            prefix + "REQ: " + request.dump( true ) + "\n" );
                        flush();

                        request.process( server );

                        server.flush( [&]( const Message& response ) {
                            log.info( prefix + "ACK: " + response.dump( true )
                                      + "\n" );
                            flush();
                            const auto packet
                                = libstdhl::Network::LSP::Packet( response );
                            iface.send( packet, client );
                        } );
                        // }
                        // catch( const std::exception& e )
                        // {
                        //     log.error( e.what() );
                        //     flush();
                        //     usleep( 1000 );
                        // }
                    }

                    iface.disconnect();

                    break;
                }
                case String::value( CONN_STDIO ):
                {
                    while( true )
                    {
                        try
                        {
                            std::string in = "";

                            if( std::cin.eof() )
                            {
                                break;
                            }

                            while( not std::cin.eof() )
                            {
                                std::string tmp;
                                std::cin >> tmp;

                                in += tmp;

                                if( String::endsWith( tmp, "}\r\n" ) )
                                {
                                    break;
                                }
                            }

                            log.debug( prefix + in + "\n" );
                            flush();

                            const auto request
                                = libstdhl::Network::LSP::Packet( in );
                            log.info( prefix + "REQ: " + request.dump( true )
                                      + "\n" );
                            flush();

                            request.process( server );

                            server.flush( [&]( const Message& response ) {
                                log.info(
                                    prefix + "ACK: " + response.dump( true )
                                    + "\n" );
                                flush();
                                const auto packet
                                    = libstdhl::Network::LSP::Packet(
                                        response );
                                std::cout << packet.dump();
                            } );
                        }
                        catch( const std::exception& e )
                        {
                            log.error( e.what() );
                            flush();
                            usleep( 1000 );
                        }
                    }

                    break;
                }
                default:
                {
                    log.error( "unimplemented connection '" + conn
                               + "' selected, aborting" );
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
