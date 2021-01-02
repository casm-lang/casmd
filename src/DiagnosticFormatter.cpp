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

#include "DiagnosticFormatter.h"

using namespace casmd;
using namespace libstdhl;

DiagnosticFormatter::DiagnosticFormatter( const std::string& name )
: m_name( name )
{
}

std::string DiagnosticFormatter::visit( libstdhl::Log::Data& item )
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

            const auto& location = static_cast< const libstdhl::Log::LocationItem& >( *i );

            addDiagnostic( item.level(), location, msg );
        }
    }

    return result;
}

const std::vector< Network::LSP::Diagnostic >& DiagnosticFormatter::diagnostics( void ) const
{
    return m_diagnostics;
}

void DiagnosticFormatter::addDiagnostic(
    const libstdhl::Log::Level& level,
    const libstdhl::Log::LocationItem& location,
    const std::string& msg )
{
    Network::LSP::Position start(
        location.range().begin().line() - 1, location.range().begin().column() - 1 );
    Network::LSP::Position end(
        location.range().end().line() - 1, location.range().end().column() - 1 );
    Network::LSP::Range range( start, end );
    Network::LSP::Diagnostic diagnostic( range, msg );

    switch( level.id() )
    {
        case libstdhl::Log::Level::ID::ERROR:
        {
            diagnostic.setSeverity( Network::LSP::DiagnosticSeverity::Error );
            break;
        }
        case libstdhl::Log::Level::ID::WARNING:
        {
            diagnostic.setSeverity( Network::LSP::DiagnosticSeverity::Warning );
            break;
        }
        case libstdhl::Log::Level::ID::INFORMATIONAL:
        {
            diagnostic.setSeverity( Network::LSP::DiagnosticSeverity::Information );
            break;
        }
        case libstdhl::Log::Level::ID::NOTICE:
        {
            diagnostic.setSeverity( Network::LSP::DiagnosticSeverity::Hint );
            break;
        }
        default:
        {
            break;
        }
    }

    m_diagnostics.emplace_back( diagnostic );
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
