TESTS = 01-trig.test 02-accumulator.test 03-channel.test	\
        04-point_set.test 05-state.test 06-ppht.test		\
        07-postprocess.test

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/build-aux/tap-driver.sh

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CXXFLAGS = -Wall -Wpedantic
AM_DEFAULT_SOURCE_EXT = .cpp

EXTRA_DIST = tap.hpp image-01.hpp image-02.hpp image-03.hpp

check_PROGRAMS = $(TESTS)

