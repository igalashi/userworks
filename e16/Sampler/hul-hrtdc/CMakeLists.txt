set(EXEC hul_hrtdcSampler)
add_executable(${EXEC}
  ${CMAKE_SOURCE_DIR}/mq/Sampler/Sampler.cxx;  
  ${EXEC}.cxx;
)

target_include_directories(${EXEC} PRIVATE
  ${Boost_INCLUDE_DIRS};
  ${FairMQ_INCDIR};
  ${FairMQ_INCDIR}/fairmq;
  ${FairMQ_INCDIR}/fairlogger/bundled;
  ${CMAKE_SOURCE_DIR};
  ${CMAKE_BINARY_DIR};
)

target_link_directories(${EXEC} PRIVATE
  ${Boost_LIBRARY_DIRS};
  ${FairMQ_LIBDIR};
)

target_link_libraries(${EXEC}
  HULHRTDCDaq;
  FairMQ;
  FairMQStateMachine;
  FairLogger;
  ${Boost_LIBRARIES}; 
  ${CMAKE_THREAD_LIBS_INIT};
  stdc++fs;
  )

install(TARGETS
  ${EXEC};
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
