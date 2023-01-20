#_______________________________________________________________________________
# header for build
include_directories(
  ${Boost_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIRS}
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
add_executable(ExMonitor runExMonitor.cxx  ExMonitor.cxx)
target_link_libraries(ExMonitor PRIVATE  FairMQ FairLogger ${Boost_LIBRARIES} ${ROOT_LIBRARIES} RHTTP Reporter)

#_______________________________________________________________________________
# install
install(TARGETS
  ExMonitor
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}/examples
)

#_______________________________________________________________________________
# start script
set(EX_BIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/examples)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/startExMonitor.sh.in ${CMAKE_CURRENT_BINARY_DIR}/startExMonitor.sh @ONLY)

install(FILES      ${CMAKE_CURRENT_SOURCE_DIR}/ExMonitor.json     DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/startExMonitor.sh  DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
