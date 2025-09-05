#!/usr/bin/env bash

set -euo pipefail

# Build libsecp256k1 as an XCFramework for:
# - iOS device (arm64)
# - iOS simulator (arm64)
# - macOS (arm64)

# Resolve script base directory
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"

REPO_OWNER="bitcoin-core"
REPO_NAME="secp256k1"

# Pinned version and checksum
SECP256K1_VERSION="0.7.0"
SECP256K1_TAG="v0.7.0"
SECP256K1_SHA256="0160b24791d6691dfcebb7157acf8cb52be495eef020d10252cffa57367ac3cc"

mkdir -p "${SCRIPT_DIR}/external"

# Official release asset URL and paths
SECP256K1_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/${SECP256K1_TAG}/libsecp256k1-${SECP256K1_VERSION}.tar.gz"
TARBALL_PATH="${SCRIPT_DIR}/external/libsecp256k1-${SECP256K1_VERSION}.tar.gz"
SRC_DIR="${SCRIPT_DIR}/external/libsecp256k1-${SECP256K1_VERSION}"

# Download and extract if source not present
if [ ! -d "${SRC_DIR}" ]; then
  if [ ! -f "${TARBALL_PATH}" ]; then
    echo "Downloading ${SECP256K1_URL} -> ${TARBALL_PATH}"
    curl -fL "${SECP256K1_URL}" -o "${TARBALL_PATH}"
  fi

  CALC_SHA256=$(shasum -a 256 "${TARBALL_PATH}" | awk '{print $1}')
  if [ "${CALC_SHA256}" != "${SECP256K1_SHA256}" ]; then
    echo "ERROR: SHA256 mismatch for ${TARBALL_PATH}" >&2
    echo "Expected: ${SECP256K1_SHA256}" >&2
    echo "Actual:   ${CALC_SHA256}" >&2
    exit 1
  fi

  echo "Extracting ${TARBALL_PATH}"
  tar -xzf "${TARBALL_PATH}" -C "${SCRIPT_DIR}/external"
else
  echo "Source already present; skipping download and extraction"
fi

# Build helper for a given SDK/host/flags
build_variant() {
  local sdk="$1"; shift
  local host="$1"; shift
  local cflags="$1"; shift
  local install_path="$1"; shift
  local build_dir="$1"; shift

  mkdir -p "${build_dir}"
  mkdir -p "${install_path}"

  pushd "${build_dir}"

  if [ -f Makefile ]; then
    make distclean || true
  fi

  echo "Configuring secp256k1 for ${sdk} -> ${build_dir} (host=${host})"
  (
    CC="xcrun --sdk ${sdk} clang" \
    CFLAGS="${cflags}" \
    LDFLAGS="${cflags}" \
    "${SRC_DIR}/configure" \
      --host="${host}" \
      --prefix="${install_path}" \
      --disable-shared --enable-static --with-pic \
      --disable-tests --disable-benchmark \
      --enable-module-recovery --enable-module-ecdh

    make -j "$(getconf NPROCESSORS_ONLN)"
    make -j "$(getconf NPROCESSORS_ONLN)" install
  )

  popd
}

# Paths per-variant
INSTALL_IOS_DEV="${SCRIPT_DIR}/external/secp256k1-ios-arm64"
INSTALL_IOS_SIM="${SCRIPT_DIR}/external/secp256k1-iossim-arm64"
INSTALL_MACOS="${SCRIPT_DIR}/external/secp256k1-macos-arm64"

BUILD_IOS_DEV="${SCRIPT_DIR}/external/build-secp256k1-ios-arm64"
BUILD_IOS_SIM="${SCRIPT_DIR}/external/build-secp256k1-iossim-arm64"
BUILD_MACOS="${SCRIPT_DIR}/external/build-secp256k1-macos-arm64"

# Build iOS device (arm64)
IOS_DEV_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path)
build_variant \
  iphoneos \
  aarch64-apple-ios \
  "-arch arm64 -isysroot ${IOS_DEV_SYSROOT} -mios-version-min=12.0" \
  "${INSTALL_IOS_DEV}" \
  "${BUILD_IOS_DEV}"

# Build iOS simulator (arm64)
IOS_SIM_SYSROOT=$(xcrun --sdk iphonesimulator --show-sdk-path)
build_variant \
  iphonesimulator \
  aarch64-apple-darwin \
  "-arch arm64 -isysroot ${IOS_SIM_SYSROOT} -mios-simulator-version-min=12.0" \
  "${INSTALL_IOS_SIM}" \
  "${BUILD_IOS_SIM}"

# Build macOS (arm64)
MACOS_SYSROOT=$(xcrun --sdk macosx --show-sdk-path)
build_variant \
  macosx \
  aarch64-apple-darwin \
  "-arch arm64 -isysroot ${MACOS_SYSROOT} -mmacosx-version-min=11.0" \
  "${INSTALL_MACOS}" \
  "${BUILD_MACOS}"

# Create XCFramework
XCFRAMEWORK_PATH="${SCRIPT_DIR}/external/secp256k1.xcframework"
if [ -d "${XCFRAMEWORK_PATH}" ]; then
  echo "Removing existing ${XCFRAMEWORK_PATH}"
  rm -rf "${XCFRAMEWORK_PATH}"
fi

xcodebuild -create-xcframework \
  -library "${INSTALL_IOS_DEV}/lib/libsecp256k1.a" \
  -headers "${INSTALL_IOS_DEV}/include" \
  -library "${INSTALL_IOS_SIM}/lib/libsecp256k1.a" \
  -headers "${INSTALL_IOS_SIM}/include" \
  -library "${INSTALL_MACOS}/lib/libsecp256k1.a" \
  -headers "${INSTALL_MACOS}/include" \
  -output "${XCFRAMEWORK_PATH}"

echo "Done. XCFramework: ${XCFRAMEWORK_PATH}"
