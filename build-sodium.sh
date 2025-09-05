#!/usr/bin/env bash

set -euo pipefail

# Libsodium release details (pinned, do not override)
LIBSODIUM_VERSION="1.0.20"
LIBSODIUM_SHA256="ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19"
LIBSODIUM_URL="https://download.libsodium.org/libsodium/releases/libsodium-${LIBSODIUM_VERSION}.tar.gz"
# Resolve script base directory
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"

LIBSODIUM_TARBALL="${SCRIPT_DIR}/external/libsodium-${LIBSODIUM_VERSION}.tar.gz"
LIBSODIUM_SRC_DIR="${SCRIPT_DIR}/external/libsodium-${LIBSODIUM_VERSION}"

# Extract source if not already present. If the source directory exists,
# skip download, checksum, and extraction.
if [ ! -d "${LIBSODIUM_SRC_DIR}" ]; then
  # Ensure the tarball exists; download if missing
  if [ ! -f "${LIBSODIUM_TARBALL}" ]; then
    mkdir -p "${SCRIPT_DIR}/external"
    echo "Downloading ${LIBSODIUM_URL} -> ${LIBSODIUM_TARBALL}"
    curl -fL "${LIBSODIUM_URL}" -o "${LIBSODIUM_TARBALL}"
  fi

  # Verify SHA256 checksum before extracting
  CALC_SHA256=$(shasum -a 256 "${LIBSODIUM_TARBALL}" | awk '{print $1}')
  if [ "${CALC_SHA256}" != "${LIBSODIUM_SHA256}" ]; then
    echo "ERROR: SHA256 mismatch for ${LIBSODIUM_TARBALL}" >&2
    echo "Expected: ${LIBSODIUM_SHA256}" >&2
    echo "Actual:   ${CALC_SHA256}" >&2
    exit 1
  fi

  echo "Extracting ${LIBSODIUM_TARBALL}"
  tar -xzf "${LIBSODIUM_TARBALL}" -C "${SCRIPT_DIR}/external"
fi

# Build function for a given SDK/host/flags
build_variant() {
  local sdk="$1"; shift
  local host="$1"; shift
  local cflags="$1"; shift
  local install_path="$1"; shift
  local build_dir="$1"; shift

  mkdir -p "${build_dir}"
  mkdir -p "${install_path}"

  pushd "${LIBSODIUM_SRC_DIR}"
  
  if [ -f Makefile ]; then
    make distclean || true
  fi
  
  echo "Configuring libsodium for ${sdk} -> ${build_dir} (host=${host})"
  (
    cd "${build_dir}"
    CC="xcrun --sdk ${sdk} clang" \
    CFLAGS="${cflags}" \
    LDFLAGS="${cflags}" \
    "${LIBSODIUM_SRC_DIR}/configure" \
      --host="${host}" \
      --prefix="${install_path}" \
      --disable-shared --enable-static --with-pic
    make -j "$(getconf NPROCESSORS_ONLN)"
    make -j "$(getconf NPROCESSORS_ONLN)" install
  )

  popd
}

# Paths per-variant
INSTALL_IOS_DEV="${SCRIPT_DIR}/external/sodium-ios-arm64"
INSTALL_IOS_SIM="${SCRIPT_DIR}/external/sodium-iossim-arm64"
INSTALL_MACOS="${SCRIPT_DIR}/external/sodium-macos-arm64"

BUILD_IOS_DEV="${SCRIPT_DIR}/external/build-libsodium-ios-arm64"
BUILD_IOS_SIM="${SCRIPT_DIR}/external/build-libsodium-iossim-arm64"
BUILD_MACOS="${SCRIPT_DIR}/external/build-libsodium-macos-arm64"

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

# Remove existing XCFramework to avoid duplicate file errors on reruns
XCFRAMEWORK_PATH="${SCRIPT_DIR}/external/sodium.xcframework"
if [ -d "${XCFRAMEWORK_PATH}" ]; then
  echo "Removing existing ${XCFRAMEWORK_PATH}"
  rm -rf "${XCFRAMEWORK_PATH}"
fi

xcodebuild -create-xcframework \
  -library "${INSTALL_IOS_DEV}/lib/libsodium.a" \
  -headers "${INSTALL_IOS_DEV}/include" \
  -library "${INSTALL_IOS_SIM}/lib/libsodium.a" \
  -headers "${INSTALL_IOS_SIM}/include" \
  -library "${INSTALL_MACOS}/lib/libsodium.a" \
  -headers "${INSTALL_MACOS}/include" \
  -output "${XCFRAMEWORK_PATH}"
