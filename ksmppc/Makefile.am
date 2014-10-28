AUTOMAKE_OPTIONS    = subdir-objects
ACLOCAL_AMFLAGS     = -I m4
EXTRA_DIST          = bootstrap
AM_CPPFLAGS         = $(DEPS_CFLAGS) $(BOOST_CFLAGS) $(KISSCPP_CFLAGS) $(SMPPPDU_CFLAGS)
ksmppc_LDADD        = $(DEPS_LIBS) $(BOOST_LIBS) $(PTHREAD_LIB) $(KISSCPP_LIB) $(SMPP_PDU_LIB)
bin_PROGRAMS        = ksmppc
ksmppc_SOURCES      = src/bind_type.hpp \
                      src/cfg.hpp \
                      src/handler_send.cpp \
                      src/handler_send.hpp \
                      src/ksmppc.cpp \
                      src/ksmppc.hpp \
                      src/main.cpp \
                      src/rawpdu.hpp \
                      src/session_manager.cpp \
                      src/session_manager.hpp \
                      src/sharedsmpppdu.hpp \
                      src/smpppdu_queue.hpp \
                      src/smpp_session_config.hpp \
                      src/transmit_queue.hpp \
                      src/util.hpp \
                      src/util.cpp
dist_noinst_SCRIPTS = autogen.sh

