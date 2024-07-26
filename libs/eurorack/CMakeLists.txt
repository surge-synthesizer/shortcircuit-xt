project(eurorack VERSION 0.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME})

option(EURORACK_CLOUDS_IS_CLOUDS "Use the clouds (as opposed to superparasites) firmware for clouds" FALSE)

if(EURORACK_CLOUDS_IS_CLOUDS)
  target_sources(${PROJECT_NAME} PRIVATE
    eurorack/clouds/dsp/correlator.cc
    eurorack/clouds/dsp/granular_processor.cc
    eurorack/clouds/dsp/mu_law.cc
    eurorack/clouds/dsp/pvoc/frame_transformation.cc
    eurorack/clouds/dsp/pvoc/phase_vocoder.cc
    eurorack/clouds/dsp/pvoc/stft.cc
    eurorack/clouds/resources.cc)
  target_compile_definitions(${PROJECT_NAME} PUBLIC EURORACK_CLOUDS_IS_CLOUDS=1)
else()
  target_sources(${PROJECT_NAME} PRIVATE
    eurorack/supercell/resources.cc
    eurorack/supercell/dsp/kammerl_player.cc
    eurorack/supercell/dsp/pvoc/stft.cc
    eurorack/supercell/dsp/pvoc/spectral_clouds_transformation.cc
    eurorack/supercell/dsp/pvoc/phase_vocoder.cc
    eurorack/supercell/dsp/pvoc/frame_transformation.cc
    eurorack/supercell/dsp/granular_processor.cc
    eurorack/supercell/dsp/mu_law.cc
    eurorack/supercell/dsp/correlator.cc
  )
  target_compile_definitions(${PROJECT_NAME} PUBLIC EURORACK_CLOUDS_IS_SUPERPARASITES=1)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  eurorack/stmlib/dsp/units.cc
  eurorack/stmlib/dsp/atan.cc

  eurorack/stmlib/utils/random.cc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  TEST=1
  $<$<BOOL:${WIN32}>:_USE_MATH_DEFINES>
)

# MSVC and its pretenders
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE /FImsvc_gcc_defines.h)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_NAME} special_include)
add_library(scxt::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
