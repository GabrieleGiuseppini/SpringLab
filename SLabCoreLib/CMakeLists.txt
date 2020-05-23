
#
# SLabCoreLib library
#

set  (SOURCES
	AABB.h
	Buffer.h
	CircularList.h
	Colors.cpp
	Colors.h
	ElementContainer.h
	ElementIndexRangeIterator.h
	EventDispatcher.h
	FixedSizeVector.h
	FileSystem.h
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
	Settings.cpp
	Settings.h
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
	TaskThread.cpp
	TaskThread.h
	Utils.cpp
	Utils.h	
	Vectors.cpp
	Vectors.h
	Version.h
	ViewModel.h
)

set  (SIMULATOR_COMMON_SOURCES
	Simulator/Common/ISimulator.h
	Simulator/Common/CommonSimulatorParameters.cpp
	Simulator/Common/CommonSimulatorParameters.h
	Simulator/Common/SimulatorRegistry.cpp
	Simulator/Common/SimulatorRegistry.h
)

set  (SIMULATOR_CLASSIC_SOURCES
	Simulator/Classic/ClassicSimulator.cpp
	Simulator/Classic/ClassicSimulator.h
	Simulator/Classic/ClassicSimulatorParameters.h
	Simulator/Classic/ClassicSimulatorParameters.cpp
)

set  (GLAD_SOURCES
	glad/glad.c
	glad/glad.h
	glad/g_glad.h
	glad/khrplatform.h
)

source_group(" " FILES ${SOURCES})
source_group("glad" FILES ${GLAD_SOURCES})
source_group("Simulator/Common" FILES ${SIMULATOR_COMMON_SOURCES})
source_group("Simulator/Classic" FILES ${SIMULATOR_CLASSIC_SOURCES})

add_library (SLabCoreLib 
	${SOURCES} 
	${GLAD_SOURCES} 
	${SIMULATOR_COMMON_SOURCES}
	${SIMULATOR_CLASSIC_SOURCES})

target_include_directories(SLabCoreLib PRIVATE ${IL_INCLUDE_DIR})
target_include_directories(SLabCoreLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(SLabCoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
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

