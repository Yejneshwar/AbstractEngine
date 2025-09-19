#https://github.com/leetal/ios-cmake/tree/master
cmake -DCMAKE_TOOLCHAIN_FILE=./ios.cmake -DPLATFORM=OS64COMBINED -H. -Bbuild.ios -GXcode
