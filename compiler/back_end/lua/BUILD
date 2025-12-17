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

# Emboss Wireshark Lua dissector code generator.

load("@rules_python//python:py_binary.bzl", "py_binary")
load("@rules_python//python:py_library.bzl", "py_library")
load("@rules_python//python:py_test.bzl", "py_test")

package(
    default_visibility = [
        "//visibility:private",
    ],
)

py_binary(
    name = "emboss_codegen_lua",
    srcs = ["emboss_codegen_lua.py"],
    python_version = "PY3",
    visibility = ["//visibility:public"],
    deps = [
        ":dissector_generator",
        "//compiler/util:ir_data",
    ],
)

py_library(
    name = "dissector_generator",
    srcs = ["dissector_generator.py"],
    deps = [
        "//compiler/util:ir_data",
        "//compiler/util:ir_util",
    ],
)

py_test(
    name = "dissector_generator_test",
    srcs = ["dissector_generator_test.py"],
    deps = [
        ":dissector_generator",
        "//compiler/util:ir_data",
    ],
)
