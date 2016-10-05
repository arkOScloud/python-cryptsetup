from setuptools import setup, Extension
setup(name="pycryptsetup",
      version = '1.7.2',
      description = "Python bindings for cryptsetup",
      author = "Martin Sivak",
      author_email= "msivak@redhat.com",
      license = 'GPLv2+',
      ext_modules = [Extension("pycryptsetup", ["pycryptsetup.c"], libraries=['cryptsetup'])]
      )
