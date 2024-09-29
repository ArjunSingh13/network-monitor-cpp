cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
project(network-monitor)
add_definitions(-D_WIN32_WINNT=0x0601)
# Add the Conan-generated files to the CMake module path
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

# Find the Boost library with the required components
find_package(Boost 1.78 REQUIRED COMPONENTS filesystem system)

set(SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_executable(network-monitor ${SOURCES})
target_compile_features(network-monitor
    PRIVATE
        cxx_std_11
)

# Link Boost system library
target_link_libraries(network-monitor
    PRIVATE
        Boost::filesystem Boost::system  # Use Boost::system instead of boost::boost or boost::system
)
