# File name: __init__.py
# Date:      2009/01/19
# Author:    Martin Sivak
#
# Copyright (C) Red Hat 2009
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# in a file called COPYING along with this program; if not, write to
# the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA
# 02139, USA.

import cryptsetup
import fillins

class CryptSetup(cryptsetup.CryptSetup):
    def add_key(self, *args, **kwargs):
        return fillins.luks_add_key(*args, **kwargs)
    
    def remove_key(self, *args, **kwargs):
        return fillins.luks_remove_key(*args, **kwargs)

