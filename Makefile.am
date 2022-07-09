ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = test

nobase_include_HEADERS = ppht.hpp ppht/accumulator.hpp ppht/channel.hpp ppht/kd-search.hpp ppht/parameters.hpp ppht/point_set.hpp ppht/postprocess.hpp ppht/raster.hpp ppht/state.hpp ppht/trig.hpp ppht/types.hpp

git-add:
	$(MAKE) distdir
	@(cd "$(distdir)"; find . -depth -type f -print0) | (cd "$(top_srcdir)"; xargs -0t git add --verbose)
	$(RM) -r "$(distdir)"

doc: Doxyfile
	cd "$(top_srcdir)"; doxygen "$(abs_top_builddir)/Doxyfile"
	rm -rf doc; mv "$(top_srcdir)/doc" .

git-doc: doc
	cd "$(top_srcdir)"; git checkout gh-pages
	cd "$(top_srcdir)"; git rm -rf doc; \
	mv "$(abs_top_builddir)/doc" .; git add -f doc

git-tag:
	cd "$(top_srcdir)"; git tag -f -a "v$(PACKAGE_VERSION)" -m "v$(PACKAGE_VERSION)"

gh-release: git-tag dist-gzip
	cd "$(top_srcdir)"; \
	gh release create "v$(PACKAGE_VERSION)" \
		"$(abs_top_builddir)/$(distdir).tar.gz"

clean-local:
	@find . -name '*~' -print0 | xargs -0t $(RM)

