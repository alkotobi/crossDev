#!/bin/sh
# Configure and build CrossDev. Usage: ./build.sh <build_folder> <app_name>
# Example: ./build.sh build_mac CrossDev
#          ./build.sh build_cars Cars

set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

BUILD_DIR="${1:-build}"
APP_NAME="${2:-CrossDev}"

if [ -z "$BUILD_DIR" ] || [ -z "$APP_NAME" ]; then
    echo "Usage: $0 <build_folder> <app_name>"
    echo "  build_folder  e.g. build, build_mac, build_cars"
    echo "  app_name      e.g. CrossDev, Cars (used for .../CrossDev/<app_name>/options.json)"
    echo "Example: $0 build_mac Cars"
    exit 1
fi

echo "Config: build_dir=$BUILD_DIR app_name=$APP_NAME"
cmake -S . -B "$BUILD_DIR" -DCROSSDEV_APP_NAME="$APP_NAME"
cmake --build "$BUILD_DIR"
echo "Done: $BUILD_DIR"
