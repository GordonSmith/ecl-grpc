#OPTION('compileOptions', ' -std=c++17 -Wno-c++11-compat');
// #OPTION('compileOptions', '-Werror=c++17-compat');

RouteConfigRec := RECORD
    UNSIGNED4 seed;
    UNSIGNED4 bonus;
    STRING endpoint;
    STRING context;
END;

RouteResultRec := RECORD
    UNSIGNED4 combinedScore;
    STRING versionSummary;
END;

UNSIGNED4 test(UNSIGNED4 seed, UNSIGNED4 bonus, STRING endpoint) := EMBED(C++)
    #include "grpc-client.hpp"
    #include <iostream>
    #include <string>
#body
    const std::string endpointStr(endpoint, lenEndpoint);
    const auto addressBookStatus = test();
    const auto featureCount = test2();
    const unsigned __int32 total = static_cast<unsigned __int32>(seed + bonus + featureCount + endpointStr.size());
    std::cout << "Endpoint " << endpointStr << " handled seed " << seed
              << " with bonus " << bonus << std::endl;
    return addressBookStatus ? total + static_cast<unsigned __int32>(addressBookStatus) : total;
ENDC++;

STRING cppVersion(STRING context, UNSIGNED4 score) := EMBED(C++)
    #include <cstring>
    #include <string>

#body
    const std::string contextStr(context, lenContext);
    std::string versionTag;
    #if __cplusplus >= 202302L
        versionTag = "C++23";
    #elif __cplusplus >= 202002L
        versionTag = "C++20";
    #elif __cplusplus >= 201703L
        versionTag = "C++17";
    #elif __cplusplus >= 201402L
        versionTag = "C++14";
    #elif __cplusplus >= 201103L
        versionTag = "C++11";
    #elif __cplusplus >= 199711L
        versionTag = "C++03";
    #else
        versionTag = "Dunno";
    #endif
    const std::string payload = contextStr + ": score=" + std::to_string(score) + " built with " + versionTag;
    const size32_t len = static_cast<size32_t>(payload.size());
    char * out = (char*)rtlMalloc(len);
    memcpy(out, payload.data(), len);
    __lenResult = len;
    __result = out;
ENDEMBED;

RouteResultRec runGrpcDemo(RouteConfigRec cfg) := TRANSFORM
    score := test(cfg.seed, cfg.bonus, cfg.endpoint);
    SELF.combinedScore := score;
    SELF.versionSummary := cppVersion(cfg.context, score);
END;

RouteConfigRecDs := DATASET([
    {5, 12, 'localhost:50051', 'route_guide_run'}
], RouteConfigRec);

RouteResultRecDs := PROJECT(RouteConfigRecDs, runGrpcDemo(LEFT));

OUTPUT(RouteResultRecDs);
