/**
 * The 3-Clause BSD License
 * Copyright (c) 2019 -- Elie Michel
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This is LilyFuture.h -- https://github.com/eliemichel/LilyFuture
 */

#pragma once

#include <future>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Element Packing
///////////////////////////////////////////////////////////////////////////////

// Many thx to https://stackoverflow.com/questions/5484930/split-variadic-template-arguments

// make_pack_indices<5, 0>::type is actually the type pack_indices<0, 1, 2, 3, 4>

template <size_t...>
struct pack_indices {};

template <size_t Sp, class IntPack, size_t Ep>
struct make_indices_imp;

template <size_t Sp, size_t ... Indices, size_t Ep>
struct make_indices_imp<Sp, pack_indices<Indices...>, Ep>
{
	typedef typename make_indices_imp<Sp + 1, pack_indices<Indices..., Sp>, Ep>::type type;
};

template <size_t Ep, size_t ... Indices>
struct make_indices_imp<Ep, pack_indices<Indices...>, Ep>
{
	typedef pack_indices<Indices...> type;
};

template <size_t Ep, size_t Sp = 0>
struct make_pack_indices
{
	static_assert(Sp <= Ep, "__make_tuple_indices input error");
	typedef typename make_indices_imp<Sp, pack_indices<>, Ep>::type type;
};

// pack_element<N, Ts...>::type is the Nth type of the parameters pack

template <size_t _Ip, class _Tp>
class pack_element_imp;

template <class ..._Tp>
struct pack_types {};

template <size_t Ip>
class pack_element_imp<Ip, pack_types<> >
{
public:
	static_assert(Ip == 0, "tuple_element index out of range");
	static_assert(Ip != 0, "tuple_element index out of range");
};

template <class Hp, class ...Tp>
class pack_element_imp<0, pack_types<Hp, Tp...> >
{
public:
	typedef Hp type;
};

template <size_t Ip, class Hp, class ...Tp>
class pack_element_imp<Ip, pack_types<Hp, Tp...> >
{
public:
	typedef typename pack_element_imp<Ip - 1, pack_types<Tp...> >::type type;
};

template <size_t Ip, class ...Tp>
class pack_element
{
public:
	typedef typename pack_element_imp<Ip, pack_types<Tp...> >::type type;
};

// get<N>(ts...) return the Nth element of a parameters pack.

template <class R, size_t Ip, size_t Ij, class... Tp>
struct Get_impl
{
	static R& dispatch(Tp...);
};

template<class R, size_t Ip, size_t Jp, class Head, class... Tp>
struct Get_impl<R, Ip, Jp, Head, Tp...>
{
	static R& dispatch(Head& h, Tp&... tps)
	{
		return Get_impl<R, Ip, Jp + 1, Tp...>::dispatch(tps...);
	}
};

template<size_t Ip, class Head, class... Tp>
struct Get_impl<Head, Ip, Ip, Head, Tp...>
{
	static Head& dispatch(Head& h, Tp&... tps)
	{
		return h;
	}
};


template <size_t Ip, class ... Tp>
typename pack_element<Ip, Tp...>::type&
get(Tp&... tps)
{
	return Get_impl<typename pack_element<Ip, Tp...>::type, Ip, 0, Tp...>::dispatch(tps...);
}

///////////////////////////////////////////////////////////////////////////////
// Internal Implementations
///////////////////////////////////////////////////////////////////////////////

// TODO: enable other possible thread pooling mechanisms instead of std::async
// Thanks Evg for helping https://stackoverflow.com/questions/58766480/sum-type-in-variadic-template

// present<T> remove any future prefix from type
// For instance, if T is a common type
// T == present<T> == present<future<T>> == present<future<future<T>>> == ...
// Same for shared_future
template <typename A> struct present                        { typedef A type; };
template <typename A> struct present<std::future<A>>        { typedef A type; };
template <typename A> struct present<std::shared_future<A>> { typedef A type; };
template <typename A> using Present = typename present<A>::type;

// Reccursively get futures until the type gets to a present type
template <typename A>
Present<A> getRec(A a) {
	return a;
}
template <typename A>
Present<A> getRec(std::future<A> fA) {
	return getRec(fA.get());
}
template <typename A>
Present<A> getRec(std::shared_future<A> fA) {
	return getRec(fA.get());
}

// Arguments allowed in variadic list of then() are shared_future or future&
template<typename>
struct is_future_or_shared_future : std::false_type {};
template<typename T>
struct is_future_or_shared_future<std::future<T>> : std::true_type {};
template<typename T>
struct is_future_or_shared_future<std::shared_future<T>> : std::true_type {};

template<class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// Shared futures are copied while raw futures are moved
template <typename A>
std::future<A> && move_or_copy(std::future<A> & a) { return std::move(a); }
template <typename A>
std::shared_future<A> move_or_copy(std::shared_future<A> & a) { return a; }

// unpack futures
template<
	typename F, typename ...FutureA,
	typename B = typename std::result_of<F(Present<std::remove_reference_t<FutureA>>...)>::type,
	typename = typename std::enable_if<!std::is_void<B>::value>::type,
	typename = std::enable_if_t<std::conjunction_v<is_future_or_shared_future<FutureA>...>>
>
static std::future<Present<B>> then_imp_imp(F&& handler, FutureA&... f) {
	return std::async([handler](FutureA&... f) {
		return getRec(handler(f.get()...));
	}, move_or_copy<Present<std::remove_reference_t<FutureA>>>(f)...);
}
// unpack futures (void specialization)
template<
	typename F, typename ...FutureA,
	typename B = typename std::result_of<F(Present<remove_cvref_t<FutureA>>...)>::type,
	typename = typename std::enable_if<std::is_void<B>::value>::type,
	typename = std::enable_if_t<std::conjunction_v<is_future_or_shared_future<FutureA>...>>
>
static std::future<void> then_imp_imp(F&& handler, FutureA&... f) {
	return std::async([handler](FutureA&... f) {
		handler(f.get()...);
	}, move_or_copy<Present<std::remove_reference_t<FutureA>>>(f)...);
}

// unpack args at given indices
template<typename F, size_t ...Indices, typename ...A>
static auto then_imp(F&& handler, pack_indices<Indices...>, A&&... args) {
	return then_imp_imp(handler, get<Indices>(args...)...);
}

///////////////////////////////////////////////////////////////////////////////
// Public API
///////////////////////////////////////////////////////////////////////////////

/**
 * then() takes some futures or shared futures and a callback to run when all values are available.
 * It requires a future to the type returned by the callback.
 * It may also be called "bind()" or "require()" depending on you mindset.
 * Use shared futures when they occure in several calls to "then", raw futures otherwise.
 */
template<
	typename ...All,
	size_t N = sizeof...(All),
	typename Fun = typename pack_element<N - 1, All...>::type,
	typename Range = typename make_pack_indices<N - 1>::type
>
auto then(All&... all) {
	Range indices;
	return then_imp(get<N - 1>(all...), indices, all...);
}

/**
 * all() transforms a vector of futures to a future vector, ready when all futures are.
 */
template<typename A>
std::future<std::vector<A>> all(std::vector<std::future<A>> & futures) {
	return std::async([](std::vector<std::future<A>> futures) {
		std::vector<A> v;
		v.reserve(futures.size());
		for (auto & f : futures) {
			v.push_back(f.get());
		}
		return v;
	}, std::move(futures));
}

