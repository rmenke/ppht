ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = test

nobase_include_HEADERS = ppht.hpp ppht/accumulator.hpp ppht/channel.hpp ppht/parameters.hpp ppht/point_set.hpp ppht/raster.hpp ppht/state.hpp ppht/trig.hpp ppht/types.hpp

git-add:
	$(MAKE) distdir
	@set -x; (cd "$(distdir)"; find . -depth -type f -print0) | \
		(cd $(top_srcdir); xargs -0t git add)
	$(RM) -r "$(distdir)"

git-doc:
	$(MAKE) Doxyfile; doxygen
	@set -xe; TMP="`mktemp -u -t doxygen`"; \
		mv doc "$${TMP}"; \
		git co gh-pages; \
		git rm -r doc; \
		mv "$${TMP}" doc; \
		git add doc

git-tag:
	git tag -f -a "v$(PACKAGE_VERSION)" -m "v$(PACKAGE_VERSION)"

clean-local:
	@find . -name '*~' -print0 | xargs -0t $(RM)

