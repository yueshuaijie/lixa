noinst_HEADERS = liblixamonkey.h
lib_LTLIBRARIES = liblixamonkey.la \
	switch_lixa_monkeyrm_dynreg.la \
	switch_lixa_monkeyrm_stareg.la \
	switch_lixa_dummyrm.la \
	switch_lixa_randomrm.la 
liblixamonkey_la_SOURCES = liblixamonkey.c
switch_lixa_monkeyrm_dynreg_la_SOURCES = lixa_monkeyrm_dynreg.c
switch_lixa_monkeyrm_stareg_la_SOURCES = lixa_monkeyrm_stareg.c
switch_lixa_dummyrm_la_SOURCES = lixa_dummyrm.c
switch_lixa_randomrm_la_SOURCES = lixa_randomrm.c
AM_CPPFLAGS = -I$(srcdir)/../../../common
AM_CFLAGS = -Wall
AM_LDFLAGS = -module $(srcdir)/../../../common/liblixab.la
AM_LIBS = $(srcdir)/../../../common/liblixab.la
liblixamonkey_la_LDFLAGS =
# the dynamic version is compiled with multithread library, the static version
# is compiled without: this allow to mix single thread/multi thread switch
# files and to simulate a more complex environment
switch_lixa_monkeyrm_dynreg_la_LDFLAGS = -module -lpthread
switch_lixa_monkeyrm_stareg_la_LDFLAGS = -module -lpthread
switch_lixa_monkeyrm_dynreg_la_LIBADD = liblixamonkey.la $(srcdir)/../../../common/liblixab.la
switch_lixa_monkeyrm_stareg_la_LIBADD = liblixamonkey.la $(srcdir)/../../../common/liblixab.la
