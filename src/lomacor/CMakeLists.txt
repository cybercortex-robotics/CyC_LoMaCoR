# Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
# Author: Sorin Mihai Grigorescu

include_directories("${CyC_THIRD_PARTY}/nlohmann-json/include")
include_directories("${CyC_THIRD_PARTY}/spdlog-1.4.2/include")
include_directories("${CyC_THIRD_PARTY}/libconfig/include")

if (NOT WIN32)
    find_package(CURL REQUIRED)
    include(FetchContent)
    
    FetchContent_Declare(
      cpr_external
      GIT_REPOSITORY    https://github.com/libcpr/cpr.git
      GIT_TAG           master
      # Tell cpr to use the system's libcurl installation
      CMAKE_ARGS        -DCPR_USE_SYSTEM_CURL=ON 
                        -DCPR_BUILD_TESTS=OFF 
                        -DCPR_BUILD_EXAMPLES=OFF
    )
    set(CPR_USE_SYSTEM_CURL ON CACHE INTERNAL "")
    set(CPR_BUILD_TESTS OFF CACHE INTERNAL "")
    
    FetchContent_MakeAvailable(cpr_external)
    
    find_package(fmt CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(nlohmann_json REQUIRED)
endif()

add_library(lomacor STATIC
    src/CZenodo.h
    src/CZenodo.cpp
    src/CStateMachine.h
    src/CStateMachine.cpp
    src/CZip.h
    src/CZip.cpp)

if (NOT WIN32)
    target_include_directories(lomacor PUBLIC $<TARGET_PROPERTY:cpr::cpr,INTERFACE_INCLUDE_DIRECTORIES> /usr/include/nlohmann)
    target_link_directories(lomacor PUBLIC ${CyC_THIRD_PARTY}/libzip/${CyC_LIBS_BINARY_KEY}/lib)
    target_link_libraries(lomacor PUBLIC zip cpr::cpr ${CURL_LIBRARIES} ${ZLIB_LIBRARIES})
elseif (WIN32)
    target_include_directories(lomacor PUBLIC ${CyC_THIRD_PARTY}/libzip/include)
    target_link_directories(lomacor PUBLIC ${CyC_THIRD_PARTY}/libzip/win-msvc-x64/lib)
    target_link_directories(lomacor PUBLIC ${CyC_THIRD_PARTY}/libconfig/win-msvc-x64/lib)
    target_link_libraries(lomacor PUBLIC zip)
endif ()

# Zenodo test unit
add_executable(tu_Zenodo src/tu_Zenodo.cpp)
target_include_directories(tu_Zenodo PUBLIC ${CyC_THIRD_PARTY}/nlohmann-json/include)
target_link_libraries(tu_Zenodo PUBLIC cpr lomacor)
if (WIN32)
    target_link_libraries(tu_Zenodo PUBLIC libconfig++)
else ()
    target_link_libraries(tu_Zenodo PUBLIC fmt::fmt config++)
endif (WIN32)

# LoMaCoR state machine test unit
add_executable(tu_Lomacor src/tu_Lomacor.cpp)
target_link_libraries(tu_Lomacor PUBLIC lomacor)
if (WIN32)
    target_link_libraries(tu_Lomacor PUBLIC libconfig++)
else ()
    target_link_libraries(tu_Lomacor PUBLIC fmt::fmt config++)
endif (WIN32)
