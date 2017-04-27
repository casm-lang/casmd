//
//  Copyright (c) 2014-2017 CASM Organization
//  All rights reserved.
//
//  Developed by: Philipp Paulweber
//                Emmanuel Pescosta
//                Florian Hahn
//                https://github.com/casm-lang/casms
//
//  This file is part of casms.
//
//  casms is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  casms is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with casms. If not, see <http://www.gnu.org/licenses/>.
//

#include "license.h"
#include "version.h"

#include "libpass.h"
#include "libstdhl.h"

#include "libcasm-fe.h"
#include "libcasm-ir.h"
#include "libcasm-tc.h"

/**
    @brief TODO

    TODO
*/

static const std::string DESCRIPTION
    = "Corinthian Abstract State Machine (CASM) Language Server\n";

int main( int argc, const char* argv[] )
{
    libpass::PassManager pm;
    libstdhl::Logger log( pm.stream() );
    log.setSource(
        libstdhl::make< libstdhl::Log::Source >( argv[ 0 ], DESCRIPTION ) );

    auto flush = [&pm, &argv]() {
        libstdhl::Log::ApplicationFormatter f( argv[ 0 ] );
        libstdhl::Log::OutputStreamSink c( std::cerr, f );
        pm.stream().flush( c );
    };

    std::vector< std::string > files_input;

    libstdhl::Args options( argc, argv, libstdhl::Args::DEFAULT,
        [&log, &files_input]( const char* arg ) {

            if( files_input.size() > 0 )
            {
                log.error( "too many files, input file '" + files_input.front()
                           + "' cannot be combined with file '"
                           + arg
                           + "'" );
                return 1;
            }

            files_input.emplace_back( arg );
            return 0;
        } );

    options.add( 't', "test-case-profile", libstdhl::Args::NONE,
        "display the unique test profile identifier",
        [&options]( const char* ) {

            std::cout << libcasm_tc::Profile::get(
                             libcasm_tc::Profile::LANGUAGE_SERVER )
                      << "\n";

            return -1;
        } );

    options.add( 'h', "help", libstdhl::Args::NONE,
        "display usage and synopsis", [&log, &options]( const char* ) {

            log.output( "\n" + DESCRIPTION + "\n" + log.source()->name()
                        + ": usage: [options] <file>\n"
                        + "\n"
                        + "options: \n"
                        + options.usage()
                        + "\n" );

            return -1;
        } );

    options.add( 'v', "version", libstdhl::Args::NONE,
        "display version information", [&log]( const char* ) {

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

    for( auto& p : libpass::PassRegistry::registeredPasses() )
    {
        libpass::PassInfo& pi = *p.second;

        if( pi.argChar() or pi.argString() )
        {
            options.add( pi.argChar(), pi.argString(), libstdhl::Args::NONE,
                pi.description(), pi.argAction() );
        }
    }

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

    if( files_input.size() == 0 )
    {
        log.error( "no input file provided" );
        flush();
        return 2;
    }

    // register all wanted passes
    // and configure their setup hooks if desired

    pm.add< libpass::LoadFilePass >(
        [&files_input]( libpass::LoadFilePass& pass ) {
            pass.setFilename( files_input.front() );

        } );

    pm.add< libcasm_fe::SourceToAstPass >();
    pm.add< libcasm_fe::TypeInferencePass >();

    int result = 0;

    try
    {
        pm.run( flush );
    }
    catch( std::exception& e )
    {
        log.error( "pass manager triggered an exception: '"
                   + std::string( e.what() )
                   + "'" );
        result = -1;
    }

    flush();

    return result;
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
