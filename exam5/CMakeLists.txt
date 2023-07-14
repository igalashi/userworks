#==============================================================================
#foreach(lvar IN ITEMS Sampler; Sink; NullDevice;
foreach(lvar IN ITEMS
	tfdump; fltcoin; TimeFrameBuilder;
	TFBFilePlayer; STFBFilePlayer
)
  set(EXEC ${lvar})
  add_executable(${EXEC} 
    ${EXEC}.cxx;
  )

  target_include_directories(${EXEC} PUBLIC 
    ${Boost_INCLUDE_DIRS};  
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    #${FairMQ_INCDIR}/fairlogger/bundled;
    ${CMAKE_SOURCE_DIR};
    #${ROOT_INCLUDE_DIRS}
  )

  target_link_directories(${EXEC} PUBLIC
    ${Boost_LIBRARY_DIRS}; 
    ${FairMQ_LIBDIR};
    #${ROOT_LIBRARIES};
  )

  target_link_libraries(${EXEC} PUBLIC 
    ${Boost_LIBRARIES};  
    ${fmt_LIB};
    FairMQ;
    FairMQStateMachine;
    FairLogger;
    #${ROOT_LIBRARIES};
  )

  install(TARGETS
    ${EXEC};
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endforeach()

add_executable(onlinedisplay
	onlinedisplay.cxx
)

target_include_directories(onlinedisplay PUBLIC 
    ${Boost_INCLUDE_DIRS};  
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    #${FairMQ_INCDIR}/fairlogger/bundled;
    ${CMAKE_SOURCE_DIR};
    ${ROOT_INCLUDE_DIRS}
)

target_link_directories(onlinedisplay PUBLIC
    ${Boost_LIBRARY_DIRS}; 
    ${FairMQ_LIBDIR};
    ${ROOT_LIBRARIES};
)

target_link_libraries(onlinedisplay PUBLIC 
    ${Boost_LIBRARIES};  
    ${fmt_LIB};
    FairMQ;
    FairMQStateMachine;
    FairLogger;
    ${ROOT_LIBRARIES};
)

install(TARGETS onlinedisplay;
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(chkfile
	chkfile.cxx
	TimeFrameBuilder.h
	SubTimeFrameHeader.h
	FilterHeader.h
	FileSinkHeader.h)
target_compile_options(chkfile
	PRIVATE -Wall -O -g)
target_link_libraries(chkfile
	stdc++fs
)

add_executable(LogiCalc
	LogiCalc.cxx)
target_compile_options(LogiCalc
	PRIVATE -Wall -O -g -D LOGICALC_TEST_MAIN)

add_executable(TriggerMap
	TriggerMap.cxx)
target_compile_options(TriggerMap
	PRIVATE -Wall -O -g -D TRIGGERMAP_TEST_MAIN)
