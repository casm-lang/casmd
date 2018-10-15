//
//  Copyright (C) 2017-2018 CASM Organization <https://casm-lang.org>
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

#include "DiagnosticFormatter.h"
#include "casmd/Version"

#include <libcasm-fe/analyze/ConsistencyCheckPass>
#include <libcasm-fe/execute/NumericExecutionPass>
#include <libpass/PassManager>
#include <libpass/PassResult>
#include <libpass/analyze/LoadFilePass>
#include <libstdhl/String>

using namespace casmd;
using namespace libpass;
using namespace libstdhl;
using namespace Network;
using namespace LSP;

LanguageServer::LanguageServer( Logger& log )
: ServerInterface()
, m_log( log )
, m_files()
{
    m_log.info( "started LSP" );
}

InitializeResult LanguageServer::initialize( const InitializeParams& params )
{
    m_log.info( __FUNCTION__ );

    // TextDocumentSyncOptions tdso;
    // tdso.setChange( TextDocumentSyncKind::Full );
    // tdso.setOpenClose( true );

    ServerCapabilities sc;
    sc.setTextDocumentSync( TextDocumentSyncKind::Full );
    sc.setCodeActionProvider( true );

    ExecuteCommandOptions eco;
    eco.addCommand( "version" );
    eco.addCommand( "run" );
    sc.setExecuteCommandProvider( eco );

    CodeLensOptions clo;
    sc.setCodeLensProvider( clo );
    sc.setHoverProvider( true );

    InitializeResult res( sc );
    m_log.debug( res.dump() );
    return res;
}

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
    }

    return ExecuteCommandResult();
}

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

    // textDocument_analyze( result.first->second );
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

    // textDocument_analyze( result->second );
}

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

    const auto& fileuri = params.textDocument().uri();
    textDocument_analyze( fileuri );

    return res;
}

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

std::string LanguageServer::textDocument_execute( const DocumentUri& fileuri )
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
    pm.setDefaultPass< libcasm_fe::NumericExecutionPass >();

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