
add_executable(gigwritetest main.cpp GigWriteTest.cpp)
target_link_libraries(gigwritetest PRIVATE libgig ${CPPUNIT_LIBRARIES})
target_include_directories(gigwritetest PRIVATE ${CPPUNIT_INCLUDE_DIRS})
add_test(NAME gigwritetest COMMAND gigwritetest)
