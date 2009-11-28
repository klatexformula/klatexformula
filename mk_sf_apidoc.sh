#!/bin/bash
cat Doxyfile.klfbackend | sed 's/footer.html/footer_sf.html/g' | doxygen - 
cat Doxyfile.klfsrc | sed 's/footer.html/footer_sf.html/g' | doxygen -
