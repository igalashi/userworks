#_______________________________________________________________________________
set(EXEC TrgDaqProxy)

add_executable(${EXEC}
  ${EXEC}.cxx;
)

target_include_directories(${EXEC} PRIVATE
  ${Boost_INCLUDE_DIRS};
  ${FairMQ_INCDIR};
  ${FairMQ_INCDIR}/fairmq;
  ${FairMQ_INCDIR}/fairlogger/bundled;
  ${CMAKE_SOURCE_DIR};
  )

target_link_directories(${EXEC} PRIVATE
  ${Boost_LIBRARY_DIRS};
  ${FairMQ_LIBDIR};
)

target_link_libraries(${EXEC}
  ${CMAKE_THREAD_LIBS_INIT};
  ${Boost_LIBRARIES}; 
  FairMQ;
  FairMQStateMachine;
  FairLogger;
  stdc++fs;
  TrgDaqUnpackerRun0b;
  TrgDaqUnpackerRun0c;
)

install(TARGETS
  ${EXEC};
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

