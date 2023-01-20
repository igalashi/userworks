set(EXEC drs4qdcDqm)
add_executable(${EXEC}
  ${CMAKE_SOURCE_DIR}/mq/Monitor/Dqm.cxx;
  ${EXEC}.cxx;
)

target_include_directories(${EXEC} PRIVATE
  ${Boost_INCLUDE_DIRS};
  ${ROOT_INCLUDE_DIRS};
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
  ${Boost_LIBRARIES};
  ${ROOT_LIBRARIES};
  FairMQ;
  FairLogger;
  ${CMAKE_THREAD_LIBS_INIT};
  DRS4QDCUnpacker;
)

install(TARGETS
  ${EXEC};
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)