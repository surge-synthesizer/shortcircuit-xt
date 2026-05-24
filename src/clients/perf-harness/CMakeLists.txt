project(perf-harness)

add_executable(scxt-perf
        perf_main.cpp
        perf_config.cpp
        perf_loader.cpp
        perf_generator.cpp
        perf_runner.cpp
        perf_report.cpp
)
target_link_libraries(scxt-perf
        scxt-core
        fmt
)
