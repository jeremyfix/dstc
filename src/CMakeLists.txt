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
set_target_properties(statistics PROPERTIES COMPILE_FLAGS "${PROJECT_ALL_CFLAGS}" LINK_FLAGS "${PROJECT_ALL_LDFLAGS} -L${CMAKE_BINARY_DIR}/src -ldstcjson")
add_dependencies(statistics dstcjson)	
 
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/statistics
            DESTINATION bin
            RENAME ${CMAKE_PROJECT_NAME}-statistics)

######################################################
# Define the stuff for the viewer
IF(${DSTC_Viewer} MATCHES "ON")
  set(qobject_headers labelviewer.h mainwindow.h ontologyviewer.h trackeroutputviewer.h)
  QT4_WRAP_CPP(qobject_headers_MOC ${qobject_headers})
  set(qt_ui_files labelviewer.ui mainwindow.ui ontology_viewer.ui trackeroutputviewer.ui)
  QT4_WRAP_UI(qt_UI ${qt_ui_files})
  add_executable (viewer labelviewer.cpp main_viewer.cpp mainwindow.cpp ontologyviewer.cpp  trackeroutputviewer.cpp ${qobject_headers_MOC} ${qt_UI})
  set_target_properties(viewer PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(viewer PROPERTIES COMPILE_FLAGS "${PROJECT_ALL_CFLAGS}" LINK_FLAGS "${PROJECT_ALL_LDFLAGS} -L${CMAKE_BINARY_DIR}/src -ldstcjson")
  target_link_libraries(viewer ${QT_LIBRARIES})
  add_dependencies(viewer dstcjson)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/viewer
    DESTINATION bin
    RENAME ${CMAKE_PROJECT_NAME}-viewer)

ENDIF()

######################################################
# Define the stuff for the tracker
set(tracker_cc_files belief.cc converter.cc goal.cc info.cc measures.cc tools.cc user_acts.cc)
set(tracker_h_files belief-tracker.h belief.h converter.h goal.h info.h measures.h tools.h user_acts.h)


add_library(
  belief-tracker
  SHARED
  ${tracker_cc_files}
)
add_dependencies(belief-tracker dstcjson)
set_target_properties(belief-tracker PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(belief-tracker PROPERTIES COMPILE_FLAGS "${PROJECT_ALL_CFLAGS}" LINK_FLAGS "${PROJECT_ALL_LDFLAGS} -L${CMAKE_BINARY_DIR}/src -ldstcjson")

install (TARGETS belief-tracker
	 DESTINATION ${LIB_INSTALL_DIR})


install (TARGETS belief-tracker
	 DESTINATION ${LIB_INSTALL_DIR})

install (FILES ${tracker_h_files}
	 DESTINATION ${INCLUDE_INSTALL_DIR})