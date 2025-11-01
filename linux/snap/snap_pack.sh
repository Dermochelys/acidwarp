#!/bin/bash
# Build snap package with SDL version from SDL_VERSION file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/../.."
SDL_VERSION_FILE="$REPO_ROOT/SDL_VERSION"
SNAPCRAFT_YAML="$SCRIPT_DIR/snapcraft.yaml"

# Check if SDL_VERSION file exists
if [ ! -f "$SDL_VERSION_FILE" ]; then
  echo "Error: SDL_VERSION file not found at $SDL_VERSION_FILE"
  exit 1
fi

# Read SDL version from file
SDL_VERSION=$(cat "$SDL_VERSION_FILE")

# Validate version format (should be like 3.2.26)
if ! [[ "$SDL_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Error: Invalid SDL version format in $SDL_VERSION_FILE: $SDL_VERSION"
  echo "Expected format: X.Y.Z (e.g., 3.2.26)"
  exit 1
fi

# Check if snapcraft.yaml exists
if [ ! -f "$SNAPCRAFT_YAML" ]; then
  echo "Error: snapcraft.yaml not found at $SNAPCRAFT_YAML"
  exit 1
fi

echo "Updating snapcraft.yaml to use SDL version: release-$SDL_VERSION"

# Update the source-tag line
# The line should be: "    source-tag: release-X.Y.Z"
sed -i.bak "s/^    source-tag: release-.*$/    source-tag: release-$SDL_VERSION/" "$SNAPCRAFT_YAML"

# Verify the change was made
if grep -q "source-tag: release-$SDL_VERSION" "$SNAPCRAFT_YAML"; then
  echo "✓ Successfully updated snapcraft.yaml"
  rm -f "$SNAPCRAFT_YAML.bak"
else
  echo "Error: Failed to update source-tag in snapcraft.yaml"
  mv "$SNAPCRAFT_YAML.bak" "$SNAPCRAFT_YAML"
  exit 1
fi

echo "Building snap package..."
cd "$SCRIPT_DIR"
snapcraft pack
