# Spécifie la version minimale de CMake requise
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Nom du projet
project(foieq)

find_package(Crow)
set(CROW_FEATURES "ssl;compression")
add_subdirectory(lib/SQLiteCpp)
find_package(OpenSSL REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/lib")
include_directories("${PROJECT_SOURCE_DIR}/include")

# Ajoute l'exécutable
add_executable(foieq main.cpp)
target_link_libraries(foieq PUBLIC Crow::Crow SQLiteCpp
  sqlite3
  OpenSSL::SSL
  OpenSSL::Crypto
  pthread
  dl)
