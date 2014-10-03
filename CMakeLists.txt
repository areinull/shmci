CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT(shmci C)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99 -D_GNU_SOURCE")

ADD_LIBRARY(shmci SHARED shmci.c)
ADD_LIBRARY(shmsbuf SHARED shmsbuf.c)

ADD_EXECUTABLE(shmci_test1 test1.c)
ADD_EXECUTABLE(shmci_test2_0 test2_0.c)
ADD_EXECUTABLE(shmci_test2_1 test2_1.c)
ADD_EXECUTABLE(shmci_test3_0 test3_0.c)
ADD_EXECUTABLE(shmci_test3_1 test3_1.c)
add_executable(shmci_test_sbuf test_sbuf.c)
add_executable(shmci_test_sbuf_1 test_sbuf_1.c)
add_executable(shmci_test_sbuf_2 test_sbuf_2.c)

TARGET_LINK_LIBRARIES(shmci_test1 shmci)
TARGET_LINK_LIBRARIES(shmci_test2_0 shmci)
TARGET_LINK_LIBRARIES(shmci_test2_1 shmci)
TARGET_LINK_LIBRARIES(shmci_test3_0 shmci)
TARGET_LINK_LIBRARIES(shmci_test3_1 shmci)
target_link_libraries(shmci_test_sbuf vu_tools shmsbuf)
target_link_libraries(shmci_test_sbuf_1 vu_tools shmsbuf)
target_link_libraries(shmci_test_sbuf_2 shmsbuf pthread)

# libdir
IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX "")
ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
SET(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
MESSAGE(STATUS "Library install dir: ${LIB_INSTALL_DIR}")

SET(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
MESSAGE(STATUS "Headers install dir: ${INCLUDE_INSTALL_DIR}")

#Find swig
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

#find PythonLibs
FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SWIG_ADD_MODULE(ShmBind python shmbind.i)
SWIG_LINK_LIBRARIES(ShmBind ${PYTHON_LIBRARIES} shmci shmsbuf)

## configure install
FIND_PACKAGE(PythonInterp REQUIRED)
EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "from sys import stdout; from distutils import sysconfig; stdout.write(sysconfig.get_python_lib())" OUTPUT_VARIABLE PYTHON_LIB_PATH)
MESSAGE(STATUS "Python lib path: ${PYTHON_LIB_PATH}")

# install

INSTALL(TARGETS shmci shmsbuf LIBRARY DESTINATION ${LIB_INSTALL_DIR})
INSTALL(FILES shmci.h shm_addr.h shmsbuf.h DESTINATION ${INCLUDE_INSTALL_DIR}/shmci)

INSTALL(TARGETS _ShmBind LIBRARY DESTINATION ${PYTHON_LIB_PATH})
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ShmBind.py
        DESTINATION ${PYTHON_LIB_PATH})

# uninstall

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(shmci_uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


