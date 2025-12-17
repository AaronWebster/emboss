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

# -*- mode: python; -*-
# vim:set ft=blazebuild:
"""Build defs for Emboss Wireshark Lua dissector generation."""

load("//:build_defs.bzl", "EmbossInfo")

EmbossLuaDissectorInfo = provider(
    fields = {
        "dissectors": "(list[File]) The `.lua` dissector files from this rule.",
        "transitive_dissectors": "(list[File]) The `.lua` dissector files from this rule and all dependencies.",
    },
    doc = "Provide Lua dissector files.",
)

def _lua_emboss_aspect_impl(target, ctx):
    emboss_lua_compiler = ctx.executable._emboss_lua_compiler
    emboss_info = target[EmbossInfo]
    src = target[EmbossInfo].direct_source
    dissectors = [ctx.actions.declare_file(src.basename + ".lua", sibling = src)]
    args = ctx.actions.args()
    args.add("--input-file")
    args.add_all(emboss_info.direct_ir)
    args.add("--output-file")
    args.add_all(dissectors)
    ctx.actions.run(
        executable = emboss_lua_compiler,
        arguments = [args],
        inputs = emboss_info.direct_ir,
        outputs = dissectors,
    )
    transitive_dissectors = depset(
        direct = dissectors,
        transitive = [
            dep[EmbossLuaDissectorInfo].transitive_dissectors
            for dep in ctx.rule.attr.deps
        ],
    )
    return [
        EmbossLuaDissectorInfo(
            dissectors = depset(dissectors),
            transitive_dissectors = transitive_dissectors,
        ),
    ]

_lua_emboss_aspect = aspect(
    implementation = _lua_emboss_aspect_impl,
    attr_aspects = ["deps"],
    required_providers = [EmbossInfo],
    attrs = {
        "_emboss_lua_compiler": attr.label(
            executable = True,
            cfg = "exec",
            default = "@com_google_emboss//compiler/back_end/lua:emboss_codegen_lua",
        ),
    },
)

def _lua_emboss_library_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("`deps` attribute must contain exactly one label.", attr = "deps")
    dep = ctx.attr.deps[0]
    return [
        dep[EmbossInfo],
        DefaultInfo(files = dep[EmbossLuaDissectorInfo].dissectors),
    ]

lua_emboss_library = rule(
    implementation = _lua_emboss_library_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_lua_emboss_aspect],
            allow_rules = ["emboss_library"],
            allow_files = False,
        ),
    },
    provides = [EmbossInfo],
)

def emboss_lua_library(name, srcs, deps = [], import_dirs = [], **kwargs):
    """Constructs a Lua dissector library from an .emb file.
    
    Args:
        name: The name of the library.
        srcs: List of .emb source files (must be exactly one).
        deps: List of emboss_library dependencies.
        import_dirs: List of import directories.
        **kwargs: Additional arguments passed to the rules.
    """
    if len(srcs) != 1:
        fail(
            "Must specify exactly one Emboss source file for emboss_lua_library.",
            "srcs",
        )

    native.alias(
        name = name + "_ir_alias",
        actual = srcs[0].replace(".emb", "_ir") if ".emb" in srcs[0] else srcs[0] + "_ir",
        **kwargs
    )

    lua_emboss_library(
        name = name,
        deps = [":" + name + "_ir_alias"],
        **kwargs
    )
