#_______________________________________________________________________________
# header for build
include_directories(
  ${Boost_INCLUDE_DIRS}
  ${FairMQ_INCDIR}
  ${FairMQ_INCDIR}/fairmq
  ${CMAKE_SOURCE_DIR}/e50daq
  )

#_______________________________________________________________________________
# library
link_directories(
  ${FairMQ_LIBDIR}
  ${Boost_LIBRARY_DIRS}
  ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR}
  )

#_______________________________________________________________________________
# executables
add_executable(ExFilter runExFilter.cxx  ExFilter.cxx)
target_link_libraries(ExFilter PRIVATE  FairMQ FairLogger ${Boost_LIBRARIES} Reporter)

#_______________________________________________________________________________
# install
install(TARGETS
  ExFilter
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}/examples
)

#_______________________________________________________________________________
# start script
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/examples)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/startExFilter.sh.in ${CMAKE_CURRENT_BINARY_DIR}/startExFilter.sh @ONLY)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ExFilter.json         DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS  ${CMAKE_CURRENT_BINARY_DIR}/startExFilter.sh  DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
