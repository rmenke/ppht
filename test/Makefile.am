TESTS = 01-raster.test \
        02-trig.test \
        03-accumulator.test \
        04-channel.test \
        05-point_set.test \
        06-state.test \
        07-ppht.test

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/build-aux/tap-driver.sh

AM_CPPFLAGS = -I$(abs_top_srcdir) -I$(abs_srcdir)
AM_CXXFLAGS = -Wall -Wpedantic

check_PROGRAMS = $(TESTS)

01_raster_test_SOURCES = 01-raster.cpp tap.hpp
02_trig_test_SOURCES = 02-trig.cpp tap.hpp
03_accumulator_test_SOURCES = 03-accumulator.cpp tap.hpp
04_channel_test_SOURCES = 04-channel.cpp tap.hpp
05_point_set_test_SOURCES = 05-point_set.cpp tap.hpp
06_state_test_SOURCES = 06-state.cpp tap.hpp
07_ppht_test_SOURCES = 07-ppht.cpp tap.hpp image-01.hpp image-02.hpp
