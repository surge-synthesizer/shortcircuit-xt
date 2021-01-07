#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"
#include "infrastructure/logfile.h"


SC3::Log::Level gTestLevel = SC3::Log::Level::None;
SC3::Log::LoggingCallback *gLogger=0;


class ConsoleLogger : public SC3::Log::LoggingCallback {
    SC3::Log::Level getLevel() { return gTestLevel;}
    void message(SC3::Log::Level  lev, const std::string &msg) {
        std::cout << "TEST " << SC3::Log::getShortLevelStr(lev) << msg << std::endl;
    }
};

int main( int argc, char* argv[] ) {
  
    ConsoleLogger *log=new ConsoleLogger();
    gLogger=log;

    int result = Catch::Session().run( argc, argv );

    delete log;
    return result;
}

void *hInstance = 0;

TEST_CASE("Tests Exist", "[basics]")
{
    SECTION("Asset True") { REQUIRE(1); }
}

