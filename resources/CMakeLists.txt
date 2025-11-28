project(scxt-resources)

cmrc_add_resource_library(${PROJECT_NAME}
        NAMESPACE scxt_resources
        "fonts/Inter/static/Inter-Light.ttf"
        "fonts/Inter/static/Inter-Regular.ttf"
        "fonts/Inter/static/Inter-Medium.ttf"
        "fonts/Inter/static/Inter-Bold.ttf"
        "fonts/Anonymous_Pro/AnonymousPro-Regular.ttf"
        "images/SCAppIcon.svg"
        "images/SCIcon.svg"
        "images/about-icons.svg"
        "opensource-credits.txt"
        )
set_target_properties(scxt-resources PROPERTIES UNITY_BUILD FALSE)

# To update these
# 1. Save an SCM
# 2. Build the 'init-maker' utility
# 3. Run it to make the .dat output per the source
cmrc_add_resource_library(${PROJECT_NAME}-core
        NAMESPACE scxt_resources_core
        "init_states/InitSampler.dat"
        "init_states/InitSamplerMulti.dat"
        "init_states/InitSynth.dat"
)
set_target_properties(scxt-resources-core PROPERTIES UNITY_BUILD FALSE)
