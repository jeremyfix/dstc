# Make sure the compiler can find include files from our library.
include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_BINARY_DIR})

######################################################
# Define our parser library
add_library(
  dstcjson
  SHARED
  JSONObjects.cc
)
  
set_target_properties(dstcjson PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(dstcjson PROPERTIES COMPILE_FLAGS "${PROJECT_ALL_CFLAGS}" LINK_FLAGS "${PROJECT_ALL_DEP_LDFLAGS}")

install (TARGETS dstcjson
	 DESTINATION ${LIB_INSTALL_DIR}
         COMPONENT binary)


install (TARGETS dstcjson
	 DESTINATION ${LIB_INSTALL_DIR}
         COMPONENT binary)

install (FILES JSONObjects.h
	 DESTINATION ${INCLUDE_INSTALL_DIR}
	 COMPONENT devel)

######################################################
# Define the binary for computing statistics on the dstc 
add_executable (statistics statistics.cc) 
set_target_properties(statistics PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(statistics PROPERTIES COMPILE_FLAGS "${PROJECT_ALL_CFLAGS}" LINK_FLAGS "${PROJECT_ALL_LDFLAGS}")
add_dependencies(statistics dstcjson)	 
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/statistics
            DESTINATION bin
            RENAME ${CMAKE_PROJECT_NAME}-statistics
	    COMPONENT binary)

######################################################
# Define the stuff for the viewer

set(qobject_headers labelviewer.h mainwindow.h ontologyviewer.h trackeroutputviewer.h)
QT4_WRAP_CPP(qobject_headers_MOC ${qobject_headers})
set(qt_ui_files labelviewer.ui mainwindow.ui ontology_viewer.ui trackeroutputviewer.ui)
QT4_WRAP_UI(qt_UI ${qt_ui_files})
add_executable (viewer labelviewer.cpp main_viewer.cpp mainwindow.cpp ontologyviewer.cpp  trackeroutputviewer.cpp ${qobject_headers_MOC} ${qt_UI})
set_target_properties(viewer PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(viewer PROPERTIES COMPILE_FLAGS "${PROJECT_ALL_CFLAGS}" LINK_FLAGS "${PROJECT_ALL_LDFLAGS}")
target_link_libraries(viewer ${QT_LIBRARIES})
add_dependencies(viewer dstcjson)