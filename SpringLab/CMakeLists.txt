
#
# SpringLab application
#

set  (SPRING_LAB_SOURCES
	AboutDialog.cpp
	AboutDialog.h
	ControlToolbar.cpp
	ControlToolbar.h
	LoggingDialog.cpp
	LoggingDialog.h
	MainApp.cpp
	MainFrame.cpp
	MainFrame.h
	ProbeToolbar.cpp
	ProbeToolbar.h
	StandardSystemPaths.cpp
	StandardSystemPaths.h
	ToolController.cpp
	ToolController.h
	Tools.cpp
	Tools.h
	WxHelpers.cpp
	WxHelpers.h
)

set  (UI_CONTROLS_SOURCES
	UIControls/ExponentialSliderCore.cpp
	UIControls/ExponentialSliderCore.h
	UIControls/FixedTickSliderCore.cpp
	UIControls/FixedTickSliderCore.h
	UIControls/ISliderCore.h
	UIControls/LinearSliderCore.cpp
	UIControls/LinearSliderCore.h
	UIControls/ScalarTimeSeriesProbeControl.cpp
	UIControls/ScalarTimeSeriesProbeControl.h
	UIControls/SliderControl.h
)

source_group(" " FILES ${SPRING_LAB_SOURCES})
source_group("UIControls" FILES ${UI_CONTROLS_SOURCES})

set(RESOURCE_FILES "")
if (MSVC)
	list(APPEND RESOURCE_FILES
		Resources/SpringLab.rc
		Resources/SLabAAA.ico
		Resources/SLabBBB.ico)
else (MSVC)
	list(APPEND RESOURCE_FILES
		Resources/SLabAAA.xpm
		Resources/SLabBBB.xpm)
endif (MSVC)

source_group("Resources" FILES ${RESOURCE_FILES})

add_executable (SpringLab ${SPRING_LAB_SOURCES} ${UI_CONTROLS_SOURCES} ${RESOURCE_FILES})

target_include_directories(SpringLab PRIVATE .)
target_include_directories(SpringLab PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(SpringLab PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (SpringLab
	SLabCoreLib
#	${OPENGL_LIBRARIES}
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})


if (MSVC)
	set_target_properties(SpringLab PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /NODEFAULTLIB:MSVCRTD")
endif (MSVC)


#
# VS properties
#

if (MSVC)
	
	set_target_properties(
		SpringLab
		PROPERTIES
			# Set debugger working directory to binary output directory
			VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"

			# Set output directory to binary output directory - VS will add the configuration type
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)

endif (MSVC)



#
# Copy files
#

message (STATUS "Copying data files and runtime files...")

file(COPY "${CMAKE_SOURCE_DIR}/Data" "${CMAKE_SOURCE_DIR}/Objects"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
file(COPY "${CMAKE_SOURCE_DIR}/Data" "${CMAKE_SOURCE_DIR}/Objects"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
file(COPY "${CMAKE_SOURCE_DIR}/Data" "${CMAKE_SOURCE_DIR}/Objects"
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")

if (WIN32)
	file(COPY ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
	file(COPY ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
	file(COPY ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
endif (WIN32)


####################################################
# Install
####################################################

# Main binary
install(TARGETS SpringLab
	CONFIGURATIONS Release
        RUNTIME
	DESTINATION .)

# Runtime libraries
if (WIN32)
	# Devil
	install(FILES ${DEVIL_RUNTIME_LIBRARIES}
		DESTINATION .)
endif (WIN32)

# Resources
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Data"
	DESTINATION .)

# License et al
install(FILES "${CMAKE_SOURCE_DIR}/license.txt" "${CMAKE_SOURCE_DIR}/changes.txt" "${CMAKE_SOURCE_DIR}/README.md"
	DESTINATION .)
