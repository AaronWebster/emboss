#!/bin/bash
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Script to check Python formatting with black.
# Usage: bazel run //:black_check
#
# This will pass if all files are correctly formatted and fail otherwise,
# printing a diff of the required changes.

set -e

# Find the black binary in the runfiles
SCRIPT_DIR="$(dirname "$0")"
BLACK_BIN="$SCRIPT_DIR/black/black"

# Try to find black in different possible locations
if [[ ! -x "$BLACK_BIN" ]]; then
    BLACK_BIN="$SCRIPT_DIR/../pip/bin/black/black"
fi

if [[ ! -x "$BLACK_BIN" ]]; then
    # Search for black in runfiles
    BLACK_BIN="$(find "$RUNFILES_DIR" -name "black" -type f -executable 2>/dev/null | head -1)"
fi

if [[ -z "$BLACK_BIN" ]] || [[ ! -x "$BLACK_BIN" ]]; then
    echo "Error: Could not find black binary" >&2
    echo "Looked in: $SCRIPT_DIR/black/black" >&2
    exit 1
fi

# Change to the workspace directory (BUILD_WORKSPACE_DIRECTORY is set by bazel run)
if [[ -n "$BUILD_WORKSPACE_DIRECTORY" ]]; then
    cd "$BUILD_WORKSPACE_DIRECTORY"
fi

# Run black in check mode
exec "$BLACK_BIN" --check --diff .
