#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"

# get distribution
if [ -e "/etc/os-release" ]
then
    DISTRO=$(grep "^ID=" "/etc/os-release" | cut -d"=" -f2| sed 's/"//g')
    DISTRO_VERSION=$(grep "^VERSION_ID=" "/etc/os-release" | cut -d"=" -f2)
else
    echo "Linux distribution not found."
    echo "Maybe the file \"/etc/os-release\" is just missing."
    echo "Or your linux distribution is too old. Cannot check requirements."
    echo ""
fi

opensuse_requirements(){
    if ! which "protoc" > /dev/null 2>&1
    then
        echo "protoc isn't installed."
        echo "Please install \"protobuf-devel\" package and try again"
        exit 1
    fi
    if ! which "clang++" > /dev/null 2>&1
    then
        echo "clang++ isn't installed."
        echo "Please install \"clang5\" package and try again"
        exit 1
    fi
}

if [ -n "${DISTRO}" ]
then
    case "${DISTRO}" in
        opensuse|opensuse-leap)
            opensuse_requirements
            ;;
        *)
            ;;
    esac
fi

protoc -I./dll/ --cpp_out=./dll/ ./dll/*.proto
clang++ -shared -fPIC -o libsteam_api.so dll/*.cpp dll/*.cc -g3 -Wno-return-type -fsanitize=address -lasan -lprotobuf-lite -std=c++14 && echo built 
