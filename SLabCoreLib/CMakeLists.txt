
#
# SLabCoreLib library
#

set  (SOURCES
	AABB.h
	Buffer.h
	BufferAllocator.h
	CircularList.h
	Colors.cpp
	Colors.h
	ElementContainer.h
	ElementIndexRangeIterator.h
	EventDispatcher.h
	FixedSizeVector.h
	FloatingPoint.h
	ImageData.h
	ImageFileTools.cpp
	ImageFileTools.h
	ImageSize.h
	ISimulationEventHandler.h
	Log.cpp
	Log.h
	Object.h
	ObjectBuilder.cpp
	ObjectBuilder.h
	ObjectDefinition.cpp
	ObjectDefinition.h
	Points.cpp
	Points.h
	RenderContext.cpp
	RenderContext.h
	ResourceLocator.h
	ShaderManager.cpp
	ShaderManager.h
	SimulationController.cpp
	SimulationController.h
	SimulationController_Interactions.cpp
	SimulationParameters.cpp
	SimulationParameters.h
	SLabDebug.h
	SLabException.h
	SLabMath.h
	SLabOpenGL.cpp
	SLabOpenGL.h
	SLabOpenGL_Ext.cpp
	SLabOpenGL_Ext.h
	SLabOpenGLMappedBuffer.h
	SLabRandomEngine.h
	SLabTypes.cpp
	SLabTypes.h
	SLabWallClock.h
	Springs.cpp
	Springs.h
	StructuralMaterial.h
	StructuralMaterialDatabase.h
	StructuralMaterial.cpp
	SysSpecifics.h
	Utils.cpp
	Utils.h	
	Vectors.cpp
	Vectors.h
	Version.h
	ViewModel.h
)

set  (GLAD_SOURCES
	glad/glad.c
	glad/glad.h
	glad/g_glad.h
	glad/khrplatform.h)

source_group(" " FILES ${SOURCES})
source_group("glad" FILES ${GLAD_SOURCES})

add_library (SLabCoreLib ${SOURCES} ${GLAD_SOURCES})

target_include_directories(SLabCoreLib PRIVATE ${IL_INCLUDE_DIR})
target_include_directories(SLabCoreLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(SLabCoreLib INTERFACE ..)

target_link_libraries (SLabCoreLib
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

	target_link_libraries (SLabCoreLib
		"stdc++fs")

endif()

