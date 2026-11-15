#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
make -C "$DIR" uninstall
