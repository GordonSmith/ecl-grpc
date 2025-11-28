#OPTION('compileOptions', ' -std=c++17 -Wno-c++11-compat');
// #OPTION('compileOptions', '-Werror=c++17-compat');

UNSIGNED4 test() := EMBED(C++)
    #include "grpc-client.hpp"
#body
    return test() + test2();
ENDC++;

test();

STRING cppVersion() := EMBED(C++)

#body
    char * out = (char*)rtlMalloc(5);
    #if __cplusplus >= 202302L
        memcpy(out, "C++23", 5);        
    #elif __cplusplus >= 202002L
        memcpy(out, "C++20", 5);        
    #elif __cplusplus >= 201703L
        memcpy(out, "C++17", 5);        
    #elif __cplusplus >= 201402L
        memcpy(out, "C++14", 5);        
    #elif __cplusplus >= 201103L
        memcpy(out, "C++11", 5);        
    #elif __cplusplus >= 199711L
        memcpy(out, "C++03", 5);        
    #else
        memcpy(out, "Dunno", 5);
    #endif
    __lenResult = 5;
    __result = out;
ENDEMBED;

cppVersion();
