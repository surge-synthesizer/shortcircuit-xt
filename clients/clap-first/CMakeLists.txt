project(scxt_clapfirst)

set(IMPL_TARGET ${PROJECT_NAME}_impl)
add_library(${IMPL_TARGET} STATIC clap-config.cpp
        scxt-plugin/scxt-plugin.cpp)

target_link_libraries(${IMPL_TARGET} PUBLIC
        clap_juce_shim
        clap_juce_shim_headers
        scxt-core
        scxt-ui
        sst-plugininfra
        clap
        clap-helpers
        clap-wrapper-extensions
        )
target_include_directories(${IMPL_TARGET} PUBLIC .)

set(CLAP_TARGET ${PROJECT_NAME}_clap)
add_library(${CLAP_TARGET} MODULE scxt-clap-entry.cpp)
target_link_libraries(${CLAP_TARGET} PUBLIC ${IMPL_TARGET})
target_library_make_clap(TARGET ${CLAP_TARGET}
        CLAP_NAME "Shortcircuit XT Clap First"
        CLAP_MACOS_BUNDLE_IDENTIFIER "org.surge-synth-team.scxt.clap_first.clap"
        CLAP_MACOS_BUNDLE_VERSION "0.0.1"
        COPY_AFTER_BUILD TRUE
        )


set(VST3_TARGET ${PROJECT_NAME}_vst3)
add_library(${VST3_TARGET} MODULE scxt-clap-entry.cpp)
target_link_libraries(${VST3_TARGET} PUBLIC ${IMPL_TARGET})
target_add_vst3_wrapper(TARGET ${VST3_TARGET}
        OUTPUT_NAME "Shortcircuit XT Clap First"
        SUPPORTS_ALL_NOTE_EXPRESSIONS TRUE
        )
target_vst3_copy_after_build(TARGET ${VST3_TARGET})