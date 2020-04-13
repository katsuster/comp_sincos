SUBDIRS = musl newlib hyb

bin_PROGRAMS = comp_sin

comp_sin_SOURCES   = main.c
comp_sin_CPPFLAGS  = -Wall -Werror
comp_sin_LDADD     = musl/libmusl.a newlib/libnewlib.a hyb/libhyb.a -lpthread -lm
