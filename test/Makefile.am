TESTS = 01-raster.test \
        02-trig.test \
        03-accumulator.test \
        04-channel.test \
        05-point_set.test \
        06-state.test \
        07-ppht.test \
        08-postprocess.test

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/build-aux/tap-driver.sh

AM_CPPFLAGS = -I$(abs_top_srcdir) -I$(abs_srcdir)
AM_CXXFLAGS = -Wall -Wpedantic
AM_DEFAULT_SOURCE_EXT = .cpp

EXTRA_DIST = tap.hpp image-01.hpp image-02.hpp

check_PROGRAMS = $(TESTS)

