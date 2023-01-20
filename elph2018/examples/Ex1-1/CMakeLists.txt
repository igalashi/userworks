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
add_executable(ex-1-1-sampler  runSampler.cxx Sampler.cxx )
target_link_libraries(ex-1-1-sampler  PRIVATE  FairMQ  FairLogger ${Boost_LIBRARIES} Reporter)


add_executable(ex-1-1-sink runSink.cxx Sink.cxx)
target_link_libraries(ex-1-1-sink  PRIVATE  FairMQ   FairLogger  ${Boost_LIBRARIES} Reporter)

#_______________________________________________________________________________
# install
install(TARGETS
  ex-1-1-sampler
  ex-1-1-sink
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}/examples
)

#_______________________________________________________________________________
# start script
set(EX_LIB_DIR    ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR})
set(EX_BIN_DIR    ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR}/examples)
set(EX_GUIBIN_DIR ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_BINDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-ex-1-1.sh.in          ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1.sh)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-ex-1-1-bg.sh.in       ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-bg.sh)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-ex-1-1-json.sh.in     ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-json.sh @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-ex-1-1-cmd.sh.in      ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-cmd.sh)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start-ex-1-1-cmd-json.sh.in ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-cmd-json.sh @ONLY)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ex-1-1.json  DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)

install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1.sh          DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1.sh          DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-bg.sh       DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-json.sh     DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-cmd.sh      DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)
install(PROGRAMS   ${CMAKE_CURRENT_BINARY_DIR}/start-ex-1-1-cmd-json.sh DESTINATION ${PROJECT_INSTALL_BINDIR}/examples)


