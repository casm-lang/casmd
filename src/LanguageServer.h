//
//  Copyright (C) 2017-2023 CASM Organization <https://casm-lang.org>
//  All rights reserved.
//
//  Developed by: Philipp Paulweber et al.
//  <https://github.com/casm-lang/casmd/graphs/contributors>
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

#ifndef _CASMD_LANGUAGE_SERVER_H_
#define _CASMD_LANGUAGE_SERVER_H_

/**
   @brief    TODO

   TODO
*/

#include <libstdhl/Log>
#include <libstdhl/Type>
#include <libstdhl/data/file/TextDocument>
#include <libstdhl/net/lsp/LSP>

#include <unordered_map>

namespace casmd
{
    using u1 = libstdhl::u1;

    class LanguageServer final : public libstdhl::Network::LSP::Server
    {
      public:
        LanguageServer( libstdhl::Logger& log );

        libstdhl::Network::LSP::InitializeResult initialize(
            const libstdhl::Network::LSP::InitializeParams& params ) override;

        //
        //
        // Lifetime
        //

        void initialized( void ) noexcept override;

        void shutdown( void ) override;

        void exit( void ) noexcept override;

        void client_cancel( const libstdhl::Network::LSP::CancelParams& params ) noexcept override;

        //
        //
        // Workspace
        //

        libstdhl::Network::LSP::ExecuteCommandResult workspace_executeCommand(
            const libstdhl::Network::LSP::ExecuteCommandParams& params ) override;

        //
        //
        // Text Synchronization
        //

        void textDocument_didOpen(
            const libstdhl::Network::LSP::DidOpenTextDocumentParams& params ) noexcept override;

        void textDocument_didChange(
            const libstdhl::Network::LSP::DidChangeTextDocumentParams& params ) noexcept override;

        //
        //
        // Language Features
        //

        libstdhl::Network::LSP::HoverResult textDocument_hover(
            const libstdhl::Network::LSP::HoverParams& params ) override;

        libstdhl::Network::LSP::CodeActionResult textDocument_codeAction(
            const libstdhl::Network::LSP::CodeActionParams& params ) override;

        libstdhl::Network::LSP::CodeLensResult textDocument_codeLens(
            const libstdhl::Network::LSP::CodeLensParams& params ) override;

      private:
        void textDocument_analyze( const libstdhl::Network::LSP::DocumentUri& fileuri );

        std::string textDocument_execute(
            const libstdhl::Network::LSP::DocumentUri& fileuri, const u1 symbolic = false );

      private:
        libstdhl::Logger& m_log;
        std::unordered_map< std::string, libstdhl::File::TextDocument > m_files;
    };
}

#endif  // _CASMD_LANGUAGE_SERVER_H_

//
//  Local variables:
//  mode: c++
//  indent-tabs-mode: nil
//  c-basic-offset: 4
//  tab-width: 4
//  End:
//  vim:noexpandtab:sw=4:ts=4:
//
