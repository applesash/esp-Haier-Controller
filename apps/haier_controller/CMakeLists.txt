cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../common/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(haier_controller)

set(HAIER_PREVIEW_STORAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/preview_storage")
file(MAKE_DIRECTORY "${HAIER_PREVIEW_STORAGE_DIR}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/../../preview/"
	 DESTINATION "${HAIER_PREVIEW_STORAGE_DIR}")
configure_file(
	"${CMAKE_CURRENT_LIST_DIR}/../../common/dashboard/dashboard_model.json"
	"${HAIER_PREVIEW_STORAGE_DIR}/dashboard_model.json"
	COPYONLY)
spiffs_create_partition_image(storage "${HAIER_PREVIEW_STORAGE_DIR}" FLASH_IN_PROJECT)
