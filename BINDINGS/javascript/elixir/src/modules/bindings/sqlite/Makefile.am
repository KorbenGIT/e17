AUTOMAKE_OPTIONS = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

if BUILD_MODULE_SQLITE
if !EINA_STATIC_BUILD_SQLITE3

INCLUDES = -I. -I$(top_srcdir)/src/include @SQLITE_CFLAGS@ @elixir_cflags@

pkgdir = $(libdir)/elixir/bindings/sqlite/$(MODULE_ARCH)

pkg_LTLIBRARIES = module.la

module_la_SOURCES = sqlite_module.c sqlite_const.c

module_la_LIBADD = $(top_builddir)/src/lib/libelixir.la @SQLITE_LIBS@ @elixir_libs@
module_la_LDFLAGS = -module -avoid-version -L$(tob_buildir)/src/lib -L$(top_buildir)/src/lib/.libs
module_la_DEPENDENCIES = $(top_builddir)/config.h

endif
endif

EXTRA_DIST = sqlite_module.c sqlite_const.c


