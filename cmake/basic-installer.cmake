# A basic installer setup.
#
# This cmake file introduces two targets
#  shortcircuit-products:      moves all the built assets to a well named directory
#  shortcircuit-installer:   depends on shortcircuit-products, builds an installer
#
# Right now shortcircuit-installer builds just the crudest zip file but this is the target
# on which we will hang the proper installers later

set(SCXT_PRODUCT_DIR ${CMAKE_BINARY_DIR}/shortcircuit-products)
file(MAKE_DIRECTORY ${SCXT_PRODUCT_DIR})

add_custom_target(shortcircuit-products)
add_custom_target(shortcircuit-installer)

function(shortcircuit_package format)
    get_target_property(output_dir ShortcircuitXT RUNTIME_OUTPUT_DIRECTORY)

    if (TARGET ShortcircuitXT_${format})
        add_dependencies(shortcircuit-products ShortcircuitXT_${format})
        add_custom_command(
                TARGET shortcircuit-products
                POST_BUILD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND echo "Installing ${output_dir}/${format} to ${SCXT_PRODUCT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${output_dir}/${format}/ ${SCXT_PRODUCT_DIR}/
        )
    endif ()
endfunction()

shortcircuit_package(VST3)
shortcircuit_package(VST)
shortcircuit_package(LV2)
shortcircuit_package(AU)
shortcircuit_package(CLAP)
shortcircuit_package(Standalone)

add_dependencies(shortcircuit-installer shortcircuit-products)

add_custom_command(
        TARGET shortcircuit-installer
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND echo "Installing LICENSE and so forth to ${SCXT_PRODUCT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/LICENSE ${SCXT_PRODUCT_DIR}/
        # COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Installer/ZipReadme.txt ${SCXT_PRODUCT_DIR}/Readme.txt
)

find_package(Git)

if (Git_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE VERSION_CHUNK
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    set(VERSION_CHUNK "unknownhash")
endif ()

string(TIMESTAMP SCXT_DATE "%Y-%m-%d")
if (WIN32)
    math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
    set(SCXT_ZIP ShortcircuitXT-${SCXT_DATE}-${VERSION_CHUNK}-${CMAKE_SYSTEM_NAME}-${BITS}bit.zip)
else ()
    set(SCXT_ZIP ShortcircuitXT-${SCXT_DATE}-${VERSION_CHUNK}-${CMAKE_SYSTEM_NAME}.zip)
endif ()


if (APPLE)
    message(STATUS "Configuring for mac installer.")
    add_custom_command(
            TARGET shortcircuit-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND ${CMAKE_SOURCE_DIR}/libs/sst/sst-plugininfra/scripts/installer_mac/make_installer.sh "Shortcircuit XT" ${CMAKE_BINARY_DIR}/shortcircuit-products ${CMAKE_SOURCE_DIR}/resources/installer_mac ${CMAKE_BINARY_DIR}/installer "${SCXT_DATE}-${VERSION_CHUNK}"
    )
elseif (WIN32)
    message(STATUS "Basic Installer: Target is installer/${SCXT_ZIP}")
    add_custom_command(
            TARGET shortcircuit-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND 7z a -r installer/${SCXT_ZIP} ${SURGE_PRODUCT_DIR}
            COMMAND ${CMAKE_COMMAND} -E echo "Installer in: installer/${SCXT_ZIP}")
else ()
    message(STATUS "Basic Installer: Target is installer/${SCXT_ZIP}")
    add_custom_command(
            TARGET shortcircuit-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND ${CMAKE_COMMAND} -E tar cvf installer/${SCXT_ZIP} --format=zip ${SCXT_PRODUCT_DIR}/
            COMMAND ${CMAKE_COMMAND} -E echo "Installer in: installer/${SCXT_ZIP}")
endif ()