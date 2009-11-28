#!/bin/bash
doxygen Doxyfile.klfbackend
doxygen Doxyfile.klfsrc
cd apidoc && tar cvfj klf-apidoc-`cat ../VERSION`.tar.bz2 klfbackend/ klfsrc/ index.html

