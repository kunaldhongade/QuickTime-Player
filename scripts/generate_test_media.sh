#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_dir="${1:-$repo_dir/test-media}"
mkdir -p "$output_dir"

ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=24 -frames:v 48 \
    -c:v libx264 -pix_fmt yuv420p -y "$output_dir/cfr-24-48.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=30 -frames:v 60 \
    -c:v libx264 -pix_fmt yuv420p -y "$output_dir/cfr-30-60.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=60000/1001 -frames:v 120 \
    -c:v libx264 -pix_fmt yuv420p -y "$output_dir/cfr-5994-120.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=24 -frames:v 72 \
    -c:v libx264 -bf 3 -g 24 -pix_fmt yuv420p -y "$output_dir/h264-bframes.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=30 -frames:v 60 \
    -vf "select='not(mod(n,3)) + not(mod(n,7))',setpts=N/24/TB" -fps_mode vfr \
    -c:v libx264 -pix_fmt yuv420p -y "$output_dir/vfr.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=24 -frames:v 24 \
    -vf "setpts=PTS+5/TB" -c:v libx264 -pix_fmt yuv420p -y "$output_dir/nonzero-start.mp4"
ffmpeg -v error -f lavfi -i color=c=navy:size=320x180:rate=1 -frames:v 1 \
    -c:v libx264 -pix_fmt yuv420p -y "$output_dir/one-frame.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=24 -frames:v 48 \
    -c:v libx264 -pix_fmt yuv420p -an -y "$output_dir/silent.mp4"
ffmpeg -v error -f lavfi -i sine=frequency=440:duration=2 -c:a aac \
    -y "$output_dir/audio-only.m4a"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=24 -frames:v 24 \
    -c:v libx264 -pix_fmt yuv420p -metadata:s:v:0 rotate=90 \
    -y "$output_dir/rotation-90.mp4"
ffmpeg -v error -f lavfi -i testsrc2=size=320x180:rate=24 -frames:v 24 \
    -vf "setpts='floor(N/2)/(12*TB)'" -fps_mode vfr -c:v ffv1 \
    -y "$output_dir/unusual-timestamps.mkv"
cp "$output_dir/cfr-24-48.mp4" "$output_dir/truncated.mp4"
truncate -s 2048 "$output_dir/truncated.mp4"

echo "Generated deterministic fixtures in $output_dir"
