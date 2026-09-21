cmake_minimum_required(VERSION 3.14)
project(gurobiSolverCWLP)

set(CMAKE_CXX_STANDARD 14)

#------------------- CREATION DES VARIABLES POUR LA BIBLIOTHEQUE GUROBI -----------------------
message(STATUS "CMAKE_SYSTEM_NAME : ${CMAKE_SYSTEM_NAME}")
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") #Pour Linux
	set(GUROBI_ROOT "/opt/local/stow/gurobi910/linux64")  #A Modifier si votre chemin est différent
endif()	
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows") #Pour Windows
	set(GUROBI_ROOT "C:/gurobi952/win64") #A Modifier si votre chemin est différent
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin") #Pour MacOS
	set(GUROBI_ROOT "/Library/gurobi952/mac64") #A Modifier si votre chemin est différent
endif()
	
message(STATUS "GUROBI_ROOT : ${GUROBI_ROOT}")

set(GUROBI_INCLUDE_DIR "${GUROBI_ROOT}/include")
message(STATUS "GUROBI_INCLUDE_DIR : ${GUROBI_INCLUDE_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux") #Pour Linux
	set(GUROBI_LIBRARY_1 "${GUROBI_ROOT}/lib/libgurobi_c++.a")
	set(GUROBI_LIBRARY_2 "${GUROBI_ROOT}/lib/libgurobi91.so")
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows") #Pour Windows
	set(GUROBI_LIBRARY_1 "${GUROBI_ROOT}/lib/gurobi95.lib")
	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    	set(GUROBI_LIBRARY_2 "${GUROBI_ROOT}/lib/gurobi_c++mdd2019.lib")
	else()
    	set(GUROBI_LIBRARY_2 "${GUROBI_ROOT}/lib/gurobi_c++md2019.lib")
	endif()
endif()
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin") #Pour MacOS
	set(GUROBI_LIBRARY_1 "${GUROBI_ROOT}/lib/libgurobi_c++.a")
	set(GUROBI_LIBRARY_2 "${GUROBI_ROOT}/lib/libgurobi91.dylib")
endif()
set(GUROBI_LIBRARIES ${GUROBI_LIBRARY_1} ${GUROBI_LIBRARY_2})
message(STATUS "GUROBI_LIBRARIES : ${GUROBI_LIBRARIES}")

include_directories(${GUROBI_INCLUDE_DIR})
include_directories(include)

#------------------- PARTIE MODIFIÉE POUR PLUSIEURS EXECUTABLES -----------------------

add_executable(fraicheurSolver
    src/main.cpp
    src/data_loader.cpp
    src/solveurHeuristique.cpp
)

target_include_directories(fraicheurSolver PRIVATE include)
target_link_libraries(fraicheurSolver ${GUROBI_LIBRARIES})
#------------------- LIEN SYMBOLIQUE VERS DATA -----------------------
if (UNIX)  #Pour MacOS et Linux
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/data/ ${CMAKE_CURRENT_BINARY_DIR}/data)
else()  #Pour Windows
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/data" _dstDir)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/data/" _srcDir)
    execute_process(COMMAND cmd.exe /c mklink /J "${_dstDir}" "${_srcDir}")
endif()
