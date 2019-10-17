AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = dvrescue
dvrescue_SOURCES = \
    ../../../Source/CLI/CLI_Help.cpp \
    ../../../Source/CLI/CLI_Main.cpp \
    ../../../Source/CLI/CommandLine_Parser.cpp \
    ../../../Source/Common/Core.cpp

INCLUDES = -I../../../Source \
           -I../../../../MediaInfoLib/Source \
           -I../../../../ZenLib/Source
AM_CXXFLAGS = -std=c++0x

# man1_MANS = ../../../Source/CLI/dvrescue.1

AM_TESTS_FD_REDIRECT = 9>&2

TESTS = test/test1.sh

TESTING_DIR = test/TestingFiles

clone_testing_files: $(TESTING_DIR)

$(TESTING_DIR):
    git clone https://github.com/MediaArea/dvrescue-sample-data.git test/TestingFiles
