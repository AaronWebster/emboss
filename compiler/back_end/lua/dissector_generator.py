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

"""Wireshark Lua dissector code generator.

Generates Lua dissector code for Wireshark from Emboss IR.
"""

from typing import List, Optional, Tuple
from compiler.util import error
from compiler.util import ir_data
from compiler.util import ir_data_utils
from compiler.util import ir_util


def _get_documentation_text(documentation: List[ir_data.Documentation]) -> str:
    """Extract documentation text from documentation list.
    
    Only processes double-dash comments, not hash comments.
    """
    if not documentation:
        return ""
    
    doc_parts = []
    for doc in documentation:
        if doc.text:
            # Documentation text is already extracted from double-dash comments
            # by the parser
            doc_parts.append(doc.text.strip())
    
    return " ".join(doc_parts)


def _get_filter_attribute(attributes: List[ir_data.Attribute]) -> Optional[str]:
    """Get the wireshark_filter attribute value if present."""
    attr_value = ir_util.get_attribute(attributes, "wireshark_filter")
    if attr_value and attr_value.string_constant:
        return attr_value.string_constant.text
    return None


def _sanitize_lua_identifier(name: str) -> str:
    """Sanitize a name to be a valid Lua identifier."""
    # Replace invalid characters with underscores
    sanitized = ""
    for char in name:
        if char.isalnum() or char == "_":
            sanitized += char
        else:
            sanitized += "_"
    
    # Ensure it doesn't start with a number
    if sanitized and sanitized[0].isdigit():
        sanitized = "_" + sanitized
    
    return sanitized


def _get_lua_type_for_field(field: ir_data.Field, ir: ir_data.EmbossIr) -> Optional[str]:
    """Get the Wireshark Lua type for a field."""
    if not field.type:
        return None
    
    if field.type.atomic_type:
        atomic = field.type.atomic_type
        if atomic.reference:
            # Find the referenced type
            referenced_type = ir_util.find_object(atomic.reference, ir)
            if referenced_type:
                type_name = referenced_type.name.canonical_name.object_path[-1]
                # Check if it's a built-in type
                if type_name in ["UInt"]:
                    return "uint"
                elif type_name in ["Int"]:
                    return "int"
                # Otherwise it's likely an enum or custom type
                return None
        
    return None


def _generate_enum_value_string(enum: ir_data.Enum, type_name: str) -> str:
    """Generate Lua code for enum value strings."""
    lines = []
    lines.append(f"local {type_name}_values = {{")
    
    for value in enum.value:
        if value.name and value.value:
            value_name = value.name.canonical_name.object_path[-1]
            # Get numeric value using ir_util
            numeric_value = ir_util.constant_value(value.value)
            if numeric_value is not None:
                lines.append(f"  [{numeric_value}] = \"{value_name}\",")
    
    lines.append("}")
    return "\n".join(lines)


def _generate_field_dissector(field: ir_data.Field, 
                               parent_filter: str,
                               ir: ir_data.EmbossIr,
                               offset_expr: str = "offset",
                               indent: str = "  ") -> Tuple[List[str], List[str]]:
    """Generate Lua code to dissect a field.
    
    Returns:
        Tuple of (field_declarations, dissector_code_lines)
    """
    if not field.name:
        return ([], [])
    
    field_name = field.name.canonical_name.object_path[-1]
    sanitized_name = _sanitize_lua_identifier(field_name)
    
    # Get filter name from attribute or construct from parent
    filter_name = _get_filter_attribute(field.attribute)
    if not filter_name:
        filter_name = f"{parent_filter}.{sanitized_name}"
    
    field_declarations = []
    dissector_code = []
    
    # Get field documentation
    doc = _get_documentation_text(field.documentation)
    
    # Check if this is a structure or enum field
    is_struct = False
    is_enum = False
    enum_value_table = None
    
    if field.type and field.type.atomic_type and field.type.atomic_type.reference:
        # Find the referenced type
        referenced_type = ir_util.find_object(field.type.atomic_type.reference, ir)
        if referenced_type:
            if referenced_type.enumeration:
                is_enum = True
                type_name = referenced_type.name.canonical_name.object_path[-1]
                enum_value_table = f"{type_name}_values"
            elif referenced_type.structure:
                is_struct = True
    
    # Get size
    size_expr = None
    if field.location and field.location.size:
        # Try to get constant size using ir_util
        size_value = ir_util.constant_value(field.location.size)
        if size_value is not None:
            size_expr = str(size_value)
    
    if not is_struct and size_expr:
        # Generate ProtoField declaration
        lua_type = _get_lua_type_for_field(field, ir)
        if lua_type or is_enum:
            size_bits = int(size_expr) if size_expr else 0
            size_bytes = (size_bits + 7) // 8
            
            # Determine appropriate Wireshark field type based on size
            if is_enum or lua_type == "uint":
                if size_bytes <= 1:
                    ws_type = "uint8"
                elif size_bytes <= 2:
                    ws_type = "uint16"
                elif size_bytes <= 4:
                    ws_type = "uint32"
                else:
                    ws_type = "uint64"
            elif lua_type == "int":
                if size_bytes <= 1:
                    ws_type = "int8"
                elif size_bytes <= 2:
                    ws_type = "int16"
                elif size_bytes <= 4:
                    ws_type = "int32"
                else:
                    ws_type = "int64"
            else:
                ws_type = "bytes"
            
            desc = f'"{field_name}"'
            if doc:
                desc = f'"{field_name} - {doc}"'
            
            # Build field declaration with enum value table if applicable
            if is_enum and enum_value_table:
                field_declarations.append(
                    f'ProtoField.{ws_type}("{filter_name}", {desc}, base.DEC, {enum_value_table})'
                )
            else:
                field_declarations.append(
                    f'ProtoField.{ws_type}("{filter_name}", {desc})'
                )
            
            # Generate dissector code
            dissector_code.append(f"{indent}-- {field_name}")
            if doc:
                dissector_code.append(f"{indent}-- {doc}")
            dissector_code.append(
                f"{indent}subtree:add(fields.{sanitized_name}, buffer({offset_expr}, {size_bytes}))"
            )
            dissector_code.append(f"{indent}{offset_expr} = {offset_expr} + {size_bytes}")
    
    return (field_declarations, dissector_code)


def _generate_struct_dissector(struct: ir_data.Structure,
                                type_def: ir_data.TypeDefinition,
                                protocol_name: str,
                                ir: ir_data.EmbossIr) -> Tuple[List[str], List[str]]:
    """Generate Lua code to dissect a structure.
    
    Returns:
        Tuple of (field_declarations, dissector_function_lines)
    """
    struct_name = type_def.name.canonical_name.object_path[-1] if type_def.name else "Unknown"
    
    # Get filter prefix from attribute or use protocol name
    filter_prefix = _get_filter_attribute(type_def.attribute)
    if not filter_prefix:
        filter_prefix = protocol_name.lower()
    
    field_declarations = []
    dissector_lines = []
    
    # Generate fields
    for field in struct.field:
        field_decls, field_code = _generate_field_dissector(
            field, filter_prefix, ir, "offset", "    "
        )
        field_declarations.extend(field_decls)
        dissector_lines.extend(field_code)
    
    return (field_declarations, dissector_lines)


def generate_dissector(ir: ir_data.EmbossIr, protocol_name: Optional[str] = None) -> Tuple[str, List]:
    """Generate a Wireshark Lua dissector from Emboss IR.
    
    Args:
        ir: The Emboss IR to generate code from
        protocol_name: Optional protocol name override
        
    Returns:
        Tuple of (generated_code, errors)
    """
    errors = []
    
    if not ir.module:
        errors.append([error.error("", ir.source_location, "No modules in IR")])
        return ("", errors)
    
    # Get the main module (first one)
    main_module = ir.module[0]
    
    # Determine protocol name
    if not protocol_name:
        if main_module.source_file_name:
            # Use source file name without extension
            protocol_name = main_module.source_file_name.replace(".emb", "")
            # Remove path components
            if "/" in protocol_name:
                protocol_name = protocol_name.split("/")[-1]
        else:
            protocol_name = "emboss"
    
    protocol_name_sanitized = _sanitize_lua_identifier(protocol_name)
    
    lines = []
    
    # Header
    lines.append("-- Wireshark Lua dissector generated by Emboss")
    lines.append("-- DO NOT EDIT")
    lines.append("")
    
    # Module documentation
    if main_module.documentation:
        doc = _get_documentation_text(main_module.documentation)
        if doc:
            lines.append(f"-- {doc}")
            lines.append("")
    
    # Create protocol
    lines.append(f'local {protocol_name_sanitized}_proto = Proto("{protocol_name}", "{protocol_name} Protocol")')
    lines.append("")
    
    # Collect all field declarations
    all_field_declarations = []
    all_dissector_code = []
    
    # Process enums first
    enum_value_strings = []
    for type_def in main_module.type:
        if type_def.enumeration:
            type_name = type_def.name.canonical_name.object_path[-1] if type_def.name else "Unknown"
            enum_value_strings.append(_generate_enum_value_string(type_def.enumeration, type_name))
    
    if enum_value_strings:
        lines.extend(enum_value_strings)
        lines.append("")
    
    # Process structures
    for type_def in main_module.type:
        if type_def.structure:
            field_decls, dissector_code = _generate_struct_dissector(
                type_def.structure, type_def, protocol_name, ir
            )
            all_field_declarations.extend(field_decls)
            all_dissector_code.extend(dissector_code)
    
    # Define fields
    if all_field_declarations:
        lines.append("-- Protocol fields")
        lines.append("local fields = {")
        for i, decl in enumerate(all_field_declarations):
            field_name = decl.split('("')[1].split('"')[0].split('.')[-1] if '("' in decl else f"field_{i}"
            lines.append(f"  {field_name} = {decl},")
        lines.append("}")
        lines.append(f"{protocol_name_sanitized}_proto.fields = {{")
        for i, decl in enumerate(all_field_declarations):
            field_name = decl.split('("')[1].split('"')[0].split('.')[-1] if '("' in decl else f"field_{i}"
            lines.append(f"  fields.{field_name},")
        lines.append("}")
        lines.append("")
    
    # Dissector function
    lines.append(f"function {protocol_name_sanitized}_proto.dissector(buffer, pinfo, tree)")
    lines.append("  pinfo.cols.protocol = \"" + protocol_name.upper() + "\"")
    lines.append(f'  local subtree = tree:add({protocol_name_sanitized}_proto, buffer(), "{protocol_name} Protocol")')
    lines.append("  local offset = 0")
    lines.append("")
    
    if all_dissector_code:
        lines.extend(all_dissector_code)
    
    lines.append("end")
    lines.append("")
    
    # Register dissector (user will need to customize this)
    lines.append("-- Register the dissector")
    lines.append("-- Uncomment and customize the following line to register on a specific port:")
    lines.append(f"-- local udp_table = DissectorTable.get(\"udp.port\")")
    lines.append(f"-- udp_table:add(12345, {protocol_name_sanitized}_proto)")
    
    return ("\n".join(lines), errors)
