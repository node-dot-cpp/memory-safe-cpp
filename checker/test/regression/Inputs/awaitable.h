/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef NODECPP_AWAITABLE_H
#define NODECPP_AWAITABLE_H

namespace std {
	namespace experimental {

		template<typename T0, typename ...T>
		struct coroutine_traits {

				using promise_type = typename T0::promise_type;

		};

		template <typename T = void>
		struct coroutine_handle;

		template <>
		struct coroutine_handle<void> {
			coroutine_handle() = default;
			coroutine_handle(const coroutine_handle&) = default;
			coroutine_handle(coroutine_handle&&) = default;

			static coroutine_handle from_address(void *_Addr) noexcept
			{
				return coroutine_handle{};
			}

		};

		template <typename T>
		struct coroutine_handle : coroutine_handle<>
		{
			coroutine_handle() = default;
			coroutine_handle(const coroutine_handle&) = default;
			coroutine_handle(coroutine_handle&&) = default;

			
		};

		struct suspend_never {
			bool await_ready() noexcept { 
				return false;
			}
			void await_suspend(std::experimental::coroutine_handle<> h_) noexcept {
			}
			void await_resume() { 
			}
		};

		
	}
}

namespace nodecpp {

struct promise_type_struct_base {

    auto initial_suspend() {
        return std::experimental::suspend_never{};
    }
	auto final_suspend() {
		return std::experimental::suspend_never{};
    }
	void unhandled_exception() {
    }
};

template<typename T> struct awaitable; // forward declaration

template<class T>
struct promise_type_struct : public promise_type_struct_base {

	promise_type_struct() : promise_type_struct_base() {}
	promise_type_struct(const promise_type_struct &) = delete;
	promise_type_struct &operator = (const promise_type_struct &) = delete;
	~promise_type_struct() {}

    auto get_return_object();
    auto return_value(T v) {
        return std::experimental::suspend_never{};
    }
    auto yield_value(T v) {
        return std::experimental::suspend_never{};
    }
};

template<>
struct promise_type_struct<void> : public promise_type_struct_base {

	promise_type_struct() : promise_type_struct_base() {}
	promise_type_struct(const promise_type_struct &) = delete;
	promise_type_struct &operator = (const promise_type_struct &) = delete;
	~promise_type_struct() {}

    auto get_return_object();
	auto return_void(void) {
        return std::experimental::suspend_never{};
    }
};


template<typename T>
struct awaitable  {
	using promise_type = promise_type_struct<T>;
	
	awaitable()  {}

    awaitable(const awaitable &) = delete;
	awaitable &operator = (const awaitable &) = delete;

	awaitable(awaitable &&s) = default;
	awaitable &operator = (awaitable &&s) = default;
	
	~awaitable() {}

    T get() {
        return T();
    }

	bool await_ready() noexcept { 
		return false;
	}
	void await_suspend(std::experimental::coroutine_handle<> h_) noexcept {
	}
	T await_resume() { 
		return T(); 
	}

};

inline
auto promise_type_struct<void>::get_return_object() {
		return awaitable<void>{};
}

template<class T>
inline
auto promise_type_struct<T>::get_return_object() {
		return awaitable<T>{};
}

template<typename T>
T wait_for_all(T t, T t2) { return t; }

template<typename T>
T wait_for_all(T t, T t2, T t3) { return t; }

template<typename T>
T wait_for_all(T t, T t2, T t3, T t4) { return t; }

nodecpp::awaitable<void> await_function();
[[nodecpp::no_await]] nodecpp::awaitable<void> no_await_function();


} // namespace nodecpp

#endif // NODECPP_AWAITABLE_H