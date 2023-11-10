if (UNIX)
  add_definitions(-Wno-switch)
endif()

add_executable(blackjackclient
  BlackJackClient.cpp
  )

target_link_libraries(blackjackclient cpprest)
