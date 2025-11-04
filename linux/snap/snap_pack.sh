#!/bin/bash
# Build snap package with SDL version from SDL_VERSION and SDL3_IMAGE_VERSION files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/../.."
SDL_VERSION_FILE="$REPO_ROOT/SDL_VERSION"
SDL3_IMAGE_VERSION_FILE="$REPO_ROOT/SDL3_IMAGE_VERSION"
SNAPCRAFT_YAML="$SCRIPT_DIR/snapcraft.yaml"

# Check if SDL_VERSION file exists
if [ ! -f "$SDL_VERSION_FILE" ]; then
  echo "Error: SDL_VERSION file not found at $SDL_VERSION_FILE"
  exit 1
fi

# Check if SDL3_IMAGE_VERSION file exists
if [ ! -f "$SDL3_IMAGE_VERSION_FILE" ]; then
  echo "Error: SDL3_IMAGE_VERSION file not found at $SDL3_IMAGE_VERSION_FILE"
  exit 1
fi

# Read SDL version from file
SDL_VERSION=$(cat "$SDL_VERSION_FILE")

# Read SDL3_image version from file
SDL3_IMAGE_VERSION=$(cat "$SDL3_IMAGE_VERSION_FILE")

# Validate SDL version format (should be like 3.2.26)
if ! [[ "$SDL_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Error: Invalid SDL version format in $SDL_VERSION_FILE: $SDL_VERSION"
  echo "Expected format: X.Y.Z (e.g., 3.2.26)"
  exit 1
fi

# Validate SDL3_image version format (should be like 3.2.4)
if ! [[ "$SDL3_IMAGE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "Error: Invalid SDL3_image version format in $SDL3_IMAGE_VERSION_FILE: $SDL3_IMAGE_VERSION"
  echo "Expected format: X.Y.Z (e.g., 3.2.4)"
  exit 1
fi

# Check if snapcraft.yaml exists
if [ ! -f "$SNAPCRAFT_YAML" ]; then
  echo "Error: snapcraft.yaml not found at $SNAPCRAFT_YAML"
  exit 1
fi

# Extract current versions from snapcraft.yaml
CURRENT_SDL_VERSION=$(awk '/^  sdl3:/{flag=1; next} /^  sdl3-image:/{flag=0} flag && /^    source-tag:/{gsub(/.*release-/, ""); print; exit}' "$SNAPCRAFT_YAML")
CURRENT_SDL3_IMAGE_VERSION=$(awk '/^  sdl3-image:/{flag=1; next} /^  acidwarp:/{flag=0} flag && /^    source-tag:/{gsub(/.*release-/, ""); print; exit}' "$SNAPCRAFT_YAML")

echo "=== Version Check ==="
echo "Target SDL3 version:       release-$SDL_VERSION"
echo "Current SDL3 version:      release-$CURRENT_SDL_VERSION"
echo ""
echo "Target SDL3_image version: release-$SDL3_IMAGE_VERSION"
echo "Current SDL3_image version: release-$CURRENT_SDL3_IMAGE_VERSION"
echo ""

# Determine if updates are needed
SDL_NEEDS_UPDATE=0
SDL3_IMAGE_NEEDS_UPDATE=0

if [ "$SDL_VERSION" != "$CURRENT_SDL_VERSION" ]; then
  SDL_NEEDS_UPDATE=1
fi

if [ "$SDL3_IMAGE_VERSION" != "$CURRENT_SDL3_IMAGE_VERSION" ]; then
  SDL3_IMAGE_NEEDS_UPDATE=1
fi

if [ $SDL_NEEDS_UPDATE -eq 0 ] && [ $SDL3_IMAGE_NEEDS_UPDATE -eq 0 ]; then
  echo "✓ All versions already up-to-date in snapcraft.yaml"
else
  echo "Updating snapcraft.yaml..."
  # Create a temporary copy for editing
  cp "$SNAPCRAFT_YAML" "$SNAPCRAFT_YAML.bak"
fi

# Perform updates only if needed
if [ $SDL_NEEDS_UPDATE -eq 1 ] || [ $SDL3_IMAGE_NEEDS_UPDATE -eq 1 ]; then
  # Update the SDL3 source-tag line (in the sdl3: section)
  # We need to update the first occurrence (SDL3) and the second occurrence (SDL3_image) differently

  awk -v sdl_ver="$SDL_VERSION" -v img_ver="$SDL3_IMAGE_VERSION" '
    /^  sdl3:/ { in_sdl3=1; in_sdl3_image=0; print; next }
    /^  sdl3-image:/ { in_sdl3=0; in_sdl3_image=1; print; next }
    /^  acidwarp:/ { in_sdl3=0; in_sdl3_image=0; print; next }
    /^    source-tag:/ {
      if (in_sdl3) {
        printf "    source-tag: release-%s\n", sdl_ver
        next
      }
      if (in_sdl3_image) {
        printf "    source-tag: release-%s\n", img_ver
        next
      }
    }
    { print }
  ' "$SNAPCRAFT_YAML.bak" > "$SNAPCRAFT_YAML"

  # Verify the changes were made
  if grep -q "source-tag: release-$SDL_VERSION" "$SNAPCRAFT_YAML" && \
     grep -q "source-tag: release-$SDL3_IMAGE_VERSION" "$SNAPCRAFT_YAML"; then

    # Show what was updated
    if [ $SDL_NEEDS_UPDATE -eq 1 ]; then
      echo "  ✓ Updated SDL3: release-$CURRENT_SDL_VERSION → release-$SDL_VERSION"
    fi
    if [ $SDL3_IMAGE_NEEDS_UPDATE -eq 1 ]; then
      echo "  ✓ Updated SDL3_image: release-$CURRENT_SDL3_IMAGE_VERSION → release-$SDL3_IMAGE_VERSION"
    fi

    rm -f "$SNAPCRAFT_YAML.bak"
  else
    echo "Error: Failed to update source-tag in snapcraft.yaml"
    mv "$SNAPCRAFT_YAML.bak" "$SNAPCRAFT_YAML"
    exit 1
  fi
fi

echo ""

echo "Building snap package..."
cd "$SCRIPT_DIR"
snapcraft pack
