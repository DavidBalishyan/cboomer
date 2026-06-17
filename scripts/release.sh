#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "help" ]; then
    echo "Usage: $0 <version>"
    echo "Tag and release a new version of cboomer."
    echo ""
    echo "Arguments:"
    echo "  version    Tag in the form vMAJOR.MINOR.PATCH (e.g. v1.3.0)"
    echo ""
    echo "Steps:"
    echo "  1. Verifies the working tree is clean"
    echo "  2. Replaces the 'commit: [TODO]' placeholder in CHANGELOG.md"
    echo "     with the current commit hash and commits the change"
    echo "  3. Creates an annotated git tag"
    echo "  4. Pushes the commit and tag to origin"
    echo "  5. Creates a GitHub release (requires gh CLI)"
    exit 0
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 v1.3.0"
    echo "Run '$0 help' for details."
    exit 1
fi

VERSION="$1"

if ! echo "$VERSION" | grep -qE '^v[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Error: version must be in the form vMAJOR.MINOR.PATCH (e.g. v1.3.0)"
    exit 1
fi

if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Error: working tree is not clean. Commit or stash your changes first."
    exit 1
fi

COMMIT="$(git rev-parse HEAD)"
SHORT="${COMMIT:0:7}"

echo "Tagging $SHORT as $VERSION"

CHANGELOG="CHANGELOG.md"

if grep -q 'commit: \[TODO\]' "$CHANGELOG"; then
    sed -i "s|commit: \[TODO\]|commit: [$SHORT](https://github.com/DavidBalishyan/cboomer/commit/$COMMIT)|" "$CHANGELOG"
    git add "$CHANGELOG"
    git commit -m "Add commit for $VERSION in CHANGELOG"
else
    echo "Warning: no 'commit: [TODO]' placeholder found in $CHANGELOG, skipping update"
fi

git tag -a "$VERSION" -m "$VERSION"

git push origin HEAD
git push origin "$VERSION"

if command -v gh >/dev/null 2>&1; then
    gh release create "$VERSION" --generate-notes
else
    echo "gh not found. Create the release manually at:"
    echo "  https://github.com/DavidBalishyan/cboomer/releases/new?tag=$VERSION"
fi
