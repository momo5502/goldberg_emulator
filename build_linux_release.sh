#!/bin/bash
if [ ! -d "${PWD}/release" ]; then
    mkdir "${PWD}/release"
fi
if [ ! -d "${PWD}/release/experimental" ]; then
    mkdir "${PWD}/release/experimental"
fi
if [ ! -d "${PWD}/release/lobby_connect" ]; then
    mkdir "${PWD}/release/lobby_connect"
fi
protoc -I"${PWD}/dll/" --cpp_out="${PWD}/dll/" "${PWD}"/dll/*.proto
rm -rf "${PWD}/release/experimental"/*
pushd "${PWD}/release/experimental" >/dev/null
echo "Building release libsteam_api.so...."
clang++ -DNDEBUG=1 -DEMU_RELEASE_BUILD=1 -shared -fPIC -o libsteam_api.so ../../dll/*.cpp ../../dll/*.cc -g3 -Wno-return-type -fsanitize=address -shared-libasan -lprotobuf-lite -std=c++14 && echo built libsteam_api.so
cp libsteam_api.so libsteam_api.so.dbg
strip --enable-deterministic-archives --strip-unneeded --remove-section=.comment --remove-section=.note libsteam_api.so
objcopy --add-gnu-debuglink=libsteam_api.so.dbg libsteam_api.so
popd >/dev/null
rm -rf "${PWD}/release/lobby_connect"/*
pushd "${PWD}/release/lobby_connect" >/dev/null
echo "Building release lobby_connect...."
clang++ -DNO_DISK_WRITES=1 -DLOBBY_CONNECT=1 -DNDEBUG=1 -DEMU_RELEASE_BUILD=1 -fPIC -o lobby_connect ../../lobby_connect.cpp ../../dll/*.cpp ../../dll/*.cc -g3 -Wno-return-type -fsanitize=address -shared-libasan -lprotobuf-lite -std=c++14 && echo built lobby_connect
cp lobby_connect lobby_connect.dbg
strip --enable-deterministic-archives --strip-unneeded --remove-section=.comment --remove-section=.note lobby_connect
objcopy --add-gnu-debuglink=lobby_connect.dbg lobby_connect
popd >/dev/null
