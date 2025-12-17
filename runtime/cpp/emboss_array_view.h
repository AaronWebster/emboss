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

// View classes for arrays and bit arrays.
#ifndef EMBOSS_RUNTIME_CPP_EMBOSS_ARRAY_VIEW_H_
#define EMBOSS_RUNTIME_CPP_EMBOSS_ARRAY_VIEW_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <tuple>
#include <type_traits>

#include "runtime/cpp/emboss_arithmetic.h"

namespace emboss {

// Forward declarations for use by WriteShorthandArrayCommentToTextStream.
class TextOutputOptions;
namespace support {
template <class Array, class Stream>
void WriteShorthandAsciiArrayCommentToTextStream(
    const Array *array, Stream *stream, const TextOutputOptions &options);
}
namespace prelude {
template <class Parameters, class BitViewType>
class UIntView;
template <class Parameters, class BitViewType>
class IntView;
}  // namespace prelude

namespace support {

// Advance direction for ElementViewIterator.
enum class ElementViewIteratorDirection { kForward, kReverse };

// Iterator adapter for elements in a GenericArrayView.
template <class GenericArrayView, ElementViewIteratorDirection kDirection>
class ElementViewIterator {
 public:
  using iterator_category = ::std::random_access_iterator_tag;
  using value_type = typename GenericArrayView::ViewType;
  using difference_type = ::std::ptrdiff_t;
  using pointer = typename ::std::add_pointer<value_type>::type;
  using reference = typename ::std::add_lvalue_reference<value_type>::type;

  explicit ElementViewIterator(const GenericArrayView array_view,
                               ::std::ptrdiff_t index)
      : array_view_(array_view), view_(array_view.at(index)), index_(index) {}

  ElementViewIterator() = default;

  reference operator*() { return view_; }

  pointer operator->() { return &view_; }

  ElementViewIterator &operator+=(difference_type d) {
    index_ += (kDirection == ElementViewIteratorDirection::kForward ? d : -d);
    view_ = array_view_.at(index_);
    return *this;
  }

  ElementViewIterator &operator-=(difference_type d) { return *this += (-d); }

  ElementViewIterator &operator++() {
    *this += 1;
    return *this;
  }

  ElementViewIterator &operator--() {
    *this -= 1;
    return *this;
  }

  ElementViewIterator operator++(int) {
    auto copy = *this;
    ++(*this);
    return copy;
  }

  ElementViewIterator operator--(int) {
    auto copy = *this;
    --(*this);
    return copy;
  }

  ElementViewIterator operator+(difference_type d) const {
    auto copy = *this;
    copy += d;
    return copy;
  }

  ElementViewIterator operator-(difference_type d) const {
    return *this + (-d);
  }

  difference_type operator-(const ElementViewIterator &other) const {
    return kDirection == ElementViewIteratorDirection::kForward
               ? index_ - other.index_
               : other.index_ - index_;
  }

  bool operator==(const ElementViewIterator &other) const {
    return array_view_ == other.array_view_ && index_ == other.index_;
  }

  bool operator!=(const ElementViewIterator &other) const {
    return !(*this == other);
  }

  bool operator<(const ElementViewIterator &other) const {
    return kDirection == ElementViewIteratorDirection::kForward
               ? index_ < other.index_
               : other.index_ < index_;
  }

  bool operator<=(const ElementViewIterator &other) const {
    return kDirection == ElementViewIteratorDirection::kForward
               ? index_ <= other.index_
               : other.index_ <= index_;
  }

  bool operator>(const ElementViewIterator &other) const {
    return !(*this <= other);
  }

  bool operator>=(const ElementViewIterator &other) const {
    return !(*this < other);
  }

 private:
  const GenericArrayView array_view_;
  typename GenericArrayView::ViewType view_;
  ::std::ptrdiff_t index_;
};

// View for an array in a structure.
//
// ElementView should be the view class for a single array element (e.g.,
// UIntView<...> or ArrayView<...>).
//
// BufferType is the storage type that will be passed into the array.
//
// kElementSize is the fixed size of a single element, in addressable units.
//
// kAddressableUnitSize is the size of a single addressable unit.  It should be
// either 1 (one bit) or 8 (one byte).
//
// ElementViewParameterTypes is a list of the types of parameters which must be
// passed down to each element of the array.  ElementViewParameterTypes can be
// empty.
template <class ElementView, class BufferType, ::std::size_t kElementSize,
          ::std::size_t kAddressableUnitSize,
          typename... ElementViewParameterTypes>
class GenericArrayView final {
 public:
  using ViewType = ElementView;
  using ForwardIterator =
      ElementViewIterator<GenericArrayView,
                          ElementViewIteratorDirection::kForward>;
  using ReverseIterator =
      ElementViewIterator<GenericArrayView,
                          ElementViewIteratorDirection::kReverse>;

  GenericArrayView() : buffer_() {}
  explicit GenericArrayView(const ElementViewParameterTypes &...parameters,
                            BufferType buffer)
      : parameters_{parameters...}, buffer_{buffer} {}

  ElementView operator[](::std::size_t index) const {
    return IndexOperatorHelper<(sizeof...(ElementViewParameterTypes) ==
                                0)>::UncheckedConstructElement(parameters_,
                                                               buffer_, index);
  }

  ElementView at(::std::size_t index) const {
    return IndexOperatorHelper<(sizeof...(ElementViewParameterTypes) ==
                                0)>::ConstructElement(parameters_, buffer_,
                                                      index, ElementCount());
  }

  ForwardIterator begin() const { return ForwardIterator(*this, 0); }
  ForwardIterator end() const { return ForwardIterator(*this, ElementCount()); }
  ReverseIterator rbegin() const {
    return ReverseIterator(*this, ElementCount() - 1);
  }
  ReverseIterator rend() const { return ReverseIterator(*this, -1); }

  // In order to selectively enable SizeInBytes and SizeInBits, it is
  // necessary to make them into templates.  Further, it is necessary for
  // ::std::enable_if to have a dependency on the template parameter, otherwise
  // SFINAE won't kick in.  Thus, these are templated on an int, and that int
  // is (spuriously) used as the left argument to `,` in the enable_if
  // condition.  The explicit cast to void is needed to silence GCC's
  // -Wunused-value.
  template <int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 8),
                            ::std::size_t>::type
  SizeInBytes() const {
    return buffer_.SizeInBytes();
  }
  template <int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 1),
                            ::std::size_t>::type
  SizeInBits() const {
    return buffer_.SizeInBits();
  }

  ::std::size_t ElementCount() const { return SizeOfBuffer() / kElementSize; }
  bool Ok() const {
    if (!buffer_.Ok()) return false;
    if (SizeOfBuffer() % kElementSize != 0) return false;
    for (::std::size_t i = 0; i < ElementCount(); ++i) {
      if (!(*this)[i].Ok()) return false;
    }
    return true;
  }
  template <class OtherElementView, class OtherBufferType>
  bool Equals(
      const GenericArrayView<OtherElementView, OtherBufferType, kElementSize,
                             kAddressableUnitSize> &other) const {
    if (ElementCount() != other.ElementCount()) return false;
    for (::std::size_t i = 0; i < ElementCount(); ++i) {
      if (!(*this)[i].Equals(other[i])) return false;
    }
    return true;
  }
  template <class OtherElementView, class OtherBufferType>
  bool UncheckedEquals(
      const GenericArrayView<OtherElementView, OtherBufferType, kElementSize,
                             kAddressableUnitSize> &other) const {
    if (ElementCount() != other.ElementCount()) return false;
    for (::std::size_t i = 0; i < ElementCount(); ++i) {
      if (!(*this)[i].UncheckedEquals(other[i])) return false;
    }
    return true;
  }
  bool IsComplete() const { return buffer_.Ok(); }

  template <class Stream>
  bool UpdateFromTextStream(Stream *stream) const {
    return ReadArrayFromTextStream(this, stream);
  }

  template <class Stream>
  void WriteToTextStream(Stream *stream,
                         const TextOutputOptions &options) const {
    WriteArrayToTextStream(this, stream, options);
  }

  static constexpr bool IsAggregate() { return true; }

  BufferType BackingStorage() const { return buffer_; }

  // Forwards to BufferType's ToString(), if any, but only if ElementView is a
  // 1-byte type.
  template <typename String>
  typename ::std::enable_if<kAddressableUnitSize == 8 && kElementSize == 1,
                            String>::type
  ToString() const {
    EMBOSS_CHECK(Ok());
    return BackingStorage().template ToString<String>();
  }

  bool operator==(const GenericArrayView &other) const {
    return parameters_ == other.parameters_ && buffer_ == other.buffer_;
  }

  // Packed bit array support: UnpackedSizeInBytes calculates the size needed
  // for an unpacked buffer when unpacking to TargetBits per element.
  // Only available for byte arrays (kAddressableUnitSize == 8).
  template <::std::size_t TargetBits, int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 8),
                            ::std::size_t>::type
  UnpackedSizeInBytes() const {
    static_assert(TargetBits > 0 && TargetBits <= 64,
                  "TargetBits must be between 1 and 64");
    static_assert(TargetBits % 8 == 0,
                  "TargetBits must be a multiple of 8");
    return ElementCount() * (TargetBits / 8);
  }

  // Unpacks packed bit data from this array to an external buffer.
  // TargetBits specifies the bit width of each unpacked element (must be 8, 16,
  // 32, or 64). SourceBits specifies the bit width of each packed element in
  // this array (e.g., 12 for 12-bit packed data).
  // Returns true if successful, false if buffer is too small or parameters are
  // invalid. Only available for byte arrays (kAddressableUnitSize == 8).
  template <::std::size_t TargetBits, ::std::size_t SourceBits, int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 8), bool>::type
  UnpackTo(::std::uint8_t *output_buffer, ::std::size_t output_buffer_size) const {
    static_assert(TargetBits == 8 || TargetBits == 16 || TargetBits == 32 ||
                      TargetBits == 64,
                  "TargetBits must be 8, 16, 32, or 64");
    static_assert(SourceBits > 0 && SourceBits <= 64,
                  "SourceBits must be between 1 and 64");
    static_assert(SourceBits <= TargetBits,
                  "SourceBits must be <= TargetBits");

    if (!Ok()) return false;

    // Calculate how many SourceBits elements fit in the buffer
    const ::std::size_t total_bits = SizeInBytes() * 8;
    const ::std::size_t element_count = total_bits / SourceBits;
    const ::std::size_t required_size = element_count * (TargetBits / 8);
    if (output_buffer_size < required_size) return false;

    return UnpackToImpl<TargetBits, SourceBits>(output_buffer, element_count);
  }

  // Packs data from an external buffer into this array's packed format.
  // SourceBits specifies the bit width of each element in the input buffer
  // (must be 8, 16, 32, or 64). TargetBits specifies the bit width of each
  // packed element in this array (e.g., 12 for 12-bit packed data).
  // Returns true if successful, false if parameters are invalid.
  // Only available for byte arrays (kAddressableUnitSize == 8).
  template <::std::size_t SourceBits, ::std::size_t TargetBits, int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 8), bool>::type
  PackFrom(const ::std::uint8_t *input_buffer, ::std::size_t element_count) {
    static_assert(SourceBits == 8 || SourceBits == 16 || SourceBits == 32 ||
                      SourceBits == 64,
                  "SourceBits must be 8, 16, 32, or 64");
    static_assert(TargetBits > 0 && TargetBits <= 64,
                  "TargetBits must be between 1 and 64");
    static_assert(TargetBits <= SourceBits,
                  "TargetBits must be <= SourceBits");

    if (!buffer_.Ok()) return false;
    
    // Check if buffer can hold element_count * TargetBits bits
    const ::std::size_t required_bits = element_count * TargetBits;
    const ::std::size_t available_bits = SizeInBytes() * 8;
    if (available_bits < required_bits) return false;

    return PackFromImpl<SourceBits, TargetBits>(input_buffer, element_count);
  }

 private:
  // This uses the same technique to select the correct definition of
  // SizeOfBuffer() as in the SizeInBits()/SizeInBytes() selection above.
  template <int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 8),
                            ::std::size_t>::type
  SizeOfBuffer() const {
    return SizeInBytes();
  }
  template <int N = 0>
  typename ::std::enable_if<((void)N, kAddressableUnitSize == 1),
                            ::std::size_t>::type
  SizeOfBuffer() const {
    return SizeInBits();
  }

  // Helper function to unpack packed bits to a wider format.
  // Uses a naive implementation that extracts bits one element at a time.
  template <::std::size_t TargetBits, ::std::size_t SourceBits>
  bool UnpackToImpl(::std::uint8_t *output_buffer,
                    ::std::size_t element_count) const {
    const ::std::uint8_t *input =
        reinterpret_cast<const ::std::uint8_t *>(buffer_.data());
    if (input == nullptr) return false;

    ::std::size_t bit_offset = 0;
    for (::std::size_t i = 0; i < element_count; ++i) {
      // Extract SourceBits from the packed array
      ::std::uint64_t value = 0;
      for (::std::size_t bit = 0; bit < SourceBits; ++bit) {
        const ::std::size_t byte_index = (bit_offset + bit) / 8;
        const ::std::size_t bit_index = (bit_offset + bit) % 8;
        if ((input[byte_index] >> bit_index) & 1) {
          value |= (static_cast</**/ ::std::uint64_t>(1) << bit);
        }
      }
      bit_offset += SourceBits;

      // Write the value to the output buffer
      if (TargetBits == 8) {
        output_buffer[i] = static_cast</**/ ::std::uint8_t>(value);
      } else if (TargetBits == 16) {
        ::std::uint16_t *output16 =
            reinterpret_cast</**/ ::std::uint16_t *>(output_buffer);
        output16[i] = static_cast</**/ ::std::uint16_t>(value);
      } else if (TargetBits == 32) {
        ::std::uint32_t *output32 =
            reinterpret_cast</**/ ::std::uint32_t *>(output_buffer);
        output32[i] = static_cast</**/ ::std::uint32_t>(value);
      } else if (TargetBits == 64) {
        ::std::uint64_t *output64 =
            reinterpret_cast</**/ ::std::uint64_t *>(output_buffer);
        output64[i] = value;
      }
    }
    return true;
  }

  // Helper function to pack data from a wider format to packed bits.
  // Uses a naive implementation that packs bits one element at a time.
  template <::std::size_t SourceBits, ::std::size_t TargetBits>
  bool PackFromImpl(const ::std::uint8_t *input_buffer,
                    ::std::size_t element_count) {
    ::std::uint8_t *output =
        reinterpret_cast</**/ ::std::uint8_t *>(buffer_.data());
    if (output == nullptr) return false;

    // Calculate and verify output buffer size
    const ::std::size_t output_bytes = (element_count * TargetBits + 7) / 8;
    const ::std::size_t available_bytes = buffer_.SizeInBytes();
    if (output_bytes > available_bytes) return false;

    // Clear the output buffer first
    for (::std::size_t i = 0; i < output_bytes; ++i) {
      output[i] = 0;
    }

    ::std::size_t bit_offset = 0;
    for (::std::size_t i = 0; i < element_count; ++i) {
      // Read the value from the input buffer
      ::std::uint64_t value = 0;
      if (SourceBits == 8) {
        value = input_buffer[i];
      } else if (SourceBits == 16) {
        const ::std::uint16_t *input16 =
            reinterpret_cast<const ::std::uint16_t *>(input_buffer);
        value = input16[i];
      } else if (SourceBits == 32) {
        const ::std::uint32_t *input32 =
            reinterpret_cast<const ::std::uint32_t *>(input_buffer);
        value = input32[i];
      } else if (SourceBits == 64) {
        const ::std::uint64_t *input64 =
            reinterpret_cast<const ::std::uint64_t *>(input_buffer);
        value = input64[i];
      }

      // Mask the value to TargetBits
      const ::std::uint64_t mask =
          (TargetBits >= 64) ? ::std::uint64_t(-1)
                             : ((static_cast</**/ ::std::uint64_t>(1)
                                 << TargetBits) -
                                1);
      value &= mask;

      // Pack TargetBits into the output array
      for (::std::size_t bit = 0; bit < TargetBits; ++bit) {
        const ::std::size_t byte_index = (bit_offset + bit) / 8;
        const ::std::size_t bit_index = (bit_offset + bit) % 8;
        if ((value >> bit) & 1) {
          output[byte_index] |= (1 << bit_index);
        }
      }
      bit_offset += TargetBits;
    }
    return true;
  }

  // This mess is needed to expand the parameters_ tuple into individual
  // arguments to the ElementView constructor.  If parameters_ has M elements,
  // then:
  //
  // IndexOperatorHelper<false>::ConstructElement() calls
  // IndexOperatorHelper<false, 0>::ConstructElement(), which calls
  // IndexOperatorHelper<false, 0, 1>::ConstructElement(), and so on, up to
  // IndexOperatorHelper<false, 0, 1, ..., M-1>::ConstructElement(), which calls
  // IndexOperatorHelper<true, 0, 1, ..., M>::ConstructElement()
  //
  // That last call will resolve to the second, specialized version of
  // IndexOperatorHelper.  That version's ConstructElement() uses
  // `std::get<N>(parameters)...`, which will be expanded into
  // `std::get<0>(parameters), std::get<1>(parameters), std::get<2>(parameters),
  // ..., std::get<M>(parameters)`.
  //
  // If there are 0 parameters, then operator[]() will call
  // IndexOperatorHelper<true>::ConstructElement(), which still works --
  // `std::get<N>(parameters)...,` will be replaced by ``.
  //
  // In C++14, a lot of this can be replaced by std::index_sequence_of, and in
  // C++17 it can be replaced with std::apply and a lambda.
  //
  // An alternate solution would be to force each parameterized view to have a
  // constructor that accepts a tuple, instead of individual parameters, but
  // that (further) complicates the matrix of constructors for view types.
  template <bool, ::std::size_t... N>
  struct IndexOperatorHelper {
    static ElementView ConstructElement(
        const ::std::tuple<ElementViewParameterTypes...> &parameters,
        BufferType buffer, ::std::size_t index, ::std::size_t size) {
      return IndexOperatorHelper<
          (sizeof...(ElementViewParameterTypes) == 1 + sizeof...(N)), N...,
          sizeof...(N)>::ConstructElement(parameters, buffer, index, size);
    }

    static ElementView UncheckedConstructElement(
        const ::std::tuple<ElementViewParameterTypes...> &parameters,
        BufferType buffer, ::std::size_t index) {
      return IndexOperatorHelper<
          (sizeof...(ElementViewParameterTypes) == 1 + sizeof...(N)), N...,
          sizeof...(N)>::UncheckedConstructElement(parameters, buffer, index);
    }
  };

  template </**/ ::std::size_t... N>
  struct IndexOperatorHelper<true, N...> {
    static ElementView ConstructElement(
        const ::std::tuple<ElementViewParameterTypes...> &parameters,
        BufferType buffer, ::std::size_t index, ::std::size_t size) {
      return ElementView(
          ::std::get<N>(parameters)...,
          index < 0 || index >= size
              ? typename BufferType::template OffsetStorageType<kElementSize,
                                                                0>(nullptr)
              : buffer.template GetOffsetStorage<kElementSize, 0>(
                    kElementSize * index, kElementSize));
    }

    static ElementView UncheckedConstructElement(
        const ::std::tuple<ElementViewParameterTypes...> &parameters,
        BufferType buffer, ::std::size_t index) {
      return ElementView(::std::get<N>(parameters)...,
                         buffer.template GetOffsetStorage<kElementSize, 0>(
                             kElementSize * index, kElementSize));
    }
  };

  ::std::tuple<ElementViewParameterTypes...> parameters_;
  BufferType buffer_;
};

// Optionally prints a shorthand representation of a BitArray in a comment.
template <class ElementView, class BufferType, ::std::size_t kElementSize,
          ::std::size_t kAddressableUnitSize, class Stream>
void WriteShorthandArrayCommentToTextStream(
    const GenericArrayView<ElementView, BufferType, kElementSize,
                           kAddressableUnitSize> *array,
    Stream *stream, const TextOutputOptions &options) {
  // Intentionally empty.  Overload for specific element types.
  // Avoid unused parameters error:
  static_cast<void>(array);
  static_cast<void>(stream);
  static_cast<void>(options);
}

// Overload for arrays of UInt.
// Prints out the elements as ASCII characters for arrays of UInt:8.
template <class BufferType, class BitViewType, class Stream,
          ::std::size_t kElementSize, class Parameters,
          class = typename ::std::enable_if<Parameters::kBits == 8>::type>
void WriteShorthandArrayCommentToTextStream(
    const GenericArrayView<prelude::UIntView<Parameters, BitViewType>,
                           BufferType, kElementSize, 8> *array,
    Stream *stream, const TextOutputOptions &options) {
  WriteShorthandAsciiArrayCommentToTextStream(array, stream, options);
}

// Overload for arrays of UInt.
// Prints out the elements as ASCII characters for arrays of Int:8.
template <class BufferType, class BitViewType, class Stream,
          ::std::size_t kElementSize, class Parameters,
          class = typename ::std::enable_if<Parameters::kBits == 8>::type>
void WriteShorthandArrayCommentToTextStream(
    const GenericArrayView<prelude::IntView<Parameters, BitViewType>,
                           BufferType, kElementSize, 8> *array,
    Stream *stream, const TextOutputOptions &options) {
  WriteShorthandAsciiArrayCommentToTextStream(array, stream, options);
}

}  // namespace support
}  // namespace emboss

#endif  // EMBOSS_RUNTIME_CPP_EMBOSS_ARRAY_VIEW_H_
