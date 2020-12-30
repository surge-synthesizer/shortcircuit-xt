#define CATCH_CONFIG_MAIN
#include "catch2/catch2.hpp"


void *hInstance = 0;

TEST_CASE("Tests Exist", "[basics]")
{
    SECTION("Asset True") { REQUIRE(1); }
}