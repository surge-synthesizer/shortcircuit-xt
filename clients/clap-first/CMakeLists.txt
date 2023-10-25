project(scxt_clapfirst)

set(CLAP_TARGET ${PROJECT_NAME}_clap)
add_library(${CLAP_TARGET} MODULE
        scxt-clap-entry.cpp
        clap-config.cpp
        scxt-plugin/scxt-plugin.cpp
        )

if (NOT TARGET clap_juce_shim)
    message(FATAL_ERROR "CJS")
endif()

target_link_libraries(${CLAP_TARGET} PUBLIC
        clap_juce_shim
        clap_juce_shim_headers
        scxt-core
        scxt-ui
        sst-plugininfra
        clap
        clap-helpers
        clap-wrapper-extensions
        )

target_include_directories(${CLAP_TARGET} PUBLIC .)

target_library_make_clap(TARGET ${CLAP_TARGET}
        CLAP_NAME "Shortcircuit XT Clap First"
        CLAP_MACOS_BUNDLE_IDENTIFIER "org.surge-synth-team.scxt.clap_first.clap"
        CLAP_MACOS_BUNDLE_VERSION "0.0.1"
        COPY_AFTER_BUILD TRUE
        )
