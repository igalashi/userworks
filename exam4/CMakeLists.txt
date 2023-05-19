#==============================================================================
#foreach(lvar IN ITEMS Sampler; Sink; NullDevice;
foreach(lvar IN ITEMS
	tfdump; fltcoin; TFBFilePlayer; STFBFilePlayer
)
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
    ${fmt_LIB};
    FairMQ;
    FairMQStateMachine;
    FairLogger;
  )

  install(TARGETS
    ${EXEC};
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endforeach()
