# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-src"
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-build"
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix"
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix/tmp"
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp"
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix/src"
  "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Lukas Knoepfle/Documents/Uni/PPProjekte/Uebung2/cmake-build-debug/_deps/cli11-subbuild/cli11-populate-prefix/src/cli11-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
