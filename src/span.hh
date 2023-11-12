/**
 * std::span<> implementation for older compilers.
 * Copyright (c) 2022 Egor
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#ifndef VHVC_SPAN
#define VHVC_SPAN

#include <stddef.h>	// for ::size_t
#include <cstddef>	// for std::byte
#include <array>	// for std::array
#include <type_traits>	// for many things
#include <iterator>	// for std::reverse_iterator
#include <limits>	// for std::numeric_limits
#include <memory>	// for std::addressof
#include <utility>	// for std::declval

namespace vhvc {
#if __cpp_inline_variables >= 201606L
	inline
#endif
	constexpr size_t dynamic_extent = std::numeric_limits<size_t>::max();

	template<class ElementType, size_t Extent>
	class span;

	// private stuff
	namespace span_impl {
		// this is used to bypass CTAD in the T(&)[N] constructor
		template<class T>
		struct type_identity { using type = T; };

		/* SFINAE helpers.
		 * These are defined like enable_if<value, int> to simplify the use in
		 * constructors. See SPANCK macro below.
		 *
		 * I want this code to be usable in C++11, so I use ::type and ::value
		 * everywhere.
		 *
		 * The range constructor uses r.data() and r.size(). The real ranges
		 * functions also check member begin/end and ADL begin/end/size.
		 * (Arrays too, but we have to exclude arrays anyway)
		 * std::initializer_list in particular is affected by this.
		 *
		 * For iterator constructors we rely on addressof(*begin) and end-begin.
		 * No fancy checks are done to determine if it's a contiguous iter.
		 */

		// checks if it's okay to have a size of N in span<..., E>
		template<size_t E, size_t N>
		struct extent_eq: std::enable_if<E == dynamic_extent || E == N, int> {};

		// checks that It is contiguous iterator
		template<class It, class = void>
		struct is_citer_impl: std::false_type {};
		template<class It>
		struct is_citer_impl<It, decltype(void(std::addressof(*std::declval<It>())))>: std::true_type {};
#if !defined(_MSC_VER) || _MSC_VER >= 1900
		template<class It>
		struct is_citer: std::enable_if<is_citer_impl<It>::value, int> {};
#else
		// FIXME: before which version of MSVC exactly is this needed?
		template<class It>
		struct is_citer: std::enable_if<true, int> {};
#endif

		// checks that It is contiguous iterator and End is a corresponding end iterator
		template<class It, class End, class = void>
		struct is_citer_pair_impl: std::false_type {};
		template<class It, class End>
		struct is_citer_pair_impl<It, End, decltype(void(std::declval<End>() - std::declval<It>()))>: std::true_type {};
#if !defined(_MSC_VER) || _MSC_VER >= 1900
		template<class It, class End>
		struct is_citer_pair: std::enable_if<
				is_citer_impl<It>::value &&
				is_citer_pair_impl<It, End>::value &&
				!std::is_convertible<End, size_t>::value,
			int> {};
#else
		// FIXME: before which version of MSVC exactly is this needed?
		template<class It, class End>
		struct is_citer_pair: std::enable_if<
				!std::is_convertible<End, size_t>::value,
			int> {};
#endif

		// checks that R is a contiguous, sized range, but not a span or std::array
		template<class R, class = void>
		struct is_range_impl: std::false_type {};
		template<class R>
		struct is_range_impl<R, decltype(void((std::declval<R>().data(), std::declval<R>().size())))>: std::true_type {};
#if !defined(_MSC_VER) || _MSC_VER >= 1900
		template<class R>
		struct is_range: std::enable_if<is_range_impl<R>::value, int> {};
#else
		// FIXME: before which version of MSVC exactly is this needed?
		template<class It>
		struct is_range: std::enable_if<true, int> {};
#endif
		template<class ElementType, size_t Extent>
		struct is_range<span<ElementType, Extent>> {};
		template<class ElementType, size_t Extent>
		struct is_range<std::array<ElementType, Extent>> {};

		// base type that stores the dynamic_extent
		template<size_t Extent>
		struct base {
			constexpr base(size_t) noexcept {}
			constexpr size_t get_size() const noexcept { return Extent; }
		};
		template<>
		struct base<dynamic_extent> {
			constexpr base(size_t size) noexcept :size_(size) {}
			constexpr size_t get_size() const noexcept { return size_; }
		private:
			size_t size_;
		};

		template<size_t Extent, size_t Offset, size_t Count>
		struct subextent: std::integral_constant<size_t,
			Count != dynamic_extent
				? Count
				: Extent != dynamic_extent
					? Extent - Offset
					: dynamic_extent> {};
	}

	template<class ElementType, size_t Extent = dynamic_extent>
	class span: private span_impl::base<Extent> {
	public:
		// constants and types
		using element_type = ElementType;
		using value_type = typename std::remove_cv<element_type>::type;
		using size_type = size_t;
		using difference_type = ptrdiff_t;
		using pointer = element_type*;
		using const_pointer = const element_type*;
		using reference = element_type&;
		using const_reference = const element_type&;
		using iterator = pointer;
		using reverse_iterator = std::reverse_iterator<iterator>;
		static constexpr size_type extent = Extent;

		// constructors, copy, and assignment
#define SPANCK(...) typename span_impl::__VA_ARGS__::type = 0
		template<size_t E = Extent, SPANCK(extent_eq<E, 0>)>
		constexpr span() noexcept :span::base(0), data_(nullptr) {}
		template<class It, SPANCK(is_citer<It>)>
		constexpr span(It first, size_type count) :span::base(count), data_(std::addressof(*first)) {}
		template<class It, class End, SPANCK(is_citer_pair<It, End>)>
		constexpr span(It first, End last) :span::base(last - first), data_(std::addressof(*first)) {}
		template<size_t N, size_t E = Extent, SPANCK(extent_eq<E, N>)>
		constexpr span(typename span_impl::type_identity<element_type>::type (&arr)[N]) noexcept :span::base(N), data_(arr) {}
		template<class T, size_t N, size_t E = Extent, SPANCK(extent_eq<E, N>)>
		constexpr span(std::array<T, N>& arr) noexcept :span::base(arr.size()), data_(arr.data()) {}
		template<class T, size_t N, size_t E = Extent, SPANCK(extent_eq<E, N>)>
		constexpr span(const std::array<T, N>& arr) noexcept :span::base(arr.size()), data_(arr.data()) {}
		template<typename R, SPANCK(is_range<R>)>
		constexpr span(R&& r) :span::base(r.size()), data_(r.data()) {}
#undef SPANCK
		constexpr span(const span& other) noexcept = default;
		template<class OtherElementType, size_t OtherExtent>
		constexpr span(const span<OtherElementType, OtherExtent>& other) noexcept :span::base(other.size()), data_(other.data()) {}

		// Not needed and requires __cpp_constexpr_dynamic_alloc >= 201907L
		// constexpr ~span() noexcept = default;

#if __cpp_constexpr >= 201304L
		constexpr
#endif
		span& operator=(const span& other) noexcept = default;

		// subviews
		template<size_t Count>
		constexpr span<element_type, Count> first() const {
			static_assert(Count <= Extent, "Count out of bounds");
			return span<element_type, Count>(data_, Count);
		}
		template<size_t Count>
		constexpr span<element_type, Count> last() const {
			static_assert(Count <= Extent, "Count out of bounds");
			return span<element_type, Count>(data_ + size() - Count, Count);
		}
		template<size_t Offset, size_t Count = dynamic_extent>
		constexpr span<element_type, span_impl::subextent<Extent, Offset, Count>::value> subspan() const {
			static_assert(Offset <= Extent, "Offset out of bounds");
			static_assert(Count == dynamic_extent || Count <= Extent - Offset, "Count out of bounds");
			return span<element_type, span_impl::subextent<Extent, Offset, Count>::value>(
					data_ + Offset,
					Count != dynamic_extent ? Count : size() - Offset);
		}

		constexpr span<element_type, dynamic_extent> first(size_type count) const {
			return span<element_type, dynamic_extent>(data_, count);
		}
		constexpr span<element_type, dynamic_extent> last(size_type count) const {
			return span<element_type, dynamic_extent>(data_ + size() - count, count);
		}
		constexpr span<element_type> subspan(size_type offset, size_type count = dynamic_extent) const {
			return span<element_type, dynamic_extent>(data_ + offset, count == dynamic_extent ? size() - offset : count);
		}

		// observers
		constexpr size_type size() const noexcept { return this->get_size(); }
		constexpr size_type size_bytes() const noexcept { return size() * sizeof(element_type); }
#ifdef __has_cpp_attribute
#  if __has_cpp_attribute(nodiscard) > 201603L
		[[nodiscard]]
#  endif
#endif
		constexpr bool empty() const noexcept { return size() == 0; }

		// element access
		constexpr reference operator[](size_type idx) const { return data_[idx]; }
		constexpr reference front() const { return data_[0]; }
		constexpr reference back() const { return data_[size()-1]; }
		constexpr pointer data() const noexcept { return data_; }

		// iterator support
		constexpr iterator begin() const noexcept { return data_; }
		constexpr iterator end() const noexcept { return data_ + size(); }
		constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); };
		constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); };

	private:
		pointer data_;
	};

#if __cpp_deduction_guides >= 201606L
	template<class It, class EndOrSize>
	span(It, EndOrSize) -> span<std::remove_reference_t<decltype(*std::declval<It>())>>;
	template<class T, size_t N>
	span(T (&)[N]) -> span<T, N>;
	template<class T, size_t N>
	span(std::array<T, N>&) -> span<T, N>;
	template<class T, size_t N>
	span(const std::array<T, N>&) -> span<const T, N>;
	template<class R>
	span(R&&) -> span<std::remove_reference_t<decltype(*std::declval<R>().data())>>;
#endif

#if __cpp_lib_byte >= 201603L && (!defined(_MSC_VER) || _MSC_VER >= 1920)
	template<class ElementType, size_t Extent>
	span<const std::byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>
	as_bytes(span<ElementType, Extent> s) {
		return span<const std::byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>(
			reinterpret_cast<const std::byte *>(s.data()),
			s.size_bytes());
	}
	template<class ElementType, size_t Extent>
	span<std::byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>
	as_writable_bytes(span<ElementType, Extent> s) {
		return span<std::byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>(
			reinterpret_cast<std::byte *>(s.data()),
			s.size_bytes());
	}
#endif

	// rom-properties helpers
	template<class T>
	span<T> reinterpret_span(span<const uint8_t> s) {
		return span<T>(
			reinterpret_cast<T*>(s.data()),
			s.size()/sizeof(T));
	}
	template<class T>
	span<T> reinterpret_span_limit(span<const uint8_t> s, size_t limit) {
		auto ns = reinterpret_span<T>(s);
		return limit < ns.size() ? ns.first(limit) : ns;
	}
}

#endif /* VHVC_SPAN */
