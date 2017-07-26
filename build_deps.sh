#!/bin/bash
set -e -u -E

########################################
# download & build depend software
########################################

WORK_DIR=$(cd $(dirname $0); pwd)
DEPS_SOURCE=$WORK_DIR/thirdsrc
DEPS_PREFIX=$WORK_DIR/thirdparty
DEPS_CONFIG="--prefix=${DEPS_PREFIX} --disable-shared --with-pic"
FLAG_DIR=$WORK_DIR/.build
GIT_REPO="http://gitlab.baidu.com/wanghaitao01/thirdsrc"
#GIT_REPO="https://github.com/taotaowill/thirdsrc"

export PATH=${DEPS_PREFIX}/bin:$PATH
mkdir -p ${DEPS_SOURCE} ${DEPS_PREFIX} ${FLAG_DIR}

cd ${DEPS_SOURCE}

# boost
if [ ! -f "${FLAG_DIR}/boost_1_57_0" ] \
    || [ ! -d "${DEPS_PREFIX}/boost_1_57_0/boost" ]; then
    wget --no-check-certificate -O boost_1_57_0.tar.gz  $GIT_REPO/raw/master/boost_1_57_0.tar.gz
    tar zxf boost_1_57_0.tar.gz
    rm -rf ${DEPS_PREFIX}/boost_1_57_0
    mv boost_1_57_0 ${DEPS_PREFIX}
    cd ${DEPS_PREFIX}/boost_1_57_0 && ./bootstrap.sh && ./b2 --with-filesystem link=static
    cd -
    touch "${FLAG_DIR}/boost_1_57_0"
fi

# protobuf
if [ ! -f "${FLAG_DIR}/protobuf_2_6_1" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libprotobuf.a" ] \
    || [ ! -d "${DEPS_PREFIX}/include/google/protobuf" ]; then
    wget --no-check-certificate -O protobuf-2.6.1.tar.gz $GIT_REPO/raw/master/protobuf-2.6.1.tar.gz
    tar zxf protobuf-2.6.1.tar.gz
    cd protobuf-2.6.1
    ./configure ${DEPS_CONFIG}
    make -j4
    make install
    cd -
    touch "${FLAG_DIR}/protobuf_2_6_1"
fi

# snappy
if [ ! -f "${FLAG_DIR}/snappy_1_1_1" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libsnappy.a" ] \
    || [ ! -f "${DEPS_PREFIX}/include/snappy.h" ]; then
    wget --no-check-certificate -O snappy-1.1.1.tar.gz $GIT_REPO/raw/master/snappy-1.1.1.tar.gz
    tar zxf snappy-1.1.1.tar.gz
    cd snappy-1.1.1
    ./configure ${DEPS_CONFIG}
    make -j4
    make install
    cd -
    touch "${FLAG_DIR}/snappy_1_1_1"
fi

# sofa-pbrpc
if [ ! -f "${FLAG_DIR}/sofa-pbrpc_1_1_3" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libsofa-pbrpc.a" ] \
    || [ ! -d "${DEPS_PREFIX}/include/sofa/pbrpc" ]; then
    wget --no-check-certificate -O sofa-pbrpc-1.1.3.tar.gz $GIT_REPO/raw/master/sofa-pbrpc-1.1.3.tar.gz
    tar zxf sofa-pbrpc-1.1.3.tar.gz
    cd sofa-pbrpc-1.1.3
    sed -i '/BOOST_HEADER_DIR=/ d' depends.mk
    sed -i '/PROTOBUF_DIR=/ d' depends.mk
    sed -i '/SNAPPY_DIR=/ d' depends.mk
    echo "BOOST_HEADER_DIR=${DEPS_PREFIX}/boost_1_57_0" >> depends.mk
    echo "PROTOBUF_DIR=${DEPS_PREFIX}" >> depends.mk
    echo "SNAPPY_DIR=${DEPS_PREFIX}" >> depends.mk
    echo "PREFIX=${DEPS_PREFIX}" >> depends.mk
    cd src
    cd ..
    make -j4
    make install
    cd ..
    touch "${FLAG_DIR}/sofa-pbrpc_1_1_3"
fi

# cmake for gflags
if ! which cmake ; then
    wget --no-check-certificate -O CMake-3.2.1.tar.gz $GIT_REPO/raw/master/CMake-3.2.1.tar.gz
    tar zxf CMake-3.2.1.tar.gz
    cd CMake-3.2.1
    ./configure --prefix=${DEPS_PREFIX}
    make -j4
    make install
    cd -
fi

# gflags
if [ ! -f "${FLAG_DIR}/gflags_2_1_1" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libgflags.a" ] \
    || [ ! -d "${DEPS_PREFIX}/include/gflags" ]; then
    wget --no-check-certificate -O gflags-2.1.1.tar.gz $GIT_REPO/raw/master/gflags-2.1.1.tar.gz
    tar zxf gflags-2.1.1.tar.gz
    cd gflags-2.1.1
    cmake -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX} -DGFLAGS_NAMESPACE=google -DCMAKE_CXX_FLAGS=-fPIC
    make -j4
    make install
    cd -
    touch "${FLAG_DIR}/gflags_2_1_1"
fi

# glog
if [ ! -f "${FLAG_DIR}/glog_0_3_3" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libglog.a" ] \
    || [ ! -d "${DEPS_PREFIX}/include/glog" ]; then
    wget --no-check-certificate -O glog-0.3.3.tar.gz $GIT_REPO/raw/master/glog-0.3.3.tar.gz
    tar zxf glog-0.3.3.tar.gz
    cd glog-0.3.3
    ./configure ${DEPS_CONFIG} CPPFLAGS=-I${DEPS_PREFIX}/include LDFLAGS=-L${DEPS_PREFIX}/lib
    make -j4
    make install
    cd -
    touch "${FLAG_DIR}/glog_0_3_3"
fi

# gtest
if [ ! -f "${FLAG_DIR}/gtest_1_7_0" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libgtest.a" ] \
    || [ ! -d "${DEPS_PREFIX}/include/gtest" ]; then
    rm -rf ./googletest-release-1.7.0
    wget --no-check-certificate -O gtest_1_7_0.tar.gz $GIT_REPO/raw/master/gtest_1_7_0.tar.gz
    tar -xzf gtest_1_7_0.tar.gz
    cd googletest-release-1.7.0
    sed -i 's/-Wno-missing-field-initializers//g' cmake/internal_utils.cmake
    cmake .
    make
    cp -af lib*.a ${DEPS_PREFIX}/lib
    cp -af include/gtest ${DEPS_PREFIX}/include
    cd -
    touch "${FLAG_DIR}/gtest_1_7_0"
fi

# libunwind for gperftools
if [ ! -f "${FLAG_DIR}/libunwind_0_99_beta" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libunwind.a" ] \
    || [ ! -f "${DEPS_PREFIX}/include/libunwind.h" ]; then
    wget --no-check-certificate -O libunwind-0.99-beta.tar.gz $GIT_REPO/raw/master/libunwind-0.99-beta.tar.gz
    tar zxf libunwind-0.99-beta.tar.gz
    cd libunwind-0.99-beta
    ./configure ${DEPS_CONFIG}
    make CFLAGS=-fPIC -j4
    make CFLAGS=-fPIC install
    cd -
    touch "${FLAG_DIR}/libunwind_0_99_beta"
fi

# gperftools (tcmalloc)
if [ ! -f "${FLAG_DIR}/gperftools_2_2_1" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libtcmalloc_minimal.a" ]; then
    wget --no-check-certificate -O gperftools-2.2.1.tar.gz $GIT_REPO/raw/master/gperftools-2.2.1.tar.gz
    tar zxf gperftools-2.2.1.tar.gz
    cd gperftools-2.2.1
    ./configure ${DEPS_CONFIG} CPPFLAGS=-I${DEPS_PREFIX}/include LDFLAGS=-L${DEPS_PREFIX}/lib
    make -j4
    make install
    cd -
    touch "${FLAG_DIR}/gperftools_2_2_1"
fi

# ins
#if [ ! -f "${FLAG_DIR}/ins" ] \
#    || [ ! -f "${DEPS_PREFIX}/lib/libins_sdk.a" ] \
#    || [ ! -f "${DEPS_PREFIX}/include/ins_sdk.h" ]; then
#    rm -rf ins
#    git clone https://github.com/baidu/ins
#    cd ins
#    sed -i "s|^PREFIX=.*|PREFIX=${DEPS_PREFIX}|" Makefile
#    sed -i "s|^PROTOC ?=.*|PROTOC=${DEPS_PREFIX}/bin/protoc|" Makefile
#    sed -i "s|^GFLAGS_PATH ?=.*|GFLAGS_PATH=${DEPS_PREFIX}|" Makefile
#    export BOOST_PATH=${DEPS_PREFIX}/boost_1_57_0
#    make -j4 default >/dev/null && make -j4 install_sdk >/dev/null
#    cd -
#    touch "${FLAG_DIR}/ins"
#fi
if [ ! -f "${FLAG_DIR}/ins" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libins_sdk.a" ] \
    || [ ! -f "${DEPS_PREFIX}/include/ins_sdk.h" ]; then
    wget --no-check-certificate http://gitlab.baidu.com/wanghaitao01/packages/raw/master/ins-0.15.tar.gz
    tar xzf ins-0.15.tar.gz
    cd ins-0.15
    sed -i "s|^PREFIX=.*|PREFIX=${DEPS_PREFIX}|" Makefile
    sed -i "s|^PROTOC=.*|PROTOC=${DEPS_PREFIX}/bin/protoc|" Makefile
    export BOOST_PATH=${DEPS_PREFIX}/boost_1_57_0
    make -j4 ins >/dev/null && make -j4 install_sdk >/dev/null
    mkdir -p output/bin && cp ins output/bin
    cd -
    touch "${FLAG_DIR}/ins"
fi

# common
if [ ! -f "${FLAG_DIR}/common" ] \
    || [ ! -f "${DEPS_PREFIX}/lib/libcommon.a" ] \
    || [ ! -f "${DEPS_PREFIX}/include/common/mutex.h" ]; then
    rm -rf common
    git clone https://github.com/baidu/common
    cd common
    sed -i 's/^INCLUDE_PATH=.*/INCLUDE_PATH=-Iinclude -I..\/..\/thirdparty\/boost_1_57_0/' Makefile
    make -j4
    mkdir -p ${DEPS_PREFIX}/include/common
    cp -rf include/* ${DEPS_PREFIX}/include/common
    cp libcommon.a ${DEPS_PREFIX}/lib
    cd -
    touch "${FLAG_DIR}/common"
fi

cd ${WORK_DIR}
echo "build deps done!"

