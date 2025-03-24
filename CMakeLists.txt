cmake_minimum_required(VERSION 3.10)
project(Nextron)

set(CMAKE_CXX_STANDARD 17)

# Пути к заголовочным файлам
target_include_directories(Nextron PRIVATE 
    ${CMAKE_SOURCE_DIR}/engine 
    ${CMAKE_SOURCE_DIR}/include
)

# Поиск исходников автоматически
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    engine/**/*.cpp
    gameplay/**/*.cpp
)

# Создаём исполняемый файл
add_executable(Nextron ${SOURCES})

# Подключаем OpenGL и GLFW
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Подключаем библиотеки
target_link_libraries(Nextron PRIVATE glfw OpenGL::GL)

# Если GLFW лежит в папке lib
# target_link_directories(Nextron PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(Nextron PRIVATE glfw3)

# Копируем DLL после сборки (для Windows)
if (WIN32)
    add_custom_command(TARGET Nextron POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/glfw3.dll" $<TARGET_FILE_DIR:Nextron>)
endif()
