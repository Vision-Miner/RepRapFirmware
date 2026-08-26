#!/usr/bin/env bash
#
# check-version.sh — assert that src/Version.h matches a release tag.
#
# This is the guard that keeps the version line honest: a tag may only exist for
# a commit whose MAIN_VERSION says the same thing. Run by release-build and by
# the release workflow before anything is compiled.
#
#   check-version.sh [tag]
#
# The tag defaults to $GITHUB_REF_NAME when set (a tag-triggered CI run), then to
# the tag on HEAD. Exit status: 0 match, 1 mismatch, 2 nothing to compare.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if ! RRF_ROOT="$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel 2>/dev/null)"; then
	RRF_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
fi
# shellcheck source=/dev/null
. "${SCRIPT_DIR}/repos.conf"

version_in_header() {
	sed -n 's/^# *define[[:space:]]*MAIN_VERSION[[:space:]]*"\(.*\)".*/\1/p' "${RRF_ROOT}/src/Version.h"
}

tag="${1:-${GITHUB_REF_NAME:-}}"
if [ -z "$tag" ]; then
	tag="$(git -C "$RRF_ROOT" describe --tags --exact-match HEAD 2>/dev/null || true)"
fi
if [ -z "$tag" ]; then
	printf 'check-version: no tag given and HEAD is not tagged\n' >&2
	exit 2
fi

header="$(version_in_header)"
if [ -z "$header" ]; then
	printf 'check-version: could not read MAIN_VERSION from %s\n' "${RRF_ROOT}/src/Version.h" >&2
	exit 2
fi

expected="${tag#"$VM_TAG_PREFIX"}"
if [ "$header" = "$expected" ]; then
	printf 'check-version: OK — tag %s matches MAIN_VERSION "%s"\n' "$tag" "$header"
	exit 0
fi

printf 'check-version: MISMATCH\n  tag          %s (expects MAIN_VERSION "%s")\n  Version.h    "%s"\n' \
	"$tag" "$expected" "$header" >&2
exit 1
