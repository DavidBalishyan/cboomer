#!/usr/bin/env bash
set -euo pipefail

DRY_RUN=0

case "${1:-}" in
    help)
        echo "Usage: $0 [--dry-run] <version>"
        echo "Tag and release a new version of cboomer."
        echo ""
        echo "Arguments:"
        echo "  version    Tag in the form vMAJOR.MINOR.PATCH (e.g. v1.3.0)"
        echo ""
        echo "Options:"
        echo "  --dry-run  Print what would be done without making changes"
        echo ""
        echo "Steps:"
        echo "  1. Verifies the working tree is clean"
        echo "  2. Replaces the 'commit: [TODO]' placeholder in CHANGELOG.md"
        echo "     with the current commit hash and commits the change"
        echo "  3. Creates an annotated git tag"
        echo "  4. Pushes the commit and tag to origin"
        echo "  5. Creates a GitHub release using the CHANGELOG section as notes"
        exit 0
        ;;
    --dry-run)
        DRY_RUN=1
        shift
        ;;
esac

if [ $# -ne 1 ]; then
    echo "Usage: $0 [--dry-run] <version>"
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
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "[DRY RUN] would replace 'commit: [TODO]' in $CHANGELOG with commit $SHORT"
        echo "[DRY RUN] would commit and tag $VERSION"
    else
        sed -i "s|commit: \[TODO\]|commit: [$SHORT](https://github.com/DavidBalishyan/cboomer/commit/$COMMIT)|" "$CHANGELOG"
        git add "$CHANGELOG"
        git commit -m "Add commit for $VERSION in CHANGELOG"
        git tag -a "$VERSION" -m "$VERSION"
        git push origin HEAD
        git push origin "$VERSION"
    fi
else
    echo "Warning: no 'commit: [TODO]' placeholder found in $CHANGELOG, skipping update"
fi

NOTES_FILE=$(mktemp)
trap 'rm -f "$NOTES_FILE"' EXIT

awk -v ver="$VERSION" '
    /^# / {
        if (found) exit
        if ($0 == "# " ver) found = 1
        next
    }
    found { print }
' "$CHANGELOG" > "$NOTES_FILE"

if [ "$DRY_RUN" -eq 1 ]; then
    echo "[DRY RUN] release notes would be:"
    echo "---"
    cat "$NOTES_FILE"
    echo "---"
    echo "[DRY RUN] would run: gh release create $VERSION --notes-file <notes>"
elif command -v gh >/dev/null 2>&1; then
    if [ -s "$NOTES_FILE" ]; then
        gh release create "$VERSION" --notes-file "$NOTES_FILE"
    else
        echo "Warning: no changelog section found for $VERSION, using generated notes"
        gh release create "$VERSION" --generate-notes
    fi
else
    echo "gh not found. Create the release manually at:"
    echo "  https://github.com/DavidBalishyan/cboomer/releases/new?tag=$VERSION"
fi
