#
# https://cmake.org/Wiki/CMake_Useful_Variables
#

# установка параметров, необходимых для сборки проекта
include( "${trCMAKES}/params.cmake" )
#include( "${trCMAKES}/server.cmake" )

# подключение исходных файлов библиотек
include( "${trCMAKES}/extlibs/glcore33.cmake" )

# где искать заголовки
include_directories( ${trINCLUDE_DIRS} )

find_package( PkgConfig REQUIRED )
pkg_check_modules( PNG REQUIRED libpng16 )
pkg_check_modules( GLFW REQUIRED glfw3 )
pkg_check_modules( SQLITE REQUIRED sqlite3 )

## Список библиотек
SET( trLIBS ${trLIBS} ${GLFW_LIBRARIES} ${PNG_LIBRARIES} ${SQLITE_LIBRARIES}
  pthread stdc++fs glcore33 )

add_executable( ${EXEC_NAME} ${trSRC} )
target_link_libraries( ${EXEC_NAME} ${trLIBS} )
