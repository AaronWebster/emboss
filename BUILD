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

load("@pip//:requirements.bzl", "entry_point")

exports_files([
    "build_defs.bzl",
    "LICENSE",
    "requirements.txt",
])

# Black formatter binary. Use with:
#   bazel run //:black -- [args]
# Example to format all Python files:
#   bazel run //:black -- .
alias(
    name = "black",
    actual = entry_point("black"),
)

# Target to fix Python formatting issues.
# Usage: bazel run //:black_fix -- .
sh_binary(
    name = "black_fix",
    srcs = ["scripts/black_fix.sh"],
    data = [
        ":black",
        "scripts/black_common.sh",
    ],
)

# Target to check Python formatting.
# Usage: bazel run //:black_check
# This runs black --check --diff . on the workspace.
sh_binary(
    name = "black_check",
    srcs = ["scripts/black_check.sh"],
    data = [
        ":black",
        "scripts/black_common.sh",
    ],
)
