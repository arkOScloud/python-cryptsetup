#!/usr/bin/python
import sys
import os

sys.path.insert(0, "build/lib.linux-x86_64-2.5/")

import cryptsetup

def log(pri, txt):
    print "LOG:", txt, "\n"

def askyes(txt):
    print "Asking about:", txt, "\n"
    return 1

c = cryptsetup.CryptSetup(askyes, log)
print c.log(5, "loguj!")
print c.askyes("bude zima?")

print "/dev/sda1", c.isLuks("/dev/sda1")
print "/dev/loop0", c.isLuks("/dev/loop0")
print "/dev/loop0", c.luksUUID("/dev/loop0")

print "sifra", c.luksStatus("sifra")
print "open sifra", c.luksOpen("/dev/loop0", "sifra")
print "sifra", c.luksStatus("sifra")
print "close sifra", c.luksClose("sifra")
print "sifra", c.luksStatus("sifra")
