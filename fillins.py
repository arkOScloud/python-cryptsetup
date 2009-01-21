# File name: fillins.py
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

def luks_add_key(device,
                 new_passphrase=None, new_key_file=None,
                 passphrase=None, key_file=None):

    p = os.pipe()
    if passphrase:
        os.write(p[1], "%s\n" % passphrase)
        key_spec = ""
    elif key_file and os.path.isfile(key_file):
        key_spec = "--key-file %s" % key_file
    else:
        raise ValueError(_("luks_add_key requires either a passphrase or a key file"))

    if new_passphrase:
        os.write(p[1], "%s\n" % new_passphrase)
        new_key_spec = ""
    elif new_key_file and os.path.isfile(new_key_file):
        new_key_spec = "%s" % new_key_file
    else:
        raise ValueError(_("luks_add_key requires either a passphrase or a key file to add"))

    os.close(p[1])

    rc = iutil.execWithRedirect("cryptsetup",
                                ["-q",
                                 key_spec,
                                 "luksAddKey",
                                 device,
                                 new_key_spec],
                                stdin = p[0],
                                stdout = "/dev/null",
                                stderr = "/dev/null",
                                searchPath = 1)

    os.close(p[0])
    if rc:
        raise RuntimeError(_("luks add key failed"))

def luks_remove_key(device,
                    del_passphrase=None, del_key_file=None,
                    passphrase=None, key_file=None):

    p = os.pipe()
    if passphrase:
        os.write(p[1], "%s\n" % passphrase)
        key_spec = ""
    elif key_file and os.path.isfile(key_file):
        key_spec = "--key-file %s" % key_file
    else:
        raise ValueError(_("luks_remove_key requires either a passphrase or a key file"))

    if del_passphrase:
        os.write(p[1], "%s\n" % del_passphrase)
        del_key_spec = ""
    elif del_key_file and os.path.isfile(del_key_file):
        del_key_spec = "%s" % del_key_file
    else:
        raise ValueError(_("luks_remove_key requires either a passphrase or a key file to remove"))

    os.close(p[1])

    rc = iutil.execWithRedirect("cryptsetup",
                                ["-q",
                                 key_spec,
                                 "luksRemoveKey",
                                 device,
                                 del_key_spec],
                                stdin = p[0],
                                stdout = "/dev/null",
                                stderr = "/dev/null",
                                searchPath = 1)

    os.close(p[0])
    if rc:
        raise RuntimeError(_("luks_remove_key failed"))



