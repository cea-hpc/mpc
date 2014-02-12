#This file use the template toolchain file from cmake corss compile wiki
#thanks to the original authors

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER icc -mmic)
SET(CMAKE_CXX_COMPILER icpc -mmic)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH  )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
