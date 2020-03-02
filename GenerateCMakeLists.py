import os, sys

# CMake path
cmakePath    = "${CMAKE_SOURCE_DIR}/Source"
cmakeBinPath = cmakePath + "/Bin"

# OS path
currentPath  = os.path.dirname(os.path.realpath(__file__))
sourcePath   = currentPath + "\Source"
binPath      = currentPath + "\Bin"
linkLibrary  = "d3d12.lib dxgi.lib d3dcompiler.lib"

# Create CMakeLists file
filePath = currentPath + "\CMakeLists.txt"
f = open(filePath, "w+")

# Min version of CMake required
f.write("CMAKE_MINIMUM_REQUIRED(VERSION 3.6)\n")
f.write("PROJECT(OpenLight)\n\n")
f.write("IF(WIN32)\n")
f.write("   SET(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS\")\n")
f.write("ENDIF(WIN32)\n\n")
f.write(f"INCLUDE_DIRECTORIES({cmakePath})\n\n")

# Loop over whole directory
sourceList     = "\n${MAIN_CPP}\n"
rootFilterName = "OpenLight"
numOfDir       = 0
tree           = os.walk(sourcePath)

# Main cpp needs to be added to project filter mannually
f.write(f"FILE(GLOB MAIN_CPP {cmakePath}/main.cpp)\n")
f.write("SOURCE_GROUP(\"%s\" FILES ${MAIN_CPP})\n\n" % rootFilterName)

# Automatically add source file folder into IDE's filter folder
for path, dirList, fileList in tree:
    for dirName in dirList:
        osDir        = os.path.join(path, dirName)
        relativePath = osDir.replace(sourcePath, "")
        cmakeDir     = cmakePath + relativePath.replace("\\", "/")
        # print(relativePath)
        # print(cmakeDir)

        # Incase duplicate folder filter name
        folderName     = dirName.upper()
        source         = f"{folderName}_SOURCE_{numOfDir}"
        sourceWithSign = "${%s}" % source
        filterName     = relativePath.replace("\\", "\\\\")
        # print(filterName)

        f.write(f"FILE(GLOB {source} {cmakeDir}/*.*)\n")
        f.write(f"SOURCE_GROUP(\"{rootFilterName}{filterName}\" FILES {sourceWithSign})\n\n")

        # executable path needs all source folder path
        sourceList += sourceWithSign + " \n"

        numOfDir = numOfDir + 1

f.write(f"SET(EXECUTABLE_OUTPUT_PATH {cmakeBinPath})\n\n")
f.write(f"ADD_EXECUTABLE(OpenLight {sourceList})\n\n")
f.write(f"TARGET_LINK_LIBRARIES(OpenLight {linkLibrary})")

f.close()

print("Generate CMakeLists succeed")