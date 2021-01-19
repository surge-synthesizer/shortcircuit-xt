

include(CheckSymbolExists)
check_symbol_exists(clock_gettime sys/time.h HAVE_CLOCK_GETTIME)
check_symbol_exists(gettimeofday sys/time.h HAVE_GETTIMEOFDAY)
check_symbol_exists(ftime sys/timeb.h HAVE_FTIME)

configure_file(config.h.in config.h)
message( STATUS "Configuring ${SHORTCBLD}/geninclude/sc3_config.h" )
configure_file( ${SHORTCSRC}/src/sc3_config.h.in
        ${SHORTCBLD}/geninclude/sc3_config.h )

