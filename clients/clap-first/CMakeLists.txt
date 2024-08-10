project(scxt_clapfirst)

set(name "Shortcircuit XT")
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

set(CLAP_TARGET ${PROJECT_NAME}_CLAP)
add_library(${CLAP_TARGET} MODULE scxt-clap-entry.cpp)
target_link_libraries(${CLAP_TARGET} PUBLIC ${IMPL_TARGET})
target_library_make_clap(TARGET ${CLAP_TARGET}
        CLAP_NAME ${name}
        CLAP_MACOS_BUNDLE_IDENTIFIER "org.surge-synth-team.scxt.clap_first.clap"
        CLAP_MACOS_BUNDLE_VERSION "0.0.1"
        COPY_AFTER_BUILD ${SCXT_COPY_PLUGIN_AFTER_BUILD}
        )


set(VST3_TARGET ${PROJECT_NAME}_VST3)
add_library(${VST3_TARGET} MODULE scxt-clap-entry.cpp)
target_link_libraries(${VST3_TARGET} PUBLIC ${IMPL_TARGET})
target_add_vst3_wrapper(TARGET ${VST3_TARGET}
        OUTPUT_NAME ${name}
        SUPPORTS_ALL_NOTE_EXPRESSIONS TRUE
        )
if (${SCXT_COPY_PLUGIN_AFTER_BUILD})
    target_vst3_copy_after_build(TARGET ${VST3_TARGET})
endif()

if (APPLE)
    set(AUV2_TARGET ${PROJECT_NAME}_AU)
    add_library(${AUV2_TARGET} MODULE)
    target_sources(${AUV2_TARGET} PRIVATE  scxt-clap-entry.cpp)
    target_link_libraries(${AUV2_TARGET} PRIVATE ${IMPL_TARGET})
    target_add_auv2_wrapper(
            TARGET ${AUV2_TARGET}
            OUTPUT_NAME ${name}
            BUNDLE_IDENTIFIER "org.surge-synth-team.scxt.clap_first.auv2"
            BUNDLE_VERSION 0.0.1

            CLAP_TARGET_FOR_CONFIG ${CLAP_TARGET}
    )
    if (${SCXT_COPY_PLUGIN_AFTER_BUILD})
        target_auv2_copy_after_build(TARGET ${AUV2_TARGET})
    endif()
endif()

set(SA_TARGET ${PROJECT_NAME}_Standalone)
add_executable(${SA_TARGET} MACOSX_BUNDLE WIN32)
set_target_properties(${SA_TARGET} PROPERTIES
            OUTPUT_NAME ${name})
target_sources(${SA_TARGET} PRIVATE scxt-juce-standalone/scxt-juce-standalone.cpp)
target_link_libraries(${SA_TARGET} PRIVATE ${IMPL_TARGET} juce::juce_audio_devices juce::juce_audio_utils)
target_compile_definitions(${SA_TARGET} PRIVATE JUCE_USE_FLAC=0)

#add_executable(${SA_TARGET} scxt-clap-entry.cpp)
#target_link_libraries(${SA_TARGET} PRIVATE ${IMPL_TARGET})
#target_add_standalone_wrapper(TARGET ${SA_TARGET}
#        OUTPUT_NAME ${name}
#        STATICALLY_LINKED_CLAP_ENTRY True
#        PLUGIN_ID "org.surge-synth-team.scxt.clap-first.scxt-plugin")

set(ALL_TARGET ${PROJECT_NAME}_all)
add_custom_target(${ALL_TARGET})
add_dependencies(${ALL_TARGET} ${CLAP_TARGET} ${SA_TARGET} ${VST3_TARGET})
if (APPLE)
    add_dependencies(${ALL_TARGET} ${AUV2_TARGET})
endif()

set_target_properties(${ALL_TARGET} PROPERTIES ARTEFACT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
