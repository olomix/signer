## Run tests
```shell
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build -V
```

## Build for macOS
```
rm -fr build-macos
cmake -S . -B build-macos -G Xcode
cmake --build build-macos --config Release
```

## Build for iOS
```
rm -fr build-ios-device
cmake -S . -B build-ios-device -G Xcode \
  -DCMAKE_SYSTEM_NAME=Darwin \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build-ios-device --config Release
```

## Build for iOS Simulator
```
rm -fr build-ios-sim
cmake -S . -B build-ios-sim -G Xcode \
  -DCMAKE_SYSTEM_NAME=Darwin \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build-ios-sim --config Release
```

## Create xcFramework
```
xcodebuild -create-xcframework \
  -library "build-macos/Release/libsigner.a" \
  -headers "include" \
  -library "build-ios-device/Release-iphoneos/libsigner.a" \
  -headers "include" \
  -library "build-ios-sim/Release-iphonesimulator/libsigner.a" \
  -headers "include" \
  -output "signer.xcframework"
```