#***********************************************
#	Main Project						       *
#***********************************************

# Add all source files
FILE(GLOB_RECURSE app_sources 
	${CMAKE_SOURCE_DIR}/main/main.cpp
	${CMAKE_SOURCE_DIR}/main/*/*.*
)

# Register project source files
idf_component_register(
	SRCS ${app_sources} 
	REQUIRES driver esp_wifi nvs_flash esp_timer wpa_supplicant
	INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/firmware
	)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++20 -Wunused-variable -O3)
