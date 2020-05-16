
#
# SLabOpenGL library
#

set  (SOURCES
	ShaderManager.cpp.inl
	ShaderManager.h
	SLabOpenGL.cpp
	SLabOpenGL.h
	SLabOpenGL_Ext.cpp
	SLabOpenGL_Ext.h
	SLabOpenGLMappedBuffer.h
	)

set  (GLAD_SOURCES
	glad/glad.c
	glad/glad.h
	glad/g_glad.h
	glad/khrplatform.h)

source_group(" " FILES ${SOURCES})
source_group("glad" FILES ${GLAD_SOURCES})

add_library (SLabOpenGLLib ${SOURCES} ${GLAD_SOURCES})

target_include_directories(SLabOpenGLLib INTERFACE ..)

target_link_libraries (SLabOpenGLLib
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

	target_link_libraries (SLabOpenGLLib
		${CMAKE_DL_LIBS})

endif()