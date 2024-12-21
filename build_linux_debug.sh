#!/bin/bash
if [ ! -d "${PWD}/debug" ]; then
    mkdir "${PWD}/debug"
fi
if [ ! -d "${PWD}/debug/experimental" ]; then
    mkdir "${PWD}/debug/experimental"
fi
if [ ! -d "${PWD}/debug/lobby_connect" ]; then
    mkdir "${PWD}/debug/lobby_connect"
fi
protoc -I"${PWD}/dll/" --cpp_out="${PWD}/dll/" "${PWD}"/dll/*.proto
rm -rf "${PWD}/debug/experimental"/*
pushd "${PWD}/debug/experimental" >/dev/null
echo "Building debug libsteam_api.so...."
clang++ -shared -fPIC -o libsteam_api.so ../../dll/*.cpp ../../dll/*.cc -g3 -Wno-return-type -fsanitize=address -shared-libasan -lprotobuf-lite -std=c++14 && echo built libsteam_api.so
popd >/dev/null
rm -rf "${PWD}/debug/lobby_connect"/*
pushd "${PWD}/debug/lobby_connect" >/dev/null
echo "Building debug lobby_connect...."
clang++ -DNO_DISK_WRITES=1 -DLOBBY_CONNECT=1 -fPIC -o lobby_connect ../../lobby_connect.cpp ../../dll/*.cpp ../../dll/*.cc -g3 -Wno-return-type -fsanitize=address -shared-libasan -lprotobuf-lite -std=c++14 && echo built lobby_connect
popd >/dev/null
