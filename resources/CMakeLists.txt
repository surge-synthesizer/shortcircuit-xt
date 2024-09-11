project(scxt-resources)

cmrc_add_resource_library(${PROJECT_NAME}
        NAMESPACE scxt_resources
        "fonts/Inter/static/Inter-Light.ttf"
        "fonts/Inter/static/Inter-Regular.ttf"
        "fonts/Inter/static/Inter-Medium.ttf"
        "fonts/Inter/static/Inter-Bold.ttf"
        "fonts/Anonymous_Pro/AnonymousPro-Regular.ttf"
        "images/SCicon.svg"
        "opensource-credits.txt"
        )
set_target_properties(scxt-resources PROPERTIES UNITY_BUILD FALSE)


cmrc_add_resource_library(${PROJECT_NAME}-core
        NAMESPACE scxt_resources_core
        InitSettings.dat
)
set_target_properties(scxt-resources-core PROPERTIES UNITY_BUILD FALSE)
