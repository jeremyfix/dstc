
CMAKE_MINIMUM_REQUIRED( VERSION 2.8.4 )

project (dstc) 

SET(PACKAGE_VERSION "1.00")
SET(PACKAGE_LICENSE "GPL")

SET(CMAKE_BUILD_TYPE Release)

SET(PROJECT_DESCRIPTION_SUMMARY "C++ tools for the belief tracker problem on the DSTC data")

#######################################
# Get the libs/inc for the dependencies
#######################################

find_package(PkgConfig)

pkg_check_modules(JSONCPP jsoncpp REQUIRED)

set(PKG_CONFIG_DEPENDS "jsoncpp")

#######################################
# Get the libs/inc for the viewer
#######################################
set(DESIRED_QT_VERSION 4)
find_package(Qt REQUIRED)
include_directories(${QT_INCLUDE_DIRS} ${QT_INCLUDE_DIR})
SET(QT_USE_QTOPENGL TRUE)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

#######################################
# Setting the compilation flags
#######################################

# cflags added by the package
SET(PROJECT_CFLAGS "-Wall -std=c++11")

# cflags added by the pkg-config dependencies contains ';' as separator. This is a fix.
string(REPLACE ";" " " JSONCPP_CFLAGS "${JSONCPP_CFLAGS}")

# lib flags added by the package
SET(PROJECT_LIBS "-Wl,--no-as-needed -ldstcjson") #-lbelief-tracker, 

# libs added by the pkg-config dependencies contains ';' as separator. This is a fix.
string(REPLACE ";" " " JSONCPP_LDFLAGS "${JSONCPP_LDFLAGS}")

# ldflags required, but not provided by pkg-config
SET(PROJECT_LDFLAGS "-lboost_system -lboost_filesystem")

# Gathering of all flags
# (e.g. for compiling examples)
SET(PROJECT_ALL_CFLAGS  "${PROJECT_CFLAGS} ${JSONCPP_CFLAGS}")
SET(PROJECT_ALL_LDFLAGS "${PROJECT_LIBS} ${PROJECT_LDFLAGS} -L${CMAKE_BINARY_DIR}/src ${JSONCPP_LDFLAGS}")

# Gathering of all flags from dependancies flags
# (e.g. for compiling libraries)
SET(PROJECT_ALL_DEP_LDFLAGS "${PROJECT_LDFLAGS} ${JSONCPP_LDFLAGS}")

###################################
# Some prefix definitions
###################################

# Define the prefix where libs are going to be installed
SET(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Library directory name")

# Define the path to install the headers
SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include/${CMAKE_PROJECT_NAME}/" CACHE PATH "The subdirectory to the header prefix")

# Define the path to install the documentation
SET(DOC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/doc/${CMAKE_PROJECT_NAME}" CACHE PATH "The subdirectory to the documentation prefix")

# Define the path to install the pc pkg-config file
SET(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig/" CACHE STRING "Base directory for pkgconfig files")

###################################
# Pkg-config stuff
###################################

FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc
"
Name: ${CMAKE_PROJECT_NAME}
Description: ${PROJECT_DESCRIPTION_SUMMARY}
Version: ${PACKAGE_VERSION}
Requires: ${PKG_CONFIG_DEPENDS}
Libs: -L${LIB_INSTALL_DIR} ${PROJECT_LDFLAGS} ${PROJECT_LIBS}
Cflags: -I${INCLUDE_INSTALL_DIR} ${PROJECT_CFLAGS}
"
)


INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.pc
    	DESTINATION ${PKGCONFIG_INSTALL_DIR}
	COMPONENT devel)

###################################
#  Subdirectories
###################################
add_subdirectory(src)
#add_subdirectory(examples)
#add_subdirectory(doc)


###################################
#  Custom command for source tgz
###################################

SET(DIST_DIR "${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION}")
ADD_CUSTOM_TARGET(dist 
		  COMMAND rm -rf ${DIST_DIR}
		  COMMAND mkdir  ${DIST_DIR}
		  COMMAND cp -r ${CMAKE_SOURCE_DIR}/* ${DIST_DIR} || true 
		  COMMAND rm -rf ${DIST_DIR}/build
		  COMMAND mkdir ${DIST_DIR}/build
  		  COMMAND tar --exclude="*~" --exclude="._*" --exclude=".svn" -zcvf ${DIST_DIR}.tar.gz ${DIST_DIR}
		  COMMAND rm -rf  ${DIST_DIR}
    	          WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

###################################
# For building packages
###################################

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION_SUMMARY})
SET(CPACK_PACKAGE_VENDOR "Jeremy Fix")
SET(CPACK_PACKAGE_CONTACT "Jeremy.Fix@centralesupelec.fr")
SET(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
SET(CPACK_PACKAGE_LICENSE "${PACKAGE_LICENSE}")
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
SET(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
SET(CPACK_RESOURCE_FILE_WELCOME ${CMAKE_SOURCE_DIR}/README.md)
SET(CPACK_RESOURCE_FILE_INSTALL ${CMAKE_SOURCE_DIR}/INSTALL)
