#!/bin/bash
protoc -I"${PWD}/dll/" --cpp_out="${PWD}/dll/" "${PWD}"/dll/*.proto
clang++ -shared -fPIC -o libsteam_api.so dll/*.cpp dll/*.cc -g3 -Wno-return-type -fsanitize=address -lasan -lprotobuf-lite -std=c++14 && echo built libsteam_api.so
clang++ -fPIC -o lobby_connect lobby_connect.cpp dll/*.cpp dll/*.cc -g3 -Wno-return-type -fsanitize=address -lasan -lprotobuf-lite -std=c++14 && echo built lobby_connect