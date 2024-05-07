# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/lorenzo/esp/esp-idf/components/bootloader/subproject"
  "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader"
  "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix"
  "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix/tmp"
  "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix/src"
  "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/lorenzo/Desktop/esp-idf-ds3231/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
