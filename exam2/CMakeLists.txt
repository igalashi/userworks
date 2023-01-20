#==============================================================================
#foreach(lvar IN ITEMS Sampler; Sink; NullDevice;
foreach(lvar IN ITEMS
#	dummySampler; Worker; dummySink;
	dtdcSampler; dtdcWorker; dtdcSink)
  set(EXEC ${lvar})
  add_executable(${EXEC} 
    ${EXEC}.cxx;
  )

  target_include_directories(${EXEC} PUBLIC 
    ${Boost_INCLUDE_DIRS};  
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    ${FairMQ_INCDIR}/fairlogger/bundled;
    ${CMAKE_SOURCE_DIR};
  )

  target_link_directories(${EXEC} PUBLIC
    ${Boost_LIBRARY_DIRS}; 
    ${FairMQ_LIBDIR};
  )

  target_link_libraries(${EXEC} PUBLIC 
    ${Boost_LIBRARIES};  
    FairMQ;
    FairLogger;
  )

  install(TARGETS
    ${EXEC};
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endforeach()
