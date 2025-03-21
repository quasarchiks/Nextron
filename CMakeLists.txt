cmake_minimum_required(VERSION 3.10)
project(Nextron)

set(CMAKE_CXX_STANDARD 17)

# Пути к SFML (локальная папка в проекте)
set(SFML_DIR "${CMAKE_SOURCE_DIR}/SFML")

# Добавляем SFML в проект
find_package(SFML REQUIRED COMPONENTS graphics window system)

add_executable(Nextron src/main.cpp)

# Указываем пути к SFML
target_include_directories(Nextron PRIVATE ${SFML_DIR}/include)
target_link_directories(Nextron PRIVATE ${SFML_DIR}/lib)
target_link_libraries(Nextron PRIVATE sfml-graphics sfml-window sfml-system)

if (WIN32)
    add_custom_command(TARGET Nextron POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SFML_DIR}/bin" $<TARGET_FILE_DIR:Nextron>)
endif()
