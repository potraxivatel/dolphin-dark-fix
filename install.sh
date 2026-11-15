#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
make -C "$DIR"
make -C "$DIR" install

echo ""
echo "==> dolphin-dark-fix installed successfully!"
echo "==> Restart Dolphin to apply changes."
