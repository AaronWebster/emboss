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

#include <array>
#include <string>
#include <type_traits>
#include <vector>

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

TEST(ArrayView, CopyFromVector) {
  ::std::uint8_t bytes[8] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  ::std::vector<::std::uint8_t> source = {1, 2, 3, 4, 5, 6, 7, 8};
  byte_array.CopyFrom(source);

  EXPECT_EQ(1, byte_array[0].Read());
  EXPECT_EQ(2, byte_array[1].Read());
  EXPECT_EQ(3, byte_array[2].Read());
  EXPECT_EQ(4, byte_array[3].Read());
  EXPECT_EQ(5, byte_array[4].Read());
  EXPECT_EQ(6, byte_array[5].Read());
  EXPECT_EQ(7, byte_array[6].Read());
  EXPECT_EQ(8, byte_array[7].Read());
}

TEST(ArrayView, CopyFromArray) {
  ::std::uint8_t bytes[4] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  ::std::array<::std::uint8_t, 4> source = {{10, 20, 30, 40}};
  byte_array.CopyFrom(source);

  EXPECT_EQ(10, byte_array[0].Read());
  EXPECT_EQ(20, byte_array[1].Read());
  EXPECT_EQ(30, byte_array[2].Read());
  EXPECT_EQ(40, byte_array[3].Read());
}

TEST(ArrayView, CopyFromGenericArrayView) {
  ::std::uint8_t source_bytes[] = {1, 2, 3, 4};
  ::std::uint8_t dest_bytes[4] = {0};

  auto source_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{source_bytes, sizeof source_bytes}};
  auto dest_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{dest_bytes, sizeof dest_bytes}};

  dest_array.CopyFrom(source_array);

  EXPECT_EQ(1, dest_array[0].Read());
  EXPECT_EQ(2, dest_array[1].Read());
  EXPECT_EQ(3, dest_array[2].Read());
  EXPECT_EQ(4, dest_array[3].Read());
}

TEST(ArrayView, UncheckedCopyFromVector) {
  ::std::uint8_t bytes[4] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  ::std::vector<::std::uint8_t> source = {100, 101, 102, 103};
  byte_array.UncheckedCopyFrom(source);

  EXPECT_EQ(100, byte_array[0].UncheckedRead());
  EXPECT_EQ(101, byte_array[1].UncheckedRead());
  EXPECT_EQ(102, byte_array[2].UncheckedRead());
  EXPECT_EQ(103, byte_array[3].UncheckedRead());
}

TEST(ArrayView, UncheckedCopyFromGenericArrayView) {
  ::std::uint8_t source_bytes[] = {5, 6, 7, 8};
  ::std::uint8_t dest_bytes[4] = {0};

  auto source_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{source_bytes, sizeof source_bytes}};
  auto dest_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{dest_bytes, sizeof dest_bytes}};

  dest_array.UncheckedCopyFrom(source_array);

  EXPECT_EQ(5, dest_array[0].UncheckedRead());
  EXPECT_EQ(6, dest_array[1].UncheckedRead());
  EXPECT_EQ(7, dest_array[2].UncheckedRead());
  EXPECT_EQ(8, dest_array[3].UncheckedRead());
}

TEST(ArrayView, TryToCopyFromVector) {
  ::std::uint8_t bytes[4] = {0};
  auto byte_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  // Source with matching size should succeed
  ::std::vector<::std::uint8_t> source = {11, 22, 33, 44};
  EXPECT_TRUE(byte_array.TryToCopyFrom(source));
  EXPECT_EQ(11, byte_array[0].Read());
  EXPECT_EQ(22, byte_array[1].Read());
  EXPECT_EQ(33, byte_array[2].Read());
  EXPECT_EQ(44, byte_array[3].Read());

  // Source with insufficient size should fail
  ::std::vector<::std::uint8_t> short_source = {1, 2};
  EXPECT_FALSE(byte_array.TryToCopyFrom(short_source));
}

TEST(ArrayView, TryToCopyFromGenericArrayView) {
  ::std::uint8_t source_bytes[] = {9, 8, 7, 6};
  ::std::uint8_t dest_bytes[4] = {0};

  auto source_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{source_bytes, sizeof source_bytes}};
  auto dest_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{dest_bytes, sizeof dest_bytes}};

  EXPECT_TRUE(dest_array.TryToCopyFrom(source_array));
  EXPECT_EQ(9, dest_array[0].Read());
  EXPECT_EQ(8, dest_array[1].Read());
  EXPECT_EQ(7, dest_array[2].Read());
  EXPECT_EQ(6, dest_array[3].Read());
}

TEST(ArrayView, TryToCopyFromGenericArrayViewSizeMismatch) {
  ::std::uint8_t source_bytes[] = {1, 2, 3};
  ::std::uint8_t dest_bytes[4] = {0};

  auto source_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{source_bytes, sizeof source_bytes}};
  auto dest_array = ArrayView<FixedUIntView<8>, ReadWriteContiguousBuffer, 1>{
      ReadWriteContiguousBuffer{dest_bytes, sizeof dest_bytes}};

  // Different sizes should fail
  EXPECT_FALSE(dest_array.TryToCopyFrom(source_array));
}

TEST(ArrayView, CopyFrom32BitIntegers) {
  ::std::uint8_t bytes[16] = {0};
  auto uint32_array =
      ArrayView<FixedUIntView<32>, ReadWriteContiguousBuffer, 4>{
          ReadWriteContiguousBuffer{bytes, sizeof bytes}};

  ::std::vector<::std::uint32_t> source = {0x12345678, 0xABCDEF01, 0x11111111,
                                           0x22222222};
  uint32_array.CopyFrom(source);

  EXPECT_EQ(0x12345678U, uint32_array[0].Read());
  EXPECT_EQ(0xABCDEF01U, uint32_array[1].Read());
  EXPECT_EQ(0x11111111U, uint32_array[2].Read());
  EXPECT_EQ(0x22222222U, uint32_array[3].Read());
}

}  // namespace test
}  // namespace support
}  // namespace emboss
