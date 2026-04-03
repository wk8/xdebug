#!/bin/sh

set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
DOCKERFILE="$ROOT/phuck_off_tests/e2e/Dockerfile"

if [ "${PHUCK_OFF_DOCKER_PLATFORM:-}" != "" ]; then
    IMAGE_ID="$(docker build --platform "$PHUCK_OFF_DOCKER_PLATFORM" -q -f "$DOCKERFILE" "$ROOT")"
    docker run --rm --platform "$PHUCK_OFF_DOCKER_PLATFORM" "$IMAGE_ID"
else
    IMAGE_ID="$(docker build -q -f "$DOCKERFILE" "$ROOT")"
    docker run --rm "$IMAGE_ID"
fi
