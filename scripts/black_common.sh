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

# Shared functions for black formatter scripts.
# Source this file to use the find_black function.

# Find the black binary in the Bazel runfiles.
# Sets BLACK_BIN to the path of the black binary.
# Returns 0 on success, 1 on failure.
find_black() {
    local script_dir="$1"

    # Try to find black in different possible locations
    BLACK_BIN="$script_dir/black/black"
    if [[ -x "$BLACK_BIN" ]]; then
        return 0
    fi

    BLACK_BIN="$script_dir/../pip/bin/black/black"
    if [[ -x "$BLACK_BIN" ]]; then
        return 0
    fi

    # Search for black in runfiles
    if [[ -n "$RUNFILES_DIR" ]]; then
        BLACK_BIN="$(find "$RUNFILES_DIR" -name "black" -type f -executable 2>/dev/null | head -1)"
        if [[ -n "$BLACK_BIN" ]] && [[ -x "$BLACK_BIN" ]]; then
            return 0
        fi
    fi

    # Could not find black
    echo "Error: Could not find black binary" >&2
    echo "Searched in:" >&2
    echo "  - $script_dir/black/black" >&2
    echo "  - $script_dir/../pip/bin/black/black" >&2
    echo "  - RUNFILES_DIR=$RUNFILES_DIR" >&2
    return 1
}
