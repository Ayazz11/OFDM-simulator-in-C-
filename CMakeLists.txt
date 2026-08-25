cmake_minimum_required(VERSION 3.16)
project(nr_phy_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- Core library: all pipeline blocks except main() ------------------------
add_library(nr_phy_core
    src/LinkSimulator.cpp
)
target_include_directories(nr_phy_core PUBLIC
    include
    external/kissfft_header
)

# --- Main executable ---------------------------------------------------------
add_executable(nr_phy_sim src/main.cpp)
target_link_libraries(nr_phy_sim PRIVATE nr_phy_core)

# --- Multi-modulation sweep (QPSK/16-QAM/64-QAM comparison) ------------------
add_executable(nr_phy_sim_all_mod src/main_all_mod.cpp)
target_link_libraries(nr_phy_sim_all_mod PRIVATE nr_phy_core)

# --- Tests (Catch2, fetched) --------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(nr_phy_tests
        tests/test_qam.cpp
        tests/test_ofdm.cpp
        tests/test_pipeline.cpp
    )
    target_link_libraries(nr_phy_tests PRIVATE nr_phy_core Catch2::Catch2WithMain)

    include(CTest)
    include(Catch)
    catch_discover_tests(nr_phy_tests)
endif()
