#!/bin/bash
set -e

# Build SDL3 from latest main branch
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Target location for SDL3.xcframework (repo root)
TARGET_FRAMEWORK="$SCRIPT_DIR/SDL3.xcframework"

echo "Checking for SDL3.xcframework..."

# Check if framework already exists
if [ -d "$TARGET_FRAMEWORK" ]; then
    echo "SDL3.xcframework found."

    # Check if running in interactive mode (terminal)
    if [ -t 0 ]; then
        # Interactive mode - ask user
        read -p "Rebuild SDL3 from latest main branch? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Using existing SDL3.xcframework"
            exit 0
        fi
    else
        # Non-interactive mode (Xcode build) - use existing framework
        echo "Using existing SDL3.xcframework (non-interactive mode)"
        exit 0
    fi

    echo "Removing old framework..."
    rm -rf "$TARGET_FRAMEWORK"
fi

echo "Building SDL3 from latest main branch for iOS using Xcode..."

# Create temp directory
TEMP_DIR=$(mktemp -d)

# Cleanup function
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

cd "$TEMP_DIR"

# Clone SDL (latest main branch)
echo "Cloning SDL from main branch..."
git clone --depth 1 https://github.com/libsdl-org/SDL.git
cd SDL
COMMIT_HASH=$(git rev-parse --short HEAD)
echo "Using SDL commit $COMMIT_HASH from main branch"

# Disable code signing in the SDL Xcode project
echo "Configuring Xcode project settings..."
python3 <<'PYTHON_SCRIPT'
import sys
import re

project_file = "Xcode/SDL/SDL.xcodeproj/project.pbxproj"

# Read the project file
with open(project_file, 'r') as f:
    content = f.read()

# Remove ProvisioningStyle attributes
content = re.sub(r'ProvisioningStyle = [^;]+;', '', content)

# Remove or disable code signing settings
content = re.sub(r'CODE_SIGN_IDENTITY = "[^"]*";', 'CODE_SIGN_IDENTITY = "";', content)
content = re.sub(r'CODE_SIGN_IDENTITY = [^;"\n]+;', 'CODE_SIGN_IDENTITY = "";', content)
content = re.sub(r'"CODE_SIGN_IDENTITY\[sdk=[^\]]+\]" = "[^"]*";', '', content)

# Disable code signing requirements
content = re.sub(r'CODE_SIGNING_REQUIRED = YES;', 'CODE_SIGNING_REQUIRED = NO;', content)
content = re.sub(r'CODE_SIGNING_ALLOWED = YES;', 'CODE_SIGNING_ALLOWED = NO;', content)

# Add CODE_SIGNING_REQUIRED = NO if not present
if 'CODE_SIGNING_REQUIRED' not in content:
    content = re.sub(
        r'(buildSettings = \{)',
        r'\1\n\t\t\t\tCODE_SIGNING_REQUIRED = NO;',
        content,
        count=1
    )

# Clear development team
content = re.sub(r'DEVELOPMENT_TEAM = "[^"]*";', 'DEVELOPMENT_TEAM = "";', content)
content = re.sub(r'DEVELOPMENT_TEAM = [^;"\n]+;', 'DEVELOPMENT_TEAM = "";', content)
content = re.sub(r'DevelopmentTeam = [^;]+;', '', content)

# Code signing style
content = re.sub(r'CODE_SIGN_STYLE = [^;]+;', 'CODE_SIGN_STYLE = Manual;', content)

# Fix iOS deployment target to 15.6
content = re.sub(r'IPHONEOS_DEPLOYMENT_TARGET = [^;]+;', 'IPHONEOS_DEPLOYMENT_TARGET = 15.6;', content)

# Fix headermap settings
content = re.sub(r'ALWAYS_SEARCH_USER_PATHS = [^;]+;', 'ALWAYS_SEARCH_USER_PATHS = NO;', content)
content = re.sub(r'USE_HEADERMAP = [^;]+;', 'USE_HEADERMAP = NO;', content)

# Write back
with open(project_file, 'w') as f:
    f.write(content)

print("Configured Xcode project: disabled code signing, updated deployment target to 15.6, fixed headermap settings")
PYTHON_SCRIPT

# Build using the SDL3.xcframework scheme
echo "Building SDL3.xcframework using built-in scheme..."

# Unset conflicting deployment target variables
unset MACOSX_DEPLOYMENT_TARGET
unset IPHONEOS_DEPLOYMENT_TARGET
unset TVOS_DEPLOYMENT_TARGET
unset WATCHOS_DEPLOYMENT_TARGET

# Build the xcframework directly using the SDL3.xcframework scheme
echo "Note: 'no debug symbols' warnings are expected and harmless for Release builds"
xcodebuild build \
    -project Xcode/SDL/SDL.xcodeproj \
    -scheme SDL3.xcframework \
    -configuration Release \
    CODE_SIGN_IDENTITY="" \
    CODE_SIGNING_REQUIRED=NO \
    CODE_SIGNING_ALLOWED=NO \
    DEVELOPMENT_TEAM="" \
    IPHONEOS_DEPLOYMENT_TARGET=15.6 \
    MACOSX_DEPLOYMENT_TARGET=10.15 \
    ALWAYS_SEARCH_USER_PATHS=NO \
    USE_HEADERMAP=NO 2>&1 | grep -v "warning: no debug symbols" || true

# Find and copy built xcframework
echo "Locating built xcframework..."

# The xcframework scheme typically outputs to build/Release or similar
POSSIBLE_PATHS=(
    "build/Release/SDL3.xcframework"
    "build/SDL3.xcframework"
    "Xcode/SDL/build/Release/SDL3.xcframework"
    "Xcode/SDL/build/SDL3.xcframework"
)

FOUND=false
for path in "${POSSIBLE_PATHS[@]}"; do
    if [ -d "$path" ]; then
        echo "Found SDL3.xcframework at: $path"
        cp -R "$path" "$TARGET_FRAMEWORK"
        FOUND=true
        break
    fi
done

if [ "$FOUND" = false ]; then
    echo "Error: SDL3.xcframework not found after build"
    echo "Searched in:"
    for path in "${POSSIBLE_PATHS[@]}"; do
        echo "  - $path"
    done
    echo ""
    echo "Directory contents:"
    find . -name "SDL3.xcframework" -type d 2>/dev/null || echo "No SDL3.xcframework found anywhere"
    exit 1
fi

echo "SDL3.xcframework built successfully from main branch commit $COMMIT_HASH!"
