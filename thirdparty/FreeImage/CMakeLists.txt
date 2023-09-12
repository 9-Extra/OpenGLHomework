set(FREEIMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(freeimage INTERFACE)

target_link_libraries(freeimage INTERFACE "${FREEIMAGE_DIR}/lib/FreeImage.lib")
target_sources(freeimage INTERFACE
	FILE_SET HEADERS
	BASE_DIRS "${FREEIMAGE_DIR}/include"
)
