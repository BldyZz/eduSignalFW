#***********************************************
#	Main Project						       *
#***********************************************

# Add all source files
FILE(GLOB_RECURSE app_sources 
	${CMAKE_SOURCE_DIR}/src/*.*
	${CMAKE_SOURCE_DIR}/src/*/*.*
)

# Register project source files
idf_component_register(SRCS ${app_sources} REQUIRES wpa_supplicant)

