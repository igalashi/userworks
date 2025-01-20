#==============================================================================

set(INSTALL_BINDIR ${CMAKE_SOURCE_DIR}/bin)
set(INSTALL_LIBDIR ${CMAKE_SOURCE_DIR}/lib)


# ---------- add uhbook dependency ---------
find_path(UHBOOK_HEADER uhbook.cxx ../../uhbook/)
message(STATUS "UHBOOK_HEADER:    ${UHBOOK_HEADER}")


foreach(lvar IN ITEMS
		#rawdump;
		#RecbeSampler;
		#RecbeTimeFrameBuilder;
		#RecbeDisplay;
		#RecbeSink;
		#DummySampler;
		#RaiRawSampler;
		hdtbldisplay
)
  set(EXEC ${lvar})
  add_executable(${EXEC} 
    ${EXEC}.cxx;
  )

  target_include_directories(${EXEC} PUBLIC 
    ${Boost_INCLUDE_DIRS};  
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    ${FairLogger_INCDIR};
    ${CMAKE_SOURCE_DIR};
    ${ROOT_INCLUDE_DIRS}
    ${HIREDIS_HEADER};
    ${REDIS_PLUS_PLUS_HEADER};
    ${UHBOOK_HEADER};
    ./;
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
      fairmq;
      FairLogger;
      ${ROOT_LIBRARIES};
      ${HIREDIS_LIB};
      ${REDIS_PLUS_PLUS_LIB};
      pthread;
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
      pthread;
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
add_executable(hdtbldisplay
	hdtbldisplay.cxx
)

target_include_directories(hdtbldisplay PUBLIC 
    ${Boost_INCLUDE_DIRS};  
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    #${FairMQ_INCDIR}/fairlogger/bundled;
    ${FairLogger_INCDIR};
    ${CMAKE_SOURCE_DIR};
    ${ROOT_INCLUDE_DIRS}
)

target_link_directories(hdtbldisplay PUBLIC
    ${Boost_LIBRARY_DIRS}; 
    ${FairMQ_LIBDIR};
    ${ROOT_LIBRARIES};
)
]]

#[[
if ((FairMQ_VERSION VERSION_GREATER_EQUAL 1.8.0)
    AND (FairMQ_VERSION VERSION_LESS_EQUAL 1.8.9))
    target_link_libraries(hdtbldisplay PUBLIC 
      ${Boost_LIBRARIES};  
      fairmq;
      FairLogger;
      ${ROOT_LIBRARIES};
      ${HIREDIS_LIB};
      ${REDIS_PLUS_PLUS_LIB};
    )
elseif ((FairMQ_VERSION VERSION_GREATER_EQUAL 1.4.55)
    AND (FairMQ_VERSION VERSION_LESS_EQUAL 1.4.56))
    target_link_libraries(hdtbldisplay PUBLIC 
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

install(TARGETS hdtbldisplay;
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    #RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    #LIBRARY DESTINATION ${INSTALL_LIBDIR}
    RUNTIME DESTINATION ${INSTALL_BINDIR}
)
]]
