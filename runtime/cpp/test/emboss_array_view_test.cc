// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "runtime/cpp/emboss_array_view.h"

#include <string>
#include <type_traits>

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "runtime/cpp/emboss_prelude.h"
#include "runtime/cpp/emboss_text_util.h"

namespace emboss {
namespace support {
namespace test {

using ::emboss::prelude::IntView;
using ::emboss::prelude::UIntView;

template <class ElementView, class BufferType, ::std::size_t kElementSize>
using ArrayView = GenericArrayView<ElementView, BufferType, kElementSize, 8>;

template <class ElementView, class BufferType, ::std::size_t kElementSize>
using BitArrayView = GenericArrayView<ElementView, BufferType, kElementSize, 1>;

template </**/ ::std::size_t kBits>
using LittleEndianBitBlockN =
    BitBlock<LittleEndianByteOrderer<ReadWriteContiguousBuffer>, kBits>;

template </**/ ::std::size_t kBits>
using FixedUIntView = UIntView<FixedSizeViewParameters<kBits, AllValuesAreOk>,
                               LittleEndianBitBlockN<kBits>>;

template </**/ ::std::size_t kBits>
using FixedIntView = IntView<FixedSizeViewParameters<kBits, AllValuesAreOk>,
                             LittleEndianBitBlockN<kBits>>;

TEST(ArrayView, Methods) {
  ::std::uint8_t bytes[] = {0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09,
                            0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes - 4}};
  EXPECT_EQ(sizeof bytes - 4, byte_array.SizeInBytes());
  EXPECT_EQ(bytes[0], byte_array[0].Read());
  EXPECT_EQ(bytes[1], byte_array[1].Read());
  EXPECT_EQ(bytes[2], byte_array[2].Read());
#if EMBOSS_CHECK_ABORTS
  EXPECT_DEATH(byte_array[sizeof bytes - 4].Read(), "");
#endif  // EMBOSS_CHECK_ABORTS
  EXPECT_EQ(bytes[sizeof bytes - 4],
            byte_array[sizeof bytes - 4].UncheckedRead());
  EXPECT_TRUE(byte_array[sizeof bytes - 5].IsComplete());
  EXPECT_FALSE(byte_array[sizeof bytes - 4].IsComplete());
  EXPECT_TRUE(byte_array.Ok());
  EXPECT_TRUE(byte_array.IsComplete());
  EXPECT_FALSE((ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{
          nullptr}}.Ok()));
  EXPECT_TRUE(byte_array.IsComplete());

  auto uint32_array =
      ArrayView<FixedUIntView<32>, ReadWriteContiguousBuffer, 4>{
          ReadWriteContiguousBuffer{bytes, sizeof bytes - 4}};
  EXPECT_EQ(sizeof bytes - 4, uint32_array.SizeInBytes());
  EXPECT_TRUE(uint32_array[0].Ok());
  EXPECT_EQ(0x0d0e0f10U, uint32_array[0].Read());
  EXPECT_EQ(0x090a0b0cU, uint32_array[1].Read());
  EXPECT_EQ(0x05060708U, uint32_array[2].Read());
#if EMBOSS_CHECK_ABORTS
  EXPECT_DEATH(uint32_array[3].Read(), "");
#endif  // EMBOSS_CHECK_ABORTS
  EXPECT_EQ(0x01020304U, uint32_array[3].UncheckedRead());
  EXPECT_TRUE(uint32_array[2].IsComplete());
  EXPECT_FALSE(uint32_array[3].IsComplete());
  EXPECT_TRUE(uint32_array.Ok());
  EXPECT_TRUE(uint32_array.IsComplete());
  EXPECT_FALSE((ArrayView<FixedUIntView<32>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{
          nullptr}}.Ok()));
}

TEST(ArrayView, Ok) {
  ::std::uint8_t bytes[] = {0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09,
                            0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
  // All elements are complete and, themselves, Ok(), so the array should be
  // Ok().
  auto byte_array = ArrayView<FixedUIntView<16>, ReadWriteContiguousBuffer, 2>(
      ReadWriteContiguousBuffer(bytes, sizeof bytes - 4));
  EXPECT_TRUE(byte_array.Ok());

  // An array with a partial element at the end should not be Ok().
  byte_array = ArrayView<FixedUIntView<16>, ReadWriteContiguousBuffer, 2>(
      ReadWriteContiguousBuffer(bytes, sizeof bytes - 3));
  EXPECT_FALSE(byte_array.Ok());

  // An empty array should be Ok().
  byte_array = ArrayView<FixedUIntView<16>, ReadWriteContiguousBuffer, 2>(
      ReadWriteContiguousBuffer(bytes, 0));
  EXPECT_TRUE(byte_array.Ok());
}

TEST(ArrayView, TextFormatInput) {
  ::std::uint8_t bytes[16] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};
  EXPECT_FALSE(UpdateFromText(byte_array, ""));
  EXPECT_FALSE(UpdateFromText(byte_array, "[]"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{[0"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{[0:0}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{[]:0}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{[0] 0}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{[0] 0}"));
  EXPECT_TRUE(UpdateFromText(byte_array, "{}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{,1}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{1,,}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{ a }"));
  EXPECT_TRUE(UpdateFromText(byte_array, "{1}"));
  EXPECT_EQ(1, bytes[0]);
  EXPECT_TRUE(UpdateFromText(byte_array, " {2}"));
  EXPECT_EQ(2, bytes[0]);
  EXPECT_TRUE(UpdateFromText(byte_array, " {\t\r\n4  } junk"));
  EXPECT_EQ(4, bytes[0]);
  EXPECT_TRUE(UpdateFromText(byte_array, "{3,}"));
  EXPECT_EQ(3, bytes[0]);
  EXPECT_FALSE(UpdateFromText(byte_array, "{4 5}"));
  EXPECT_TRUE(UpdateFromText(byte_array, "{4, 5}"));
  EXPECT_EQ(4, bytes[0]);
  EXPECT_EQ(5, bytes[1]);
  EXPECT_TRUE(UpdateFromText(byte_array, "{5, [6]: 5}"));
  EXPECT_EQ(5, bytes[0]);
  EXPECT_EQ(5, bytes[1]);
  EXPECT_EQ(5, bytes[6]);
  EXPECT_TRUE(UpdateFromText(byte_array, "{6, [7]:6, 6}"));
  EXPECT_EQ(6, bytes[0]);
  EXPECT_EQ(5, bytes[1]);
  EXPECT_EQ(5, bytes[6]);
  EXPECT_EQ(6, bytes[7]);
  EXPECT_EQ(6, bytes[8]);
  EXPECT_TRUE(UpdateFromText(byte_array, "{[7]: 7, 7, [0]: 7, 7}"));
  EXPECT_EQ(7, bytes[0]);
  EXPECT_EQ(7, bytes[1]);
  EXPECT_EQ(7, bytes[7]);
  EXPECT_EQ(7, bytes[8]);
  EXPECT_FALSE(UpdateFromText(byte_array, "{[16]: 0}"));
  EXPECT_FALSE(UpdateFromText(byte_array, "{[15]: 0, 0}"));
}

TEST(ArrayView, TextFormatOutput_WithAndWithoutComments) {
  signed char bytes[16] = {-3, 2, -1, 1,  0,  1,  1,  2,
                           3,  5, 8,  13, 21, 34, 55, 89};
  auto buffer = ReadWriteContiguousBuffer{
      reinterpret_cast</**/ ::std::uint8_t *>(bytes), sizeof bytes};
  auto byte_array =
      ArrayView<FixedIntView<8>, ReadWriteContiguousBuffer, 1>{buffer};
  EXPECT_EQ(
      "{ [0]: -3, 2, -1, 1, 0, 1, 1, 2, [8]: 3, 5, 8, 13, 21, 34, 55, 89 }",
      WriteToString(byte_array));
  EXPECT_EQ(WriteToString(byte_array, MultilineText()),
            R"({
  # ............."7Y
  [0]: -3  # -0x3
  [1]: 2  # 0x2
  [2]: -1  # -0x1
  [3]: 1  # 0x1
  [4]: 0  # 0x0
  [5]: 1  # 0x1
  [6]: 1  # 0x1
  [7]: 2  # 0x2
  [8]: 3  # 0x3
  [9]: 5  # 0x5
  [10]: 8  # 0x8
  [11]: 13  # 0xd
  [12]: 21  # 0x15
  [13]: 34  # 0x22
  [14]: 55  # 0x37
  [15]: 89  # 0x59
})");
  EXPECT_EQ(
      WriteToString(byte_array,
                    MultilineText().WithIndent("    ").WithComments(false)),
      R"({
    [0]: -3
    [1]: 2
    [2]: -1
    [3]: 1
    [4]: 0
    [5]: 1
    [6]: 1
    [7]: 2
    [8]: 3
    [9]: 5
    [10]: 8
    [11]: 13
    [12]: 21
    [13]: 34
    [14]: 55
    [15]: 89
})");
  EXPECT_EQ(
      WriteToString(byte_array, TextOutputOptions().WithNumericBase(16)),
      "{ [0x0]: -0x3, 0x2, -0x1, 0x1, 0x0, 0x1, 0x1, 0x2, [0x8]: 0x3, 0x5, "
      "0x8, 0xd, 0x15, 0x22, 0x37, 0x59 }");
}

TEST(ArrayView, TextFormatOutput_8BitIntElementTypes) {
  ::std::uint8_t bytes[1] = {65};
  auto buffer = ReadWriteContiguousBuffer{bytes, sizeof bytes};
  const ::std::string expected_text = R"({
  # A
  [0]: 65  # 0x41
})";
  EXPECT_EQ(
      WriteToString(
          ArrayView<FixedIntView<8>, ReadWriteContiguousBuffer, 1>{buffer},
          MultilineText()),
      expected_text);
  EXPECT_EQ(
      WriteToString(
          ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{buffer},
          MultilineText()),
      expected_text);
}

TEST(ArrayView, TextFormatOutput_16BitIntElementTypes) {
  ::std::uint16_t bytes[1] = {65};
  auto buffer = ReadWriteContiguousBuffer{
      reinterpret_cast</**/ ::std::uint8_t *>(bytes), sizeof bytes};
  const ::std::string expected_text = R"({
  [0]: 65  # 0x41
})";
  EXPECT_EQ(
      WriteToString(
          ArrayView<FixedIntView<16>, ReadWriteContiguousBuffer, 2>{buffer},
          MultilineText()),
      expected_text);
  EXPECT_EQ(
      WriteToString(
          ArrayView<FixedUIntView<16>, ReadWriteContiguousBuffer, 2>{buffer},
          MultilineText()),
      expected_text);
}

TEST(ArrayView, TextFormatOutput_MultilineComment) {
  ::std::uint8_t bytes[65];
  for (::std::size_t i = 0; i < sizeof bytes; ++i) {
    bytes[i] = '0' + (i % 10);
  }
  for (const ::std::size_t length : {63, 64, 65}) {
    auto buffer = ReadWriteContiguousBuffer{bytes, length};
    ::std::string expected_text =
        "{\n  # "
        "012345678901234567890123456789012345678901234567890123456789012";
    if (length > 63) expected_text += "3";
    if (length > 64) expected_text += "\n  # 4";
    expected_text += "\n";
    for (::std::size_t i = 0; i < length; ++i) {
      expected_text +=
          absl::StrFormat("  [%d]: %d  # 0x%02x\n", i, bytes[i], bytes[i]);
    }
    expected_text += "}";
    EXPECT_EQ(
        WriteToString(
            ArrayView<FixedIntView<8>, ReadWriteContiguousBuffer, 1>{buffer},
            MultilineText()),
        expected_text);
  }
}

TEST(ArrayView, PackedBitArray_UnpackTo_12BitTo16Bit) {
  // Test unpacking 12-bit packed data to 16-bit containers
  // 3 elements of 12 bits each = 36 bits = 4.5 bytes, uses 5 bytes
  // But ElementCount will be (6*8)/12 = 4 elements
  ::std::uint8_t packed_bytes[6] = {
      0x34, 0x82, 0x67, 0xBC, 0x0A, 0x00  // Packed: 0x234, 0x678, 0xABC
  };
  
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{packed_bytes, sizeof packed_bytes}};

  // Unpack to 16-bit buffer - will unpack 4 elements (48 bits / 12 bits each)
  ::std::uint16_t output[4] = {0};
  bool success = byte_array.UnpackTo<16, 12>(
      reinterpret_cast<::std::uint8_t *>(output), sizeof output);
  EXPECT_TRUE(success);

  // Verify unpacked values (4 elements from 6 bytes)
  EXPECT_EQ(0x234, output[0]);
  EXPECT_EQ(0x678, output[1]);
  EXPECT_EQ(0xABC, output[2]);
  EXPECT_EQ(0x000, output[3]);  // Last element is padding
}

TEST(ArrayView, PackedBitArray_PackFrom_16BitTo12Bit) {
  // Test packing 16-bit data to 12-bit packed format
  ::std::uint16_t input[3] = {0x234, 0x678, 0xABC};

  // Need 3 * 12 bits = 36 bits = 4.5 bytes, use 5 bytes buffer
  ::std::uint8_t packed_bytes[5] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{packed_bytes, sizeof packed_bytes}};

  bool success = byte_array.PackFrom<16, 12>(
      reinterpret_cast<const ::std::uint8_t *>(input), 3);
  EXPECT_TRUE(success);

  // Verify packed values
  EXPECT_EQ(0x34, packed_bytes[0]);
  EXPECT_EQ(0x82, packed_bytes[1]);
  EXPECT_EQ(0x67, packed_bytes[2]);
  EXPECT_EQ(0xBC, packed_bytes[3]);
  EXPECT_EQ(0x0A, packed_bytes[4]);
}

TEST(ArrayView, PackedBitArray_RoundTrip_12BitTo16Bit) {
  // Test round-trip: pack 16-bit to 12-bit, then unpack back to 16-bit
  ::std::uint16_t original[4] = {0x001, 0x123, 0x456, 0x789};

  // Pack to 12-bit (4 * 12 bits = 48 bits = 6 bytes)
  ::std::uint8_t packed[6] = {0};
  auto packed_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{packed, sizeof packed}};

  EXPECT_TRUE(packed_array.PackFrom<16, 12>(
      reinterpret_cast<const ::std::uint8_t *>(original), 4));

  // Verify packed bytes match expected
  EXPECT_EQ(0x01, packed[0]);
  EXPECT_EQ(0x30, packed[1]);
  EXPECT_EQ(0x12, packed[2]);
  EXPECT_EQ(0x56, packed[3]);
  EXPECT_EQ(0x94, packed[4]);
  EXPECT_EQ(0x78, packed[5]);

  // Unpack back to 16-bit
  ::std::uint16_t unpacked[4] = {0};
  EXPECT_TRUE(packed_array.UnpackTo<16, 12>(
      reinterpret_cast<::std::uint8_t *>(unpacked), sizeof unpacked));

  // Verify values match (masked to 12 bits)
  EXPECT_EQ(0x001, unpacked[0]);
  EXPECT_EQ(0x123, unpacked[1]);
  EXPECT_EQ(0x456, unpacked[2]);
  EXPECT_EQ(0x789, unpacked[3]);
}

TEST(ArrayView, PackedBitArray_UnpackTo_8BitTo16Bit) {
  // Test unpacking 8-bit to 16-bit (simple case)
  ::std::uint8_t bytes[4] = {0x12, 0x34, 0x56, 0x78};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  ::std::uint16_t output[4] = {0};
  bool success = byte_array.UnpackTo<16, 8>(
      reinterpret_cast<::std::uint8_t *>(output), sizeof output);
  EXPECT_TRUE(success);

  EXPECT_EQ(0x12, output[0]);
  EXPECT_EQ(0x34, output[1]);
  EXPECT_EQ(0x56, output[2]);
  EXPECT_EQ(0x78, output[3]);
}

TEST(ArrayView, PackedBitArray_UnpackTo_10BitTo16Bit) {
  // Test unpacking 10-bit packed data to 16-bit containers
  // 4 elements * 10 bits = 40 bits = 5 bytes
  ::std::uint8_t packed_bytes[5] = {
      0xFF, 0x03,  // First element: 0x3FF (all 10 bits set)
      0x00, 0x00,  // Second element: 0x000
      0x55        // Third element: partial (only bottom bits)
  };

  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{packed_bytes, sizeof packed_bytes}};

  ::std::uint16_t output[4] = {0};
  bool success = byte_array.UnpackTo<16, 10>(
      reinterpret_cast<::std::uint8_t *>(output), sizeof output);
  EXPECT_TRUE(success);

  EXPECT_EQ(0x3FF, output[0]);
  EXPECT_EQ(0x000, output[1]);
  EXPECT_EQ(0x155, output[2]);
}

TEST(ArrayView, PackedBitArray_PackFrom_BufferTooSmall) {
  // Test that packing fails when buffer is too small
  ::std::uint16_t input[4] = {0x123, 0x456, 0x789, 0xABC};

  // Buffer is too small for 4 * 12 bits
  ::std::uint8_t packed_bytes[4] = {0};  // Need 6 bytes, only have 4
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{packed_bytes, sizeof packed_bytes}};

  // This should fail because buffer is too small
  bool success = byte_array.PackFrom<16, 12>(
      reinterpret_cast<const ::std::uint8_t *>(input), 4);
  EXPECT_FALSE(success);
}

TEST(ArrayView, PackedBitArray_UnpackTo_BufferTooSmall) {
  // Test that unpacking fails when output buffer is too small
  ::std::uint8_t packed_bytes[6] = {0x34, 0x12, 0x78, 0x56, 0xBC, 0x9A};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{packed_bytes, sizeof packed_bytes}};

  // Output buffer too small
  ::std::uint16_t output[2] = {0};  // Need 3 elements, only have 2
  bool success = byte_array.UnpackTo<16, 12>(
      reinterpret_cast<::std::uint8_t *>(output), sizeof output);
  EXPECT_FALSE(success);
}

TEST(ArrayView, PackedBitArray_UnpackedSizeInBytes) {
  // Test UnpackedSizeInBytes calculation for byte array
  // The array has 6 UInt:8 elements, so ElementCount() = 6
  ::std::uint8_t bytes[6] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  // 6 elements * 1 byte per element = 6 bytes for 8-bit target
  EXPECT_EQ(6, byte_array.UnpackedSizeInBytes<8>());

  // 6 elements * 2 bytes per element = 12 bytes for 16-bit target
  EXPECT_EQ(12, byte_array.UnpackedSizeInBytes<16>());

  // 6 elements * 4 bytes per element = 24 bytes for 32-bit target
  EXPECT_EQ(24, byte_array.UnpackedSizeInBytes<32>());

  // 6 elements * 8 bytes per element = 48 bytes for 64-bit target
  EXPECT_EQ(48, byte_array.UnpackedSizeInBytes<64>());
}

}  // namespace test
}  // namespace support
}  // namespace emboss
