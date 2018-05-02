find_package(Qt5Test 5.2.0 REQUIRED NO_MODULE)

add_library(qdatacubetestlib danishnamecube.cpp modeltest.cpp)
target_link_libraries(qdatacubetestlib qdatacube)

add_executable(testplaincube testplaincube.cpp)
target_link_libraries(testplaincube qdatacubetestlib Qt5::Test)
add_test(testplaincube testplaincube)

add_executable(testdatacube testdatacube.cpp)
target_link_libraries(testdatacube qdatacubetestlib Qt5::Test)
add_test(testdatacube testdatacube)

# An interactive test application
add_executable(testheaders testheaders.cpp)
target_link_libraries(testheaders qdatacubetestlib Qt5::Test)
