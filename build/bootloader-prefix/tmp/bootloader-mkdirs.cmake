# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/Espressif/frameworks/esp-idf-v5.0/components/bootloader/subproject"
  "D:/Diogo/Universidade/ASE/ase-project/build/bootloader"
  "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix"
  "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix/tmp"
  "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix/src"
  "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Diogo/Universidade/ASE/ase-project/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
