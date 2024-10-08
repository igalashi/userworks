#==============================================================================

set(INSTALL_BINDIR ${CMAKE_SOURCE_DIR}/bin)
set(INSTALL_LIBDIR ${CMAKE_SOURCE_DIR}/lib)

#foreach(lvar IN ITEMS Sampler; Sink; NullDevice;
foreach(lvar IN ITEMS
	rawdump;
	RecbeSampler;
	RecbeTimeFrameBuilder;
	RecbeDisplay;
	RecbeSink;
	DummySampler;
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
    ${FairLogger_INCDIR};
    ${CMAKE_SOURCE_DIR};
    #${ROOT_INCLUDE_DIRS}
    ${HIREDIS_HEADER};
    ${REDIS_PLUS_PLUS_HEADER};
    ./;
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
      fairmq;
      FairLogger;
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
      ${HIREDIS_LIB};
      ${REDIS_PLUS_PLUS_LIB};
    )
  else()
    message(FATAL_ERROR "Unsupported FairMQ version ${FairMQ_VERSION}")
  endif()

  install(TARGETS
    ${EXEC};
    #LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    #RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${INSTALL_LIBDIR}
    RUNTIME DESTINATION ${INSTALL_BINDIR}
  )
endforeach()


#[[
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
    #${fmt_LIB};
    FairMQ;
    FairMQStateMachine;
    FairLogger;
    ${ROOT_LIBRARIES};
)

install(TARGETS onlinedisplay;
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
]]


add_executable(CliSock
	CliSock.cxx
	)
target_compile_options(CliSock
	PRIVATE -Wall -O -g -D TEST_MAIN_CLISOCK)

#target_link_libraries(Clisock
#	stdc++fs)


add_executable(drecbe
	drecbe.cxx
	recbe.h
	)
target_compile_options(drecbe
	PRIVATE -Wall -O -g)
target_include_directories(drecbe PUBLIC 
    ${FairMQ_INCDIR};
    ./kol;
)
target_link_directories(drecbe PUBLIC
    ${FairMQ_LIBDIR};
    ./kol;
)
target_link_libraries(drecbe PUBLIC 
    zmq;
    kol;
)

add_subdirectory(kol)
