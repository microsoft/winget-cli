if (NOT WINDOWS_STORE AND NOT WINDOWS_PHONE)
  add_subdirectory(BlackJack_Server)
  add_subdirectory(BlackJack_Client)
else()
  # TODO: add BlackJack_UIClient
endif()
