#
#   Copyright (c) 2017 CASM Organization
#   All rights reserved.
#
#   Developed by: Philipp Paulweber
#                 Emmanuel Pescosta
#                 https://github.com/casm-lang/casmd
#
#   This file is part of casmd.
#
#   casmd is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   casmd is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with casmd. If not, see <http://www.gnu.org/licenses/>.
#

include_directories(
  ${LIBSTDHL_INCLUDE_DIR}
  ${LIBPASS_INCLUDE_DIR}
  ${LIBCASM_IR_INCLUDE_DIR}
  ${LIBCASM_RT_INCLUDE_DIR}
  ${LIBCASM_FE_INCLUDE_DIR}
  ${LIBCASM_TC_INCLUDE_DIR}
  )

add_library( ${PROJECT}-cpp OBJECT
  casmd.cpp
  )

configure_file(
  Version.in.h
  ${PROJECT}/Version
  )


# ecm_generate_headers( ${PROJECT}_HEADERS_CPP
#   ORIGINAL
#     CAMELCASE
#   HEADER_NAMES
#     HeaderNameHere!
#     Value
#   PREFIX
#     ${PROJECT}
#   REQUIRED_HEADERS
#     ${PROJECT}_HEADERS
#   )
# install(
#   FILES
#     ${${PROJECT}_HEADERS}
#     ${${PROJECT}_HEADERS_CPP}
#     ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}/Version
#   DESTINATION
#     "include/${PROJECT}"
#   )


# ecm_generate_headers( ${PROJECT}_SUBDIR_HEADERS_CPP
#   ORIGINAL
#     CAMELCASE
#   HEADER_NAMES
#     # FILE
#   PREFIX
#     ${PROJECT}/subdir
#   RELATIVE
#     subdir
#   REQUIRED_HEADERS
#     ${PROJECT}_SUBDIR_HEADERS
#   )
# install(
#   FILES
#     ${${PROJECT}_SUBDIR_HEADERS}
#     ${${PROJECT}_SUBDIR_HEADERS_CPP}
#   DESTINATION
#     "include/${PROJECT}/subdir"
#   )
