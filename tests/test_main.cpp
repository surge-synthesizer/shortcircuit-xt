#define CATCH_CONFIG_RUNNER
#include "catch2/catch2.hpp"
#include "infrastructure/logfile.h"

scxt::log::Level gTestLevel = scxt::log::Level::None;
scxt::log::LoggingCallback *gLogger = 0;

class ConsoleLogger : public scxt::log::LoggingCallback
{
    scxt::log::Level getLevel() { return gTestLevel; }
    void message(scxt::log::Level lev, const std::string &msg)
    {
        std::cout << "TEST " << scxt::log::getShortLevelStr(lev) << msg << std::endl;
    }
};

int main(int argc, char *argv[])
{

    ConsoleLogger *log = new ConsoleLogger();
    gLogger = log;

    int result = Catch::Session().run(argc, argv);

    delete log;
    return result;
}

void *hInstance = 0;

TEST_CASE("Tests Exist", "[basics]")
{
    SECTION("Asset True") { REQUIRE(1); }
}
