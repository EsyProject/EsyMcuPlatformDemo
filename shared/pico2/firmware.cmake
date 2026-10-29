# SPDX-FileCopyrightText: 2026 EsyProject contributors
#
# SPDX-License-Identifier: Apache-2.0

# Retain the selected SDK's RP2350 startup, embedded image block and RAM section
# layout. Generate a linker file only in the build directory; the SDK checkout
# and this repository contain no copied or modified vendor linker source.
function(demo_pico_add_firmware target layout)
    if(NOT layout MATCHES "^(bootloader|slot_a|slot_b)$")
        message(FATAL_ERROR "Unknown Pico 2 image layout: ${layout}")
    endif()
    set(demo_sdk_linker "${PICO_SDK_PATH}/src/rp2_common/pico_crt0/rp2350/memmap_default.ld")
    if(NOT EXISTS "${demo_sdk_linker}")
        message(FATAL_ERROR "Pico 2 examples require the SDK RP2350 default linker layout.")
    endif()
    file(READ "${demo_sdk_linker}" demo_linker_source)
    string(REGEX MATCHALL "INCLUDE[ \t]+\"pico_flash_region\\.ld\""
        demo_flash_includes "${demo_linker_source}")
    list(LENGTH demo_flash_includes demo_flash_include_count)
    if(NOT demo_flash_include_count EQUAL 1)
        message(FATAL_ERROR "The Pico SDK linker FLASH declaration changed; review the example layout.")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/linker")
    foreach(demo_layout IN ITEMS bootloader slot_a slot_b)
        if(demo_layout STREQUAL "bootloader")
            set(demo_flash_origin 0x10000000)
            set(demo_flash_length 128K)
            set(demo_slot_name DEFAULT)
        elseif(demo_layout STREQUAL "slot_a")
            set(demo_flash_origin 0x10040000)
            set(demo_flash_length 384K)
            set(demo_slot_name A)
        else()
            set(demo_flash_origin 0x100a0000)
            set(demo_flash_length 384K)
            set(demo_slot_name B)
        endif()
        string(REGEX REPLACE "INCLUDE[ \t]+\"pico_flash_region\\.ld\""
            "FLASH(rx) : ORIGIN = ${demo_flash_origin}, LENGTH = ${demo_flash_length}"
            demo_linker "${demo_linker_source}")
        set(demo_linker_path "${CMAKE_BINARY_DIR}/generated/linker/pico2_${demo_layout}.ld")
        file(WRITE "${demo_linker_path}" "${demo_linker}")
        set(ESY_LINKER_SCRIPT_${demo_slot_name} "${demo_linker_path}" PARENT_SCOPE)
    endforeach()
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${demo_sdk_linker}")
endfunction()
