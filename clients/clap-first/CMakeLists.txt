project(scxt_clapfirst)

message(STATUS "Shortcircuit XT building clap-first wrappers")
set(PRODUCT_NAME "Shortcircuit XT")
set(IMPL_TARGET ${PROJECT_NAME}_impl)
add_library(${IMPL_TARGET} STATIC
        scxt-plugin/clap-helpers-config.cpp
        scxt-plugin/scxt-plugin.cpp
        scxt-plugin/scxt-clap-entry-impl.cpp
)

target_link_libraries(${IMPL_TARGET} PRIVATE fmt)

target_link_libraries(${IMPL_TARGET} PUBLIC
        clap_juce_shim_headers
        scxt-core
        scxt-ui
        sst-plugininfra
        clap
        clap-helpers
        clap-wrapper-extensions
        clap_juce_shim
        )

target_include_directories(${IMPL_TARGET} PUBLIC .)

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
        STANDALONE_WINDOWS_ICON "${CMAKE_SOURCE_DIR}/resources/installer_win/scxt.ico"
)

# Our installer uses this
set_target_properties(${PROJECT_NAME}_all PROPERTIES ARTEFACT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets)

target_compile_definitions(clap-wrapper-compile-options-public INTERFACE CLAP_WRAPPER_LOGLEVEL=0)