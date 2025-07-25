#define bTEST_IMPLEMENTATION ///< as per the bUnitTests documentations
#include "bUnitTests.h" ///< "header only" unit testing framework. Implementation "lives" in this file (bEntryPoint.cpp)

// this just creates a macro 'bENTRY_POINT' to reflect the current configuration/platform's entry point
#if defined RELEASE && defined _WIN32
#    include <Windows.h>
#    define bENTRY_POINT() WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
#endif
#ifndef bENTRY_POINT
#    define bENTRY_POINT() main()
#endif

// IMPORTANT NOTE:
//
// If we are building an application (console or window), this serves as the (default) entry point to our program.
//
// Typically, libraries _do not_ have main functions, but in some cases they might! By default, libraries _will not_
// implement this main function. However, this behavior can be changed by editing this (default) file, or more
// specifically by uncommenting the following line:

// #undef bNO_ENTRY_POINT

// if we're building tests, the "bUnitTests.h" header already has a main function to use as the (test) application's
// entry point so we don't need to include another entry point. This even overrides the value set above!
//
// the 'bBUILD_TESTS' value is a unit testing application preprocessor definition, defined by the testing application
#if (not defined bBUILD_TESTS && not defined bNO_ENTRY_POINT)

int bENTRY_POINT() {
    // main function goes here!
};

#endif // !bBUILD_TESTS && !bNO_ENTRY_POINT

// since this is single-file header only "library", we need to include -something- here so something compiles to trigger
// tests to run (since tests run automatically after a build in release mode)! Could also achieve this by including an
// "emtpy" bJSON.cpp file which only has the following line, but this is just as easy:
#include "bJSON.h"
