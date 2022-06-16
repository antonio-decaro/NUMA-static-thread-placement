#################################################################################
#                       User Makefile configuration                             #
#################################################################################

#Required Field: root of pin directory. Or use make PIN_ROOT=...
#export PIN_ROOT=

#Enable topology discovery and load to output true core numbering and perform true topology mapping
USE_HWLOC=1

#Enable debug mode
#export DEBUG=0

#Set the Maximum number of thread to handle
export MAX_THREADS=64

#Custom compiler. Not tested without g++
CXX=g++

#Provide additional path for headers and library
export CXXFLAGS=
export LDFLAGS=

