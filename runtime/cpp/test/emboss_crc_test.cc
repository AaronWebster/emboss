// Copyright 2024 Google LLC
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

#include "runtime/cpp/emboss_crc.h"

#include <cstdint>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "runtime/cpp/emboss_cpp_util.h"
#include "runtime/cpp/emboss_prelude.h"

namespace emboss {
namespace support {
namespace test {

// A simple mock array view for testing the Crc32 function.
// This mimics the interface expected by the Crc32 template function.
class MockByteArrayView {
 public:
  explicit MockByteArrayView(const ::std::uint8_t* data, ::std::size_t size)
      : data_(data), size_(size) {}

  // Mock element view that provides UncheckedRead().
  class ElementView {
   public:
    explicit ElementView(::std::uint8_t value) : value_(value) {}
    ::std::uint8_t UncheckedRead() const { return value_; }

   private:
    ::std::uint8_t value_;
  };

  // Mock SizeInBytes view that provides Read().
  class SizeView {
   public:
    explicit SizeView(::std::size_t size) : size_(size) {}
    ::std::size_t Read() const { return size_; }

   private:
    ::std::size_t size_;
  };

  SizeView SizeInBytes() const { return SizeView(size_); }

  ElementView operator[](::std::size_t index) const {
    return ElementView(data_[index]);
  }

  bool Ok() const { return data_ != nullptr; }

 private:
  const ::std::uint8_t* data_;
  ::std::size_t size_;
};

// Test CRC-32 with empty input.
// CRC-32 of empty data should be 0x00000000.
TEST(Crc32, EmptyInput) {
  const ::std::uint8_t data[] = {};
  MockByteArrayView view(data, 0);
  EXPECT_EQ(0x00000000U, Crc32(view));
}

// Test CRC-32 with the well-known "123456789" test vector.
// The expected CRC-32 (IEEE) of "123456789" is 0xCBF43926.
TEST(Crc32, KnownTestVector123456789) {
  const ::std::uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  MockByteArrayView view(data, sizeof(data));
  EXPECT_EQ(0xCBF43926U, Crc32(view));
}

// Test CRC-32 with a single byte input.
TEST(Crc32, SingleByte) {
  const ::std::uint8_t data[] = {0x00};
  MockByteArrayView view(data, sizeof(data));
  // CRC-32 of a single 0x00 byte is 0xD202EF8D
  EXPECT_EQ(0xD202EF8DU, Crc32(view));
}

// Test CRC-32 with a single byte 0xFF.
TEST(Crc32, SingleByteFF) {
  const ::std::uint8_t data[] = {0xFF};
  MockByteArrayView view(data, sizeof(data));
  // CRC-32 of a single 0xFF byte is 0xFF000000
  EXPECT_EQ(0xFF000000U, Crc32(view));
}

// Test CRC-32 with "hello" string.
TEST(Crc32, HelloString) {
  const ::std::uint8_t data[] = {'h', 'e', 'l', 'l', 'o'};
  MockByteArrayView view(data, sizeof(data));
  // CRC-32 of "hello" is 0x3610A686
  EXPECT_EQ(0x3610A686U, Crc32(view));
}

// Test CRC-32 with all zeros.
TEST(Crc32, AllZeros) {
  const ::std::uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
  MockByteArrayView view(data, sizeof(data));
  // CRC-32 of four 0x00 bytes is 0x2144DF1C
  EXPECT_EQ(0x2144DF1CU, Crc32(view));
}

// Test CRC-32 with all 0xFF bytes.
TEST(Crc32, AllFF) {
  const ::std::uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF};
  MockByteArrayView view(data, sizeof(data));
  // CRC-32 of four 0xFF bytes is 0xFFFFFFFF
  EXPECT_EQ(0xFFFFFFFFU, Crc32(view));
}

// Test CRC-32 with sequential bytes.
TEST(Crc32, SequentialBytes) {
  const ::std::uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  MockByteArrayView view(data, sizeof(data));
  // CRC-32 of {0x01, 0x02, 0x03, 0x04} is 0xB63CFBCD
  EXPECT_EQ(0xB63CFBCDU, Crc32(view));
}

// Test that the CRC-32 lookup table is correctly populated.
// The first entry should be 0 (CRC of nothing XOR'd with nothing).
TEST(Crc32Table, FirstEntry) {
  EXPECT_EQ(0x00000000U, internal::kCrc32Table[0]);
}

// The second entry should match the reflected polynomial.
TEST(Crc32Table, SecondEntry) {
  // For input byte 0x01, using reflected polynomial 0xEDB88320
  EXPECT_EQ(0x77073096U, internal::kCrc32Table[1]);
}

// Verify the table has 256 entries by checking last valid access.
TEST(Crc32Table, LastEntry) {
  // Last entry (index 255)
  EXPECT_EQ(0x2D02EF8DU, internal::kCrc32Table[255]);
}

// Test with a longer message to verify consistent behavior.
TEST(Crc32, LongerMessage) {
  const char* message = "The quick brown fox jumps over the lazy dog";
  const ::std::uint8_t* data =
      reinterpret_cast<const ::std::uint8_t*>(message);
  MockByteArrayView view(data, 43);  // Length of the message
  // CRC-32 of this message is 0x414FA339
  EXPECT_EQ(0x414FA339U, Crc32(view));
}

}  // namespace test
}  // namespace support
}  // namespace emboss
