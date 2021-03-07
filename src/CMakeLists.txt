add_library(nlutils SHARED escape.c exec.c nlutils.c sha1.c
	str.c stream.c net.c log.c thread.c variant.c kvp.c debug.c
	url.c fifo.c hash.c url_req.c mem.c nl_time.c inline_defs.c)

find_library(LIBEVENT_CORE_LIBRARY event_core HINTS /usr/local/lib)
target_link_libraries(nlutils dl rt m ${LIBEVENT_CORE_LIBRARY})
set_target_properties(nlutils PROPERTIES VERSION ${NLUTILS_VERSION} SOVERSION ${NLUTILS_SO_VERSION})

install(TARGETS nlutils LIBRARY DESTINATION lib)
