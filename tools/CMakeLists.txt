if(NOT NL_PACKAGE)
	install(PROGRAMS mk_firmware.sh mk_sfx.sh
		DESTINATION bin)
	install(FILES pkg_helper.sh root_helper.sh build_root_helper.sh
		DESTINATION share/nlutils)
endif(NOT NL_PACKAGE)

file(COPY mk_firmware.sh mk_sfx.sh pkg_helper.sh root_helper.sh build_root_helper.sh
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
