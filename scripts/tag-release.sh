#!/usr/bin/env bash
# Helper script to tag releases and trigger container builds
# Usage: ./scripts/tag-release.sh v1.2.3

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 v1.2.3"
    exit 1
fi

VERSION=$1

# Validate version format
if ! [[ $VERSION =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version must be in format v1.2.3"
    exit 1
fi

# Create and push tag
git tag -a "$VERSION" -m "Release $VERSION"
git push origin "$VERSION"

echo "Tagged and pushed $VERSION"
echo "Container builds will start automatically"
echo
echo "Container images will be available at:"
echo "  ghcr.io/compiler-research/cppinterop:$VERSION"
echo "  ghcr.io/compiler-research/cppinterop:latest"
echo "  ghcr.io/compiler-research/cppinterop:cuda"