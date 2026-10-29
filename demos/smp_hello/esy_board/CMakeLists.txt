# SPDX-FileCopyrightText: 2026 EsyProject contributors
# SPDX-License-Identifier: Apache-2.0

add_library(esy_board STATIC src/esy_board.c)
target_include_directories(esy_board PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(esy_board PUBLIC esy_common_defs PRIVATE esy_opt_default)
