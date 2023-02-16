NOTE:

When using the shared lib:
1. with LD_LIBRARY_PATH
As long as the shared library is not installed in a default location (such as /usr/lib), we must indicate where it is found. This is possible with the LD_LIBRARY_PATH environment variable.
	LD_LIBRARY_PATH=$(pwd)/bin/shared bin/use-shared-library
2. Move the shared library to a default location
Let's move the shared library to /usr/lib so that we can execute bin/use-shared-library without explicitly setting the LD_LIBRARY_PATH variable.

References: https://renenyffenegger.ch/notes/development/languages/C-C-plus-plus/GCC/create-libraries/index
