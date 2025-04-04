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

add_custom_target(shortcircuit-products ALL)
add_custom_target(shortcircuit-installer)

function(shortcircuit_package format suffix)
    if (TARGET scxt_clapfirst_all)
        get_target_property(output_dir scxt_clapfirst_all ARTEFACT_DIRECTORY)

        if (TARGET scxt_clapfirst_${format})
            get_target_property(output_name scxt_clapfirst_${format} LIBRARY_OUTPUT_NAME)
            if( ${output_name} STREQUAL "output_name-NOTFOUND")
                get_target_property(output_name scxt_clapfirst_${format} OUTPUT_NAME)
            endif()

            add_dependencies(shortcircuit-products scxt_clapfirst_${format})

            set(isdir 0)
            if (APPLE)
                set(isdir 1)
                set(target_to_root "../../..")
            else()
                if ("${suffix}" STREQUAL ".vst3")
                    set(isdir 1)
                    set(target_to_root "../../..")
                endif()
            endif()

            message(STATUS "Adding scxt_clapfirst_${format} to installer from '${output_name}${suffix}'")

            if (${isdir} EQUAL 1)
                add_custom_command(
                        TARGET shortcircuit-products
                        POST_BUILD
                        USES_TERMINAL
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        COMMAND echo "Installing directory '${output_dir}/${output_name}${suffix}' to ${SCXT_PRODUCT_DIR}"
                        COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:scxt_clapfirst_${format}>/${target_to_root}/${output_name}${suffix}" "${SCXT_PRODUCT_DIR}/${output_name}${suffix}"
                )
            else()
                add_custom_command(
                        TARGET shortcircuit-products
                        POST_BUILD
                        USES_TERMINAL
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        COMMAND echo "Installing file '${output_dir}/${output_name}${suffix}' to ${SCXT_PRODUCT_DIR}"
                        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:scxt_clapfirst_${format}>" "${SCXT_PRODUCT_DIR}/"

                )
            endif()
        endif()
    endif()
endfunction()

shortcircuit_package(vst3 ".vst3")
shortcircuit_package(auv2 ".component")
shortcircuit_package(clap ".clap")
if (APPLE)
    shortcircuit_package(standalone ".app")
elseif (UNIX)
    shortcircuit_package(standalone "")
else()
    shortcircuit_package(standalone ".exe")
endif()

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
    if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64ec")
        set(SCXT_ZIP ShortcircuitXT-${SCXT_DATE}-${VERSION_CHUNK}-${CMAKE_SYSTEM_NAME}-${CMAKE_GENERATOR_PLATFORM}.zip)
    else()
        set(SCXT_ZIP ShortcircuitXT-${SCXT_DATE}-${VERSION_CHUNK}-${CMAKE_SYSTEM_NAME}-${BITS}bit.zip)
    endif()
else ()
    set(SCXT_ZIP ShortcircuitXT-${SCXT_EXTRA_INSTALLER_NAME}${SCXT_DATE}-${VERSION_CHUNK}-${CMAKE_SYSTEM_NAME}.zip)
endif ()
message(STATUS "Installer ZIP is ${SCXT_ZIP}")

if (APPLE)
    message(STATUS "Configuring for Mac installer.")
    add_custom_command(
            TARGET shortcircuit-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND ${CMAKE_SOURCE_DIR}/libs/sst/sst-plugininfra/scripts/installer_mac/make_installer.sh "Shortcircuit XT" ${CMAKE_BINARY_DIR}/shortcircuit-products ${CMAKE_SOURCE_DIR}/resources/installer_mac ${CMAKE_BINARY_DIR}/installer "${SCXT_DATE}-${VERSION_CHUNK}"
    )
elseif (WIN32)
    message(STATUS "Configuring for Windows installer")
    add_custom_command(
            TARGET shortcircuit-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND 7z a -r installer/${SCXT_ZIP} ${SCXT_PRODUCT_DIR}/
            COMMAND ${CMAKE_COMMAND} -E echo "ZIP Installer in: installer/${SCXT_ZIP}")
    find_program(SHORTCIRCUIT_NUGET_EXE nuget.exe PATHS ENV "PATH")
    if(SHORTCIRCUIT_NUGET_EXE)
        if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64ec")
            message(STATUS "Zip only for now on arm64ec")
        else()
            message(STATUS "NuGet found at ${SHORTCIRCUIT_NUGET_EXE}")
            add_custom_command(
                TARGET shortcircuit-installer
                POST_BUILD
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMAND ${SHORTCIRCUIT_NUGET_EXE} install Tools.InnoSetup -version 6.2.1
                COMMAND Tools.InnoSetup.6.2.1/tools/iscc.exe /O"installer" /DSCXT_SRC="${CMAKE_SOURCE_DIR}" /DSCXT_BIN="${CMAKE_BINARY_DIR}" /DMyAppVersion="${SCXT_DATE}-${VERSION_CHUNK}" "${CMAKE_SOURCE_DIR}/resources/installer_win/scxt${BITS}.iss")
            endif()
    else()
        message(STATUS "NuGet not found!")
    endif()
else ()
    message(STATUS "Basic installer: Target is installer/${SCXT_ZIP}")
    add_custom_command(
            TARGET shortcircuit-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND ${CMAKE_COMMAND} -E tar cvf installer/${SCXT_ZIP} --format=zip ${SCXT_PRODUCT_DIR}/
            COMMAND ${CMAKE_COMMAND} -E echo "Installer in: installer/${SCXT_ZIP}")
endif ()
