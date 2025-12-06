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

# Script to run black formatter to fix Python files.
# Usage: bazel run //:black_fix -- [args]
#
# Example: bazel run //:black_fix -- .

set -e

SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/black_common.sh"

# Find the black binary
find_black "$SCRIPT_DIR" || exit 1

# Change to the workspace directory (BUILD_WORKSPACE_DIRECTORY is set by bazel run)
if [[ -n "$BUILD_WORKSPACE_DIRECTORY" ]]; then
    cd "$BUILD_WORKSPACE_DIRECTORY"
fi

# Run black with the provided arguments
exec "$BLACK_BIN" "$@"
