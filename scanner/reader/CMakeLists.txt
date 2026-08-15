cmake_minimum_required(VERSION 3.16)
project(obd2_rpi VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Opciones de compilación ────────────────────────────────────────────────
option(BUILD_TESTS    "Compilar tests"        OFF)
option(ENABLE_GM      "Soporte GM modo 22"    ON)
option(ENABLE_LOGGING "Logging CSV"           ON)
option(INSTALL_SERVICE "Instalar systemd"     OFF)

# ─── Flags según plataforma (solo ARM nativo) ───────────────────────────────
include(CheckCXXCompilerFlag)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(ARCH_FLAGS "-O2 -march=armv8-a")
    else()
        set(ARCH_FLAGS "-O2 -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS}")
    message(STATUS "Arquitectura ARM: ${CMAKE_SYSTEM_PROCESSOR}")
else()
    message(STATUS "Arquitectura NO-ARM: ${CMAKE_SYSTEM_PROCESSOR} (compilacion cruzada/simulacion)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ─── Dependencias ───────────────────────────────────────────────────────────
find_package(Threads REQUIRED)

find_library(GPIOD_LIB gpiod REQUIRED)
find_path(GPIOD_INCLUDE gpiod.h REQUIRED)

include_directories(include ${GPIOD_INCLUDE})

# ─── Fuentes ────────────────────────────────────────────────────────────────
file(GLOB SOURCES src/*.cpp)

add_executable(obd2_rpi ${SOURCES})

target_link_libraries(obd2_rpi
    bluetooth
    Threads::Threads
    ${GPIOD_LIB}
)

# ─── Instalación ────────────────────────────────────────────────────────────
install(TARGETS obd2_rpi RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/obd2_rpi)
install(FILES obd2_rpi.service DESTINATION lib/systemd/system)

# ─── Tests ──────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Version       : ${PROJECT_VERSION}")
message(STATUS "Binario       : ${CMAKE_SOURCE_DIR}/bin/obd2_rpi")
message(STATUS "GM soporte    : ${ENABLE_GM}")
message(STATUS "Tests         : ${BUILD_TESTS}")
