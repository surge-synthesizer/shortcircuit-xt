project(scxt_clapfirst)

message(STATUS "Shortcircuit XT building clap-first wrappers")
set(PRODUCT_NAME "Shortcircuit XT")
set(IMPL_TARGET scxt-plugin)

make_clapfirst_plugins(
        TARGET_NAME ${PROJECT_NAME}
        IMPL_TARGET ${IMPL_TARGET}

        OUTPUT_NAME "${PRODUCT_NAME}"

        ENTRY_SOURCE scxt-clap-entry.cpp

        BUNDLE_IDENTIFER "org.surge-synth-team.shortcircuit-xt"
        BUNDLE_VERSION ${PROJECT_VERSION}

        COPY_AFTER_BUILD ${SCXT_COPY_PLUGIN_AFTER_BUILD}

        PLUGIN_FORMATS CLAP VST3 AUV2

        WINDOWS_FOLDER_VST3 TRUE

        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets

        # You can add a target-per-standalone you want. Syntax here is
        #   target-postfix output-name clap-id
        # This allows you to make multiple standalones from a multi-plugin clap
        STANDALONE_CONFIGURATIONS
        standalone "${PRODUCT_NAME}" "org.surge-synth-team.shortcircuit-xt"

        # STANDALONE_MACOS_ICON "${CMAKE_SOURCE_DIR}/resources/mac_installer/Icon.icns"
        STANDALONE_WINDOWS_ICON "${CMAKE_SOURCE_DIR}/resources/images/scxt.ico"
        STANDALONE_MACOS_ICON "${CMAKE_SOURCE_DIR}/resources/images/SCAppIcon.icns"
)

# Our installer uses this
set_target_properties(${PROJECT_NAME}_all PROPERTIES ARTEFACT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets)

target_compile_definitions(clap-wrapper-compile-options-public INTERFACE CLAP_WRAPPER_LOGLEVEL=0)

if (UNIX AND NOT APPLE)
    # For now I'm adding a set of cmake install rules only for linux
    # The directories on windows and macos aren't run by the INSTALL dir
    # and special casing them here vs using existing COPY_FROM_PLUGIN
    # and installer/sign workflow seems silly.
    #
    # If you disagree fix it up and send in a PR!
    include(GNUInstallDirs)

    # Typical Linux/Unix locations under the installation prefix
    set(SCXT_CLAP_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/clap")
    set(SCXT_VST3_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/vst3")
    set(SCXT_STANDALONE_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")

    # CLAP plugin (.clap bundle or module)
    if(TARGET scxt_clapfirst_clap)
        install(TARGETS scxt_clapfirst_clap
            BUNDLE  DESTINATION "${SCXT_CLAP_INSTALL_DIR}"
            LIBRARY DESTINATION "${SCXT_CLAP_INSTALL_DIR}"
            RUNTIME DESTINATION "${SCXT_CLAP_INSTALL_DIR}"
        )
    endif()

    # VST3 plugin (.vst3 bundle)
    if(TARGET scxt_clapfirst_vst3)
        set(v3name "$<TARGET_PROPERTY:scxt_clapfirst_vst3,LIBRARY_OUTPUT_NAME>.vst3")
        set(v3dir "$<TARGET_PROPERTY:scxt_clapfirst_vst3,LIBRARY_OUTPUT_DIRECTORY>/../../..")
        install(DIRECTORY "${v3dir}/${v3name}" DESTINATION "${SCXT_VST3_INSTALL_DIR}")
    endif()

    # Standalone app/executable
    if(TARGET scxt_clapfirst_standalone)
        install(TARGETS scxt_clapfirst_standalone
            BUNDLE  DESTINATION "${SCXT_STANDALONE_INSTALL_DIR}"
            RUNTIME DESTINATION "${SCXT_STANDALONE_INSTALL_DIR}"
        )
    endif()
endif()
