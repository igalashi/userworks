foreach(lvar IN ITEMS TagBroadcaster; TagSubscriber;)
  add_executable(${lvar}
    ${lvar}.cxx;
  )

  target_include_directories(${lvar} PRIVATE
    ${Boost_INCLUDE_DIRS};
    ${FairMQ_INCDIR};
    ${FairMQ_INCDIR}/fairmq;
    ${FairMQ_INCDIR}/fairlogger/bundled;
    ${CMAKE_SOURCE_DIR};
  )

  target_link_directories(${lvar} PRIVATE
    ${Boost_LIBRARY_DIRS};
    ${FairMQ_LIBDIR};
  )

  target_link_libraries(${lvar}
    FairMQ;
    FairMQStateMachine;
    FairLogger;
    ${Boost_LIBRARIES};
    ${CMAKE_THREAD_INIT};
  )

  install(TARGETS
    ${lvar};
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endforeach()

add_subdirectory(drs4qdc)
add_subdirectory(srs-atca)
add_subdirectory(trg)
add_subdirectory(hul-hrtdc)
add_subdirectory(gbtxemu)
