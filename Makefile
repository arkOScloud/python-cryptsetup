# First Aid Kit - diagnostic and repair tool for Linux
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


NAME := "pycryptsetup"
VERSION := $(shell awk '/Version:/ { print $$2 }' pycryptsetup.spec)
RELEASE := $(shell awk '/Release:/ { print $$2 }' pycryptsetup.spec)
DATADIR := $(shell rpm --eval "%_datadir")

tarball:
	git-archive --format=tar --prefix=$(NAME)-$(VERSION)/ HEAD | bzip2 -f > $(NAME)-$(VERSION).tar.bz2

srpm: tarball
	rpmbuild -ts --nodeps $(NAME)-$(VERSION).tar.bz2
	rm -f $(NAME)-$(VERSION).tar.bz2

bumpver:
	@MAYORVER=$$(echo $(VERSION) | cut -d . -f 1-2); \
	NEWSUBVER=$$((`echo $(VERSION) | cut -d . -f 3`+1)); \
	sed -i "s/Version:        $(VERSION)/Version:        $$MAYORVER.$$NEWSUBVER/" pycryptsetup.spec; \
	sed -i "s/Release:        .*%/Release:        1%/" pycryptsetup.spec; \
	sed -i "s/version=.*/version='$$MAYORVER.$$NEWSUBVER',/" setup.py;
	git commit -a -m "Bump version"

newver:
	make bumpver
	make srpm

