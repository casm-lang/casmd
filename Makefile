#
#   Copyright (C) 2017-2019 CASM Organization <https://casm-lang.org>
#   All rights reserved.
#
#   Developed by: Philipp Paulweber
#                 Emmanuel Pescosta
#                 <https://github.com/casm-lang/casmd>
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

TARGET = casmd

FORMAT  = src
FORMAT += src/*
FORMAT += etc
FORMAT += etc/*
FORMAT += etc/*/*

UPDATE_ROOT = ../../lib/stdhl

include .cmake/config.mk

ENV_FLAGS = CASM_ARG_PRE="lsp --stdio"
ifeq ($(ENV_OSYS),Windows)
  ENV_FLAGS += CASM=$(OBJ)\\$(TARGET)
else
  ENV_FLAGS += CASM=$(OBJ)/$(TARGET)
endif


ci-fetch: ci-git-access

ci-git-access:
	@echo "-- Git Access Configuration"
	@git config --global \
	url."https://$(GITHUB_TOKEN)@github.com/".insteadOf "https://github.com/"
