project(scxt_clapfirst)

set(IMPL_TARGET ${PROJECT_NAME}_impl)
add_library(${IMPL_TARGET} STATIC clap-config.cpp
        scxt-plugin/scxt-plugin.cpp)

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

if (APPLE)
    set(AUV2_TARGET ${PROJECT_NAME}_auv2)
    add_library(${AUV2_TARGET} MODULE)
    target_sources(${AUV2_TARGET} PRIVATE  scxt-clap-entry.cpp)
    target_link_libraries(${AUV2_TARGET} PRIVATE ${IMPL_TARGET})
    target_add_auv2_wrapper(
            TARGET ${AUV2_TARGET}
            OUTPUT_NAME "Shortcircuit XT Clap First"
            BUNDLE_IDENTIFIER "org.surge-synth-team.scxt.clap_first.auv2"
            BUNDLE_VERSION 0.0.1

            CLAP_TARGET_FOR_CONFIG ${CLAP_TARGET}
    )
    target_auv2_copy_after_build(TARGET ${AUV2_TARGET})
endif()

set(SA_TARGET ${PROJECT_NAME}_standalone)
add_executable(${SA_TARGET} scxt-clap-entry.cpp)
target_link_libraries(${SA_TARGET} PRIVATE ${IMPL_TARGET})
target_add_standalone_wrapper(TARGET ${SA_TARGET}
        OUTPUT_NAME "Shortcircuit XT Clap First"
        STATICALLY_LINKED_CLAP_ENTRY True
        PLUGIN_ID "org.surge-synth-team.scxt.clap-first.scxt-plugin")
