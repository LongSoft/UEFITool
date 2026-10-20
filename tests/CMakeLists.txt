CMAKE_MINIMUM_REQUIRED(VERSION 3.22)
PROJECT(DescriptorAccessTests LANGUAGES C CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
ENABLE_TESTING()

ADD_EXECUTABLE(descriptor_access_tests
 descriptor_access_tests.cpp
 ../common/descriptor.cpp
 ../common/ustring.cpp
 ../common/bstrlib/bstrlib.c
 ../common/bstrlib/bstrwrap.cpp
)
ADD_TEST(NAME descriptor_access COMMAND descriptor_access_tests)
