//
//  Copyright (C) 2017-2021 CASM Organization <https://casm-lang.org>
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

#ifndef _CASMD_DIAGNOSTIC_FORMATTER_H_
#define _CASMD_DIAGNOSTIC_FORMATTER_H_

/**
   @brief    TODO

   TODO
*/

#include <libstdhl/Log>
#include <libstdhl/net/lsp/LSP>

namespace casmd
{
    class DiagnosticFormatter : public libstdhl::Log::StringFormatter
    {
      public:
        DiagnosticFormatter( const std::string& name );

        std::string visit( libstdhl::Log::Data& item ) override;

        const std::vector< libstdhl::Network::LSP::Diagnostic >& diagnostics( void ) const;

      private:
        void addDiagnostic(
            const libstdhl::Log::Level& level,
            const libstdhl::Log::LocationItem& location,
            const std::string& msg );

      private:
        std::string m_name;
        std::vector< libstdhl::Network::LSP::Diagnostic > m_diagnostics;
    };
}

#endif  // _CASMD_DIAGNOSTIC_FORMATTER_H_

//
//  Local variables:
//  mode: c++
//  indent-tabs-mode: nil
//  c-basic-offset: 4
//  tab-width: 4
//  End:
//  vim:noexpandtab:sw=4:ts=4:
//
