# Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
# Author: Sorin Mihai Grigorescu

set(FILTER_NAME Filter_LoMaCoR_Maps)

if (NOT WIN32)
    set(CMAKE_SKIP_RPATH TRUE)

    find_package(fmt CONFIG REQUIRED)
    find_package(CURL REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(nlohmann_json REQUIRED)

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
    FetchContent_MakeAvailable(cpr_external)
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
    target_link_directories(lomacor PUBLIC ${CyC_THIRD_PARTY}/libzip/${CyC_LIBS_BINARY_KEY}/lib)
    target_link_libraries(lomacor PUBLIC zip)
endif ()

add_library(${FILTER_NAME} SHARED
	src/CLomacorMapsFilter.h
	src/CLomacorMapsFilter.cpp)

target_link_libraries(${FILTER_NAME}
	PUBLIC
        lomacor
        CycCore
        CycToolkit
        ${CyC_OPENCV_WORLD})

target_link_directories(${FILTER_NAME}
    PUBLIC 
        ${CyC_THIRD_PARTY_LIBRARY_DIRECTORIES})

if (WIN32)
    target_link_libraries(${FILTER_NAME} PUBLIC libconfig++)
else ()
    target_link_libraries(${FILTER_NAME} PUBLIC config++)
endif (WIN32)

# Build the filter into the filters folder
set_target_properties(${FILTER_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
)
# Override config-specific paths to keep everything in the same folder
set_target_properties(${FILTER_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"

    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"

    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CyC_FILTERS_PATH}"
)

# Zenodo test unit
add_executable(tu_Zenodo src/tu_Zenodo.cpp)
target_include_directories(tu_Zenodo PUBLIC ${CyC_THIRD_PARTY}/nlohmann-json/include)
target_link_libraries(tu_Zenodo PUBLIC CycToolkit ${CyC_OPENCV_WORLD} cpr lomacor)
if (NOT WIN32)
    target_link_libraries(tu_Zenodo PUBLIC fmt::fmt)
endif ()

# LoMaCoR state machine test unit
add_executable(tu_Lomacor src/tu_Lomacor.cpp)
target_link_libraries(tu_Lomacor PUBLIC CycToolkit ${CyC_OPENCV_WORLD} lomacor)
if (NOT WIN32)
    target_link_libraries(tu_Lomacor PUBLIC fmt::fmt)
endif ()
