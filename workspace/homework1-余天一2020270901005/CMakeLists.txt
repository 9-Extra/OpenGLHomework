set(TARGET_NAME homework1)

add_executable(${TARGET_NAME})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.cpp *.h)
target_sources(${TARGET_NAME} PUBLIC ${sources})

target_include_directories(${TARGET_NAME} PUBLIC ./src)

# # 操作系统库，只需要链接
# target_link_libraries(${TARGET_NAME} PUBLIC "icu.lib")
# 第三方的库
target_link_libraries(${TARGET_NAME} PUBLIC glut PUBLIC freeimage)

# 设置调试时的工作目录
set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_ROOT}/bin")


# 启用警告，但不将警告视为错误
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/W3>")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")
# 禁用 “找不到链接对象调试信息”警告
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd4099>")
