#
find_package(ROOT)
#set(ROOT_LIBRARYS ${ROOT_LIBRARYS};/home/nestdaq/root/lib/libRHTTP.so)
#find_package(ROOT::RHTTP)

#===============================================================================
include_directories(
  ${Boost_INCLUDE_DIRS}
  ${FairMQ_INCDIR}
  ${FairMQ_INCDIR}/fairmq
  ${ROOT_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
)

#===============================================================================
link_directories(
  ${Boost_LIBRARY_DIRS}
  ${FairMQ_LIBDIR}
)

#===============================================================================
# ROOT dictionary
if(ROOT_FOUND)
  message(STATUS "ROOT found")
  message(STATUS "ROOT_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}")
  message(STATUS "ROOT_LIBRARIES ${ROOT_LIBRARIES}")

#  ROOT_GENERATE_DICTIONARY(G__stl_vector_vector 
#    LINKDEF LinkDef/stl_vector_vector_LinkDef.h)
endif()
#
#add_library(stl_vector_vector SHARED G__stl_vector_vector.cxx)
add_library(Reporter SHARED Reporter.cxx)

#===============================================================================
# executables
#-------------------------------------------------------------------------------
add_executable(BenchmarkSampler     runBenchmarkSampler.cxx    BenchmarkSampler.cxx    Reporter.cxx)
add_executable(HulStrTdcEmulator    runHulStrTdcEmulator.cxx   HulStrTdcEmulator.cxx   Reporter.cxx)
add_executable(HulStrTdcSampler     runHulStrTdcSampler.cxx    HulStrTdcSampler.cxx    Reporter.cxx FPGAModule.cc   UDPRBCP.cc   BitDump.cc)
#add_executable(FileSink             runFileSink.cxx            FileSink.cxx            Reporter.cxx)
add_executable(MQSink               runMQSink.cxx              MQSink.cxx              Reporter.cxx)
add_executable(SubTimeFrameBuilder  runSubTimeFrameBuilder.cxx SubTimeFrameBuilder.cxx Reporter.cxx)
add_executable(HulStrTdcSTFBuilder  runHulStrTdcSTFBuilder.cxx HulStrTdcSTFBuilder.cxx Reporter.cxx)
add_executable(HulStrTdcFilter      runHulStrTdcFilter.cxx     HulStrTdcFilter.cxx     Reporter.cxx)
add_executable(HulStrTdcDqm         runHulStrTdcDqm.cxx        HulStrTdcDqm.cxx        Reporter.cxx)
add_executable(TimeFrameBuilder     runTimeFrameBuilder.cxx    TimeFrameBuilder.cxx    Reporter.cxx)
add_executable(TimeStampInserter    runTimeStampInserter.cxx   TimeStampInserter.cxx   Reporter.cxx)

#-------------------------------------------------------------------------------
target_link_libraries(BenchmarkSampler     PRIVATE  FairMQ  FairMQStateMachine FairLogger ${Boost_LIBRARIES})
target_link_libraries(HulStrTdcEmulator    PRIVATE  FairMQ  FairMQStateMachine FairLogger  ${Boost_LIBRARIES})
target_link_libraries(HulStrTdcSampler     PRIVATE  FairMQ  FairLogger  ${Boost_LIBRARIES})
#target_link_libraries(FileSink             PRIVATE  FairMQ  FairLogger  ${Boost_LIBRARIES} ${ROOT_LIBRARIES} stl_vector_vector stdc++fs)
target_link_libraries(MQSink               PRIVATE  FairMQ  FairLogger  ${Boost_LIBRARIES})
target_link_libraries(SubTimeFrameBuilder  PRIVATE  FairMQ  FairMQStateMachine FairLogger  ${Boost_LIBRARIES})
target_link_libraries(HulStrTdcSTFBuilder  PRIVATE  FairMQ  FairMQStateMachine FairLogger  ${Boost_LIBRARIES})
target_link_libraries(HulStrTdcFilter      PRIVATE  FairMQ  FairMQStateMachine FairLogger  ${Boost_LIBRARIES})
#target_link_libraries(HulStrTdcDqm          PRIVATE  FairMQ  FairLogger  ${Boost_LIBRARIES} ${ROOT_LIBRARIES} RHTTP)
target_link_libraries(HulStrTdcDqm          PRIVATE  FairMQ  FairLogger  ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ROOT::RHTTP)
target_link_libraries(TimeFrameBuilder     PRIVATE  FairMQ  FairMQStateMachine FairLogger  ${Boost_LIBRARIES})
target_link_libraries(TimeStampInserter    PRIVATE  FairMQ  FairMQStateMachine FairLogger  ${Boost_LIBRARIES})

#set_target_properties(FileSink PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_LIBDIR})

#===============================================================================
# install

#install(FILES  ${CMAKE_CURRENT_BINARY_DIR}/libstl_vector_vector_rdict.pcm
#   DESTINATION ${PROJECT_INSTALL_LIBDIR}
#)

#install(TARGETS
#   stl_vector_vector
#   LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
#)

install(TARGETS
   Reporter  
   LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
)

install(TARGETS
  BenchmarkSampler
  HulStrTdcEmulator
  HulStrTdcSampler
#  FileSink
  MQSink
  SubTimeFrameBuilder
  HulStrTdcSTFBuilder
  HulStrTdcFilter
  HulStrTdcDqm
  TimeFrameBuilder
  TimeStampInserter
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)




#===============================================================================
# add start scripts
#add_subdirectory(run)

# examples
#add_subdirectory(examples)

# plugins
#add_subdirectory(plugins)
