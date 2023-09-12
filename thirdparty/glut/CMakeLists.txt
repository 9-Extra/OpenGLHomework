set(GLUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(glut INTERFACE)

target_link_libraries(glut INTERFACE "${GLUT_DIR}/lib/glew32.lib" "${GLUT_DIR}/lib/glut32.lib" "Glu32.lib" "Opengl32.lib")
target_sources( glut INTERFACE
	FILE_SET HEADERS
	BASE_DIRS "${GLUT_DIR}/include"
)
