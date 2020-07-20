SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../Bin)

SET(CORE_DIR Core)
ADD_SUBDIRECTORY(${CORE_DIR})
link_directories(Lib)

FILE(GLOB CORE_SOURCE ${CORE_DIR}/*.*)
FILE(GLOB RESOURCE_SOURCE ${CORE_DIR}/Resource/*.*)
FILE(GLOB MATERIAL_SOURCE ${CORE_DIR}/Material/*.*)
FILE(GLOB GUI_SOURCE ${CORE_DIR}/GUI/*.*)
FILE(GLOB STRUCTURE_SOURCE ${CORE_DIR}/Structure/*.*)
FILE(GLOB RENDERGRAPH_SOURCE ${CORE_DIR}/RenderGraph/*.*)
FILE(GLOB VEGETATION_SOURCE ${CORE_DIR}/VegetationDemo/*.*)
FILE(GLOB DEMO_SOURCE ${CORE_DIR}/Demo/*.*)



SET(ALL_SOURCE  ${CORE_SOURCE} ${RESOURCE_SOURCE} ${MATERIAL_SOURCE} ${GUI_SOURCE}  ${STRUCTURE_SOURCE} ${VEGETATION_SOURCE} ${RENDERGRAPH_SOURCE} ${DEMO_SOURCE})

foreach(source IN LISTS ALL_SOURCE)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" source_path "${source_path}")
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

ADD_EXECUTABLE(OpenLight  ${ALL_SOURCE} main.cpp )
TARGET_LINK_LIBRARIES(OpenLight d3d12.lib dxgi.lib D3DCompiler.lib FreeImage)