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

"""Tests for dissector_generator module."""

import unittest
from compiler.back_end.lua import dissector_generator
from compiler.util import ir_data


class DissectorGeneratorTest(unittest.TestCase):
    """Tests for the Lua dissector generator."""

    def test_basic_enum_generation(self):
        """Test that we can generate a simple enum."""
        ir = ir_data.EmbossIr()
        module = ir_data.Module()
        module.source_file_name = "test.emb"
        module.source_text = "-- Test"
        
        # Create an enum
        enum_type = ir_data.TypeDefinition()
        enum_type.name = ir_data.NameDefinition()
        enum_type.name.canonical_name = ir_data.CanonicalName()
        enum_type.name.canonical_name.object_path = ["TestEnum"]
        
        enum_def = ir_data.Enum()
        
        # Add enum value
        enum_val = ir_data.EnumValue()
        enum_val.name = ir_data.NameDefinition()
        enum_val.name.canonical_name = ir_data.CanonicalName()
        enum_val.name.canonical_name.object_path = ["VALUE_ONE"]
        enum_val.value = ir_data.Expression()
        enum_val.value.constant = ir_data.NumericConstant()
        enum_val.value.constant.value = "1"
        enum_def.value.append(enum_val)
        
        enum_type.enumeration = enum_def
        enum_type.addressable_unit = ir_data.AddressableUnit.BIT
        
        module.type.append(enum_type)
        ir.module.append(module)
        
        # Generate dissector
        code, errors = dissector_generator.generate_dissector(ir, "test")
        
        self.assertEqual(len(errors), 0)
        self.assertIn("TestEnum_values", code)
        self.assertIn("VALUE_ONE", code)
        self.assertIn("test_proto", code)

    def test_documentation_extraction(self):
        """Test that documentation is properly extracted."""
        doc = ir_data.Documentation()
        doc.text = "This is a test comment"
        
        result = dissector_generator._get_documentation_text([doc])
        self.assertEqual(result, "This is a test comment")

    def test_sanitize_lua_identifier(self):
        """Test Lua identifier sanitization."""
        # Test normal identifier
        self.assertEqual(
            dissector_generator._sanitize_lua_identifier("normal_name"),
            "normal_name"
        )
        
        # Test identifier starting with number
        self.assertEqual(
            dissector_generator._sanitize_lua_identifier("123name"),
            "_123name"
        )
        
        # Test identifier with invalid characters
        self.assertEqual(
            dissector_generator._sanitize_lua_identifier("name-with-dashes"),
            "name_with_dashes"
        )


if __name__ == "__main__":
    unittest.main()
