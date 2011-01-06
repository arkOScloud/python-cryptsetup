#!/usr/bin/python
import sys
import os

import pycryptsetup as cryptsetup

def log(pri, txt):
    print "LOG[%d]: %s\n" % (pri, txt)

def askyes(txt):
    print "Asking about:", txt, "\n"
    return 1

def askpassword(txt):
    print "Providing password for %s ...\n" % txt
    return "heslo"

#c1 = cryptsetup.CryptSetup(device="/dev/sda1", yesDialog = askyes, logFunc = log, passwordDialog = askpassword)
c2 = cryptsetup.CryptSetup(device="/dev/loop0", name="sifra", yesDialog = askyes, logFunc = log, passwordDialog = askpassword)

print dir(c2)

print c2.yesDialogCB
print c2.cmdLineLogCB
print c2.passwordDialogCB

print c2.log(5, "loguj!")
print c2.askyes("bude zima?")

#print "/dev/sda1", c1.isLuks()
print "/dev/loop0", c2.isLuks()
print "/dev/loop0", c2.luksUUID()

try:
    c3 = cryptsetup.CryptSetup(name="sifra", yesDialog = askyes, logFunc = log, passwordDialog = askpassword)
    print "sifra", c3.status()
except IOError as e:
    print e

print "format sifra", c2.luksFormat()
print "/dev/loop0", c2.isLuks()
print "create password", c2.addPassphrase("heslo")
#print "close sifra", c2.deactivate()
#print "open sifra", c2.activate("sifra")
print "close sifra", c2.deactivate()
print "open sifra", c2.activate("sifra", "heslo")

#import pdb; pdb.set_trace()

c3 = cryptsetup.CryptSetup(name="sifra", yesDialog = askyes, logFunc = log, passwordDialog = askpassword)
print "sifra", c3.info()

print "close sifra", c3.deactivate()

print "sifra", c3.info()


