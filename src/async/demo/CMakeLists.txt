set(CPPPROGS AsyncAudioIO_demo AsyncDnsLookup_demo AsyncFdWatch_demo
             AsyncTimer_demo AsyncTcpClient_demo AsyncUdpSocket_demo
             AsyncCppApplication_demo AsyncTcpServer_demo AsyncConfig_demo
             AsyncSerial_demo AsyncAtTimer_demo AsyncExec_demo
             AsyncPtyStreamBuf_demo AsyncMsg_demo AsyncFramedTcpServer_demo
             AsyncFramedTcpClient_demo AsyncAudioSelector_demo
             AsyncAudioFsf_demo AsyncHttpServer_demo AsyncFactory_demo
             AsyncAudioContainer_demo AsyncTcpPrioClient_demo
             AsyncStateMachine_demo AsyncPlugin_demo
             )


if(LADSPA_FOUND)
  set(CPPPROGS ${CPPPROGS} AsyncAudioLADSPAPlugin_demo)
endif(LADSPA_FOUND)


# Build all demo applications
foreach(prog ${CPPPROGS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} ${LIBS} asynccpp asyncaudio asynccore)
endforeach(prog)

# Build the demo plugin used by the AsyncPlugin_demo application
add_library(DemoPlugin MODULE DemoPlugin.cpp)
set_target_properties(DemoPlugin PROPERTIES PREFIX "")
set_property(TARGET DemoPlugin PROPERTY NO_SONAME 1)
#target_link_libraries(DemoPlugin ${LIBS})

