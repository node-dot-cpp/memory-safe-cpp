#!/bin/sh
set -ev

export CC="ccache ${CC}"
export CXX="ccache ${CXX}"

cd checker
./checkout.sh
./build.sh
