#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="$repo_dir/dist"

if ! command -v docker >/dev/null 2>&1; then
    echo "Docker is required to build the Ubuntu package." >&2
    exit 1
fi
if ! docker info >/dev/null 2>&1; then
    echo "The Docker runtime is not running." >&2
    exit 1
fi

mkdir -p "$output_dir"
buildx_command=(docker buildx)
if ! docker buildx version >/dev/null 2>&1; then
    if [[ -x /opt/homebrew/lib/docker/cli-plugins/docker-buildx ]]; then
        buildx_command=(/opt/homebrew/lib/docker/cli-plugins/docker-buildx)
    else
        echo "Docker Buildx is required to export the package artifact." >&2
        exit 1
    fi
fi

"${buildx_command[@]}" build \
    --platform linux/amd64 \
    --file "$repo_dir/packaging/linux/Dockerfile.ubuntu24.04" \
    --target artifact \
    --output "type=local,dest=$output_dir" \
    "$repo_dir"

echo "Ubuntu package written to $output_dir"
