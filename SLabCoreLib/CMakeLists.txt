
#
# SLabCoreLib library
#

set  (SOURCES
	AABB.h
	Buffer.h
	CacheModel.h
	Chronometer.h
	CircularList.h
	Colors.cpp
	Colors.h
	ElementContainer.h
	ElementIndexRangeIterator.h
	EventDispatcher.h
	FixedSizeVector.h
	FileSystem.h
	FloatingPoint.h
	ILayoutOptimizer.h
	ImageData.h
	ImageFileTools.cpp
	ImageFileTools.h
	ImageSize.h
	IndexRemap.h
	ISimulationEventHandler.h
	Log.cpp
	Log.h
	Matrix.h
	Object.h
	ObjectBuilder.cpp
	ObjectBuilder.h
	ObjectBuilderTypes.h
	ObjectDefinition.cpp
	ObjectDefinition.h
	ObjectSimulatorSpecificStructure.h
	ParallelThreadPool.cpp
	ParallelThreadPool.h
	PerfStats.h
	Points.cpp
	Points.h
	RenderContext.cpp
	RenderContext.h
	ResourceLocator.h
	RunningAverage.h
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
	TaskThreadPool.cpp
	TaskThreadPool.h
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
	Simulator/Classic/ClassicSimulatorParameters.cpp
	Simulator/Classic/ClassicSimulatorParameters.h
)

set  (SIMULATOR_FAST_MSS_SOURCES
	Simulator/FastMSS/FastMSSBasicSimulator.cpp
	Simulator/FastMSS/FastMSSBasicSimulator.h
	Simulator/FastMSS/FastMSSCommonSimulatorParameters.cpp
	Simulator/FastMSS/FastMSSCommonSimulatorParameters.h
)

set  (SIMULATOR_FS_SOURCES
	Simulator/FS/FSBaseSimulator.cpp
	Simulator/FS/FSBaseSimulator.h
	Simulator/FS/FSByPointSimulator.cpp
	Simulator/FS/FSByPointSimulator.h
	Simulator/FS/FSByPointCompactSimulator.cpp
	Simulator/FS/FSByPointCompactSimulator.h
	Simulator/FS/FSByPointCompactIntegratingSimulator.cpp
	Simulator/FS/FSByPointCompactIntegratingSimulator.h
	Simulator/FS/FSBySpringIntrinsicsSimulator.cpp
	Simulator/FS/FSBySpringIntrinsicsSimulator.h
	Simulator/FS/FSBySpringIntrinsicsLayoutOptimizationSimulator.cpp
	Simulator/FS/FSBySpringIntrinsicsLayoutOptimizationSimulator.h
	Simulator/FS/FSBySpringStructuralIntrinsicsMTSimulator.cpp
	Simulator/FS/FSBySpringStructuralIntrinsicsMTSimulator.h
	Simulator/FS/FSBySpringStructuralIntrinsicsMTVectorizedSimulator.cpp
	Simulator/FS/FSBySpringStructuralIntrinsicsMTVectorizedSimulator.h
	Simulator/FS/FSBySpringStructuralIntrinsicsSimulator.cpp
	Simulator/FS/FSBySpringStructuralIntrinsicsSimulator.h
	Simulator/FS/FSCommonSimulatorParameters.cpp
	Simulator/FS/FSCommonSimulatorParameters.h
)

set  (SIMULATOR_GAUSS_SEIDEL_SOURCES
	Simulator/GaussSeidel/GaussSeidelByPointSimulator.cpp
	Simulator/GaussSeidel/GaussSeidelByPointSimulator.h
	Simulator/GaussSeidel/GaussSeidelCommonSimulatorParameters.cpp
	Simulator/GaussSeidel/GaussSeidelCommonSimulatorParameters.h
)

set  (SIMULATOR_POSITION_BASED_SOURCES
	Simulator/PositionBased/PositionBasedBasicSimulator.cpp
	Simulator/PositionBased/PositionBasedBasicSimulator.h
	Simulator/PositionBased/PositionBasedCommonSimulatorParameters.cpp
	Simulator/PositionBased/PositionBasedCommonSimulatorParameters.h
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
source_group("Simulator/FastMSS" FILES ${SIMULATOR_FAST_MSS_SOURCES})
source_group("Simulator/FS" FILES ${SIMULATOR_FS_SOURCES})
source_group("Simulator/GaussSeidel" FILES ${SIMULATOR_GAUSS_SEIDEL_SOURCES})
source_group("Simulator/PositionBased" FILES ${SIMULATOR_POSITION_BASED_SOURCES})

add_library (SLabCoreLib 
	${SOURCES} 
	${GLAD_SOURCES} 
	${SIMULATOR_COMMON_SOURCES}
	${SIMULATOR_CLASSIC_SOURCES}
	${SIMULATOR_FAST_MSS_SOURCES}
	${SIMULATOR_FS_SOURCES}
	${SIMULATOR_GAUSS_SEIDEL_SOURCES}
	${SIMULATOR_POSITION_BASED_SOURCES})

target_include_directories(SLabCoreLib PRIVATE ${IL_INCLUDE_DIR})
target_include_directories(SLabCoreLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(SLabCoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(SLabCoreLib INTERFACE ..)

target_link_libraries (SLabCoreLib
	eigen
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ADDITIONAL_LIBRARIES}
	)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

	target_link_libraries (SLabCoreLib
		"stdc++fs")

endif()

