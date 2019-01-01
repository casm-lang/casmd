//
//  Copyright (C) 2017-2019 CASM Organization <https://casm-lang.org>
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

#include "main.h"

#include <casmd/Version>

void casmd_main_dummy( void )
{
    const auto source = libstdhl::Memory::make< libstdhl::Log::Source >( "casmd", "CASMD" );
    libstdhl::Log::defaultSource( source );
}

TEST( casmd_main, empty )
{
    std::cout << casmd::REVTAG << "\n";
    std::cout << casmd::COMMIT << "\n";
    std::cout << casmd::BRANCH << "\n";
    std::cout << casmd::LICENSE << "\n";
    std::cout << casmd::NOTICE << "\n";
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
