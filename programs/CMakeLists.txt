add_executable(do_firmware do_firmware.c)
target_link_libraries(do_firmware nlutils)

# Don't install do_firmware in the Palace VM (TODO: Build a separate dpkg)
if(NOT NL_PACKAGE)
	install(TARGETS do_firmware
		RUNTIME DESTINATION bin)
endif(NOT NL_PACKAGE)
