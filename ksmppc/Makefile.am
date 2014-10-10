AUTOMAKE_OPTIONS    = subdir-objects
ACLOCAL_AMFLAGS     = -I m4
AM_CPPFLAGS         = $(DEPS_CFLAGS) $(BOOST_CFLAGS)
ksmppcd_LDADD       = $(DEPS_LIBS) $(BOOST_LIBS) $(PTHREAD_LIB)
bin_PROGRAMS        = ksmppcd
ksmppcd_SOURCES     = src/bind_type.hpp \
                      src/config.hpp \
                      src/handler_send.cpp \
                      src/handler_send.hpp \
                      src/ksmppc.cpp \
                      src/ksmppc.hpp \
                      src/main.cpp \
                      src/session_manager.cpp \
                      src/session_manager.hpp \
                      src/sharedsmpppdu.hpp \
                      src/smpppdu_queue.hpp \
                      src/transmit_queue.hpp \
                      src/util.hpp \
                      src/util.cpp
dist_noinst_SCRIPTS = autogen.sh

