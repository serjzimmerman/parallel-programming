AM_CXXFLAGS = -I$(top_srcdir)/include $(BOOST_CPPFLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS)

bin_PROGRAMS = sort
sort_SOURCES = src/sort.cpp
sort_LDADD = $(BOOST_LDFLAGS) -lboost_mpi -lboost_program_options -lboost_serialization
