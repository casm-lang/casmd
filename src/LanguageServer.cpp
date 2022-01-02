//
//  Copyright (C) 2017-2022 CASM Organization <https://casm-lang.org>
//  All rights reserved.
//
//  Developed by: Philipp Paulweber et al.
//                <https://github.com/casm-lang/casmd/graphs/contributors>
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

#include "DiagnosticFormatter.h"
#include "casmd/Version"

#include <libcasm-fe/analyze/ConsistencyCheckPass>
#include <libcasm-fe/execute/NumericExecutionPass>
#include <libcasm-fe/execute/SymbolicExecutionPass>
#include <libpass/PassManager>
#include <libpass/PassResult>
#include <libpass/analyze/LoadFilePass>
#include <libstdhl/String>

#include <chrono>
#include <iostream>
#include <thread>

using namespace casmd;
using namespace libpass;
using namespace libstdhl;
using namespace Network;
using namespace LSP;

//
//
// LanguageServer
//

LanguageServer::LanguageServer( Logger& log )
: Server()
, m_log( log )
, m_files()
{
    m_log.info( "started LSP" );
}

InitializeResult LanguageServer::initialize( const InitializeParams& params )
{
    m_log.info( __FUNCTION__ );

    ServerCapabilities sc;

    // TextDocumentSyncOptions tdso;
    // tdso.setChange( TextDocumentSyncKind::Full );
    // tdso.setOpenClose( true );
    // sc.setTextDocumentSync( tdso );
    sc.setTextDocumentSync( TextDocumentSyncKind::Full );

    CompletionOptions cp;
    // cp.setResolveProvider( false );
    // cp.addTriggerCharacters( "." );
    sc.setCompletionProvider( cp );

    sc.setHoverProvider( true );
    // sc.setDocumentSymbolProvider( true );
    // sc.setReferencesProvider( true );
    // sc.setDefinitionProvider( true );
    // sc.setDocumentHighlightProvider( true );
    // sc.setCodeActionProvider( true );
    // sc.setRenameProvider( true );
    // sc.setColorProvider( .. );
    // sc.setFoldingRangeProvider( ... );

    ExecuteCommandOptions eco;
    eco.addCommand( "version" );
    eco.addCommand( "run" );
    eco.addCommand( "trace" );
    sc.setExecuteCommandProvider( eco );

    // CodeLensOptions clo;
    // sc.setCodeLensProvider( clo );

    // TODO: FIXME: @ppaulweber: REMOVE this delay if LSP clients
    // (e.g. monaco, vscode etc.) can handle a really fast initialization
    using namespace std::chrono_literals;
    std::this_thread::sleep_for( 1s );

    return InitializeResult( sc );
}

//
//
// LanguageServer Lifetime
//

void LanguageServer::initialized( void ) noexcept
{
    m_log.info( __FUNCTION__ );
}

void LanguageServer::shutdown( void )
{
    m_log.info( __FUNCTION__ );
}

void LanguageServer::exit( void ) noexcept
{
    m_log.info( __FUNCTION__ );
}

void LanguageServer::client_cancel( const CancelParams& params ) noexcept
{
    m_log.info( __FUNCTION__ );
}

//
//
// LanguageServer Workspace
//

ExecuteCommandResult LanguageServer::workspace_executeCommand( const ExecuteCommandParams& params )
{
    m_log.info( __FUNCTION__ );

    const auto& command = params.command();
    const auto cmd = String::value( command );
    switch( cmd )
    {
        case String::value( "version" ):
        {
            return "\n" + std::string( DESCRIPTION ) + "\n" + m_log.source()->name() +
                   ": version: " + REVTAG + " [ " + __DATE__ + " " + __TIME__ + " ]\n" + "\n" +
                   NOTICE;
        }
        case String::value( "run" ):
        {
            const DocumentUri fileuri = DocumentUri::fromString( "inmemory://model.casm" );
            return textDocument_execute( fileuri );
        }
        case String::value( "trace" ):
        {
            const DocumentUri fileuri = DocumentUri::fromString( "inmemory://model.casm" );
            return textDocument_execute( fileuri, true );
        }
    }

    return ExecuteCommandResult();
}

//
//
// LanguageServer Text Synchronization
//

void LanguageServer::textDocument_didOpen( const DidOpenTextDocumentParams& params ) noexcept
{
    m_log.info( __FUNCTION__ );

    const auto& fileuri = params.textDocument().uri();
    const auto& filestr = params.textDocument().text();
    const auto& fileext = params.textDocument().languageId();
    const auto& filerev = params.textDocument().version();

    auto result = m_files.emplace( fileuri.toString(), File::TextDocument{ fileuri, fileext } );

    if( not result.second )
    {
        m_log.error( "text document '" + fileuri.toString() + "' already opened!" );
        return;
    }

    result.first->second.setData( filestr );

    textDocument_analyze( fileuri );
}

void LanguageServer::textDocument_didChange( const DidChangeTextDocumentParams& params ) noexcept
{
    m_log.info( __FUNCTION__ );

    const auto& fileuri = params.textDocument().uri();
    const auto& filerev = params.textDocument().version();

    auto result = m_files.find( fileuri.toString() );
    if( result == m_files.end() )
    {
        m_log.error( "unable to find text document '" + fileuri.toString() + "'" );
        return;
    }

    result->second.setData( params[ "contentChanges" ][ 0 ][ "text" ] );

    textDocument_analyze( fileuri );
}

//
//
// LanguageServer Language Features
//

HoverResult LanguageServer::textDocument_hover( const HoverParams& params )
{
    m_log.info( __FUNCTION__ );

    const auto& fileuri = params.textDocument().uri();
    const auto& filepos = params.position();

    // MarkedString( "hi there!" )
    HoverResult res;
    return res;
}

CodeActionResult LanguageServer::textDocument_codeAction( const CodeActionParams& params )
{
    m_log.info( __FUNCTION__ );
    CodeActionResult res;

    return res;
}

CodeLensResult LanguageServer::textDocument_codeLens( const CodeLensParams& params )
{
    m_log.info( __FUNCTION__ );
    CodeLensResult res;
    // res.addCodeLens( Range( Position( 1, 1 ), Position( 1, 1 ) ) );

    // const auto& fileuri = params.textDocument().uri();
    // textDocument_analyze( fileuri );

    return res;
}

//
//
// LanguageServer (private)
//

void LanguageServer::textDocument_analyze( const DocumentUri& fileuri )
{
    auto result = m_files.find( fileuri.toString() );
    if( result == m_files.end() )
    {
        m_log.error( "unable to find text document '" + fileuri.toString() + "'" );
        return;
    }

    auto& file = result->second;

    m_log.info(
        std::to_string( (u64)&file ) + " ... " + fileuri.toString() + "\n\n" + file.data() );

    // file is already in-memory, by-pass the LoadFilePass by setting its pass result
    PassResult pr;
    pr.setOutput< LoadFilePass >( file );

    PassManager pm;
    pm.setDefaultResult( pr );
    pm.setDefaultPass< libcasm_fe::ConsistencyCheckPass >();

    try
    {
        pm.run();
    }
    catch( const std::exception& e )
    {
        m_log.error( "pass manager triggered an exception: '" + std::string( e.what() ) + "'" );
    }

    DiagnosticFormatter formatter( "casmd" );
    Log::OutputStreamSink sink( std::cerr, formatter );
    pm.stream().flush( sink );

    PublishDiagnosticsParams res( file.path(), formatter.diagnostics() );

    textDocument_publishDiagnostics( res );
}

std::string LanguageServer::textDocument_execute( const DocumentUri& fileuri, const u1 symbolic )
{
    auto result = m_files.find( fileuri.toString() );
    if( result == m_files.end() )
    {
        const auto msg = "unable to find text document '" + fileuri.toString() + "'";
        m_log.error( msg );
        throw std::invalid_argument( msg );
    }

    auto& file = result->second;

    m_log.info(
        std::to_string( (u64)&file ) + " ... " + fileuri.toString() + "\n\n" + file.data() );

    PassResult pr;
    pr.setOutput< LoadFilePass >( file );

    PassManager pm;
    pm.setDefaultResult( pr );

    if( not symbolic )
    {
        pm.setDefaultPass< libcasm_fe::NumericExecutionPass >();
    }
    else
    {
        pm.setDefaultPass< libcasm_fe::SymbolicExecutionPass >();
    }

    std::ostringstream local;
    auto cout_buff = std::cout.rdbuf();
    std::cout.rdbuf( local.rdbuf() );

    try
    {
        pm.run();
    }
    catch( const std::exception& e )
    {
        m_log.error( "pass manager triggered an exception: '" + std::string( e.what() ) + "'" );
    }

    std::cout.rdbuf( cout_buff );

    DiagnosticFormatter formatter( "casmd" );
    Log::OutputStreamSink sink( std::cerr, formatter );
    pm.stream().flush( sink );

    PublishDiagnosticsParams res( file.path(), formatter.diagnostics() );

    textDocument_publishDiagnostics( res );

    return local.str();
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
