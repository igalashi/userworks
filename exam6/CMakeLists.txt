#==============================================================================


set(INSTALL_BINDIR ${CMAKE_SOURCE_DIR}/bin)

#foreach(lvar IN ITEMS Sampler; Sink; NullDevice;
foreach(lvar IN ITEMS
	tfdump; fltcoin; flttrig;
	#LogicFilter;
	TimeFrameBuilder;
	TFBFilePlayer; STFBFilePlayer;
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
    #${ROOT_INCLUDE_DIRS}
  )

  target_link_directories(${EXEC} PUBLIC
    ${Boost_LIBRARY_DIRS}; 
    ${FairMQ_LIBDIR};
    #${ROOT_LIBRARIES};
  )

  if ((FairMQ_VERSION VERSION_GREATER_EQUAL 1.8.0)
    AND (FairMQ_VERSION VERSION_LESS_EQUAL 1.8.9))
    target_link_libraries(${EXEC} PUBLIC 
      ${Boost_LIBRARIES};  
      #FairMQ;
      #FairMQStateMachine;
      fairmq;
      FairLogger;
      #${ROOT_LIBRARIES};
    )
    #message("FairMQ version ${FairMQ_VERSION}")
  elseif ((FairMQ_VERSION VERSION_GREATER_EQUAL 1.4.55)
    AND (FairMQ_VERSION VERSION_LESS_EQUAL 1.4.56))
    target_link_libraries(${EXEC} PUBLIC 
      ${Boost_LIBRARIES};  
      FairMQ;
      FairMQStateMachine;
      FairLogger;
    )
    #message("FairMQ version ${FairMQ_VERSION}")
  else()
    message(FATAL_ERROR "Unsupported FairMQ version ${FairMQ_VERSION}")
  endif()


  install(TARGETS
    ${EXEC};
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    #RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DESTINATION ${INSTALL_BINDIR}
  )
endforeach()


foreach(lvar IN ITEMS
	onlinedisplay;
	#	TriggerView;
)

set(EXEC ${lvar})
  add_executable(${EXEC} 
  ${EXEC}.cxx;
)

#add_executable(onlinedisplay
#	onlinedisplay.cxx
#)

target_include_directories(${EXEC} PUBLIC 
    ${Boost_INCLUDE_DIRS};  
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    ${FairMQ_INCDIR}/fairlogger/bundled;
    ${CMAKE_SOURCE_DIR};
    ${ROOT_INCLUDE_DIRS};
    ${HIREDIS_HEADER};
    ${REDIS_PLUS_PLUS_HEADER};
)

target_link_directories(${EXEC} PUBLIC
    ${Boost_LIBRARY_DIRS}; 
    ${FairMQ_LIBDIR};
    ${ROOT_LIBRARIES};
)

if ((FairMQ_VERSION VERSION_GREATER_EQUAL 1.8.0)
    AND (FairMQ_VERSION VERSION_LESS_EQUAL 1.8.9))
    target_link_libraries(${EXEC} PUBLIC 
        ${Boost_LIBRARIES};  
        #FairMQ;
        #FairMQStateMachine;
        fairmq;
        FairLogger;
        ${ROOT_LIBRARIES};
        ${HIREDIS_LIB};
        ${REDIS_PLUS_PLUS_LIB};
    )
elseif ((FairMQ_VERSION VERSION_GREATER_EQUAL 1.4.55)
    AND (FairMQ_VERSION VERSION_LESS_EQUAL 1.4.56))
    target_link_libraries(${EXEC} PUBLIC 
        ${Boost_LIBRARIES};  
        FairMQ;
        FairMQStateMachine;
        FairLogger;
        ${ROOT_LIBRARIES};
        ${HIREDIS_LIB};
        ${REDIS_PLUS_PLUS_LIB};
    )
else()
    message(FATAL_ERROR "Unsupported FairMQ version ${FairMQ_VERSION}")
endif()


install(TARGETS ${EXEC};
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    #RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DESTINATION ${INSTALL_BINDIR}
)

endforeach()


add_executable(chkfile
	chkfile.cxx
	TimeFrameBuilder.h
	SubTimeFrameHeader.h
	FilterHeader.h
	FileSinkHeader.h
)

target_compile_options(chkfile
	PRIVATE -Wall -O -g
)

target_link_libraries(chkfile
	stdc++fs
)

add_executable(LogiCalc
	LogiCalc.cxx
)

target_compile_options(LogiCalc
	PRIVATE -Wall -O -g -D LOGICALC_TEST_MAIN
)

add_executable(TriggerMap
	TriggerMap.cxx
)

target_compile_options(TriggerMap
	PRIVATE -Wall -O -g -D TRIGGERMAP_TEST_MAIN
)

add_executable(SignalParser
	SignalParser.cxx
)

target_compile_options(SignalParser
	PRIVATE -Wall -O -g -D SIGNALPARSER_TEST_MAIN
)
