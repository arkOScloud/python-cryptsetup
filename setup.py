from distutils.core import setup, Extension
setup(name="cryptsetup", version="1.0",
              ext_modules=[Extension("cryptsetup", ["cryptsetup.c"], library_dirs=['.'], libraries=['cryptsetup'])])

