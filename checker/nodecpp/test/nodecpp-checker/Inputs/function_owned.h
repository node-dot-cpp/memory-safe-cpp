#ifndef FUNCTION_OWNED_H
#define FUNCTION_OWNED_H

namespace std {

typedef void* nullptr_t;

template<class T>
T&& move(T&& t) { return t; }

template<class T>
class reference_wrapper {};

class type_info {};

// mock version of std::function

template <typename>
class function; // no definition but needed

template< class R, class... Args >
class function<R(Args...)> {
	type_info ti;
public:
	typedef R result_type;

	function() noexcept {}
	function( std::nullptr_t np ) noexcept {}

	function( const function& other ) {}

	function( function&& other ) {}

	template< class F > 
	function( F f ) {}

	function& operator=( const function& other ) { return *this; }
	function& operator=( function&& other ) { return *this; }
	function& operator=( std::nullptr_t np ) { return *this; }

	template< class F > 
	function& operator=( F&& f ) { return *this; }

	template< class F > 
	function& operator=( std::reference_wrapper<F> f ) { return *this; }

	void swap( function& other ) noexcept {}
	explicit operator bool() const noexcept { return false; }
	R operator()( Args... args ) const { return R(); }

	const std::type_info& target_type() const noexcept { return ti; }

	template< class T > 
	T* target() noexcept { return nullptr; }

	template< class T > 
	const T* target() const noexcept { return nullptr; }
};

template< class R, class... Args >
void swap( function<R(Args...)> &lhs, function<R(Args...)> &rhs ) {}

template< class R, class... ArgTypes >
bool operator==( const function<R(ArgTypes...)>& f, std::nullptr_t ) noexcept { return false; }

template< class R, class... ArgTypes >
bool operator==( std::nullptr_t, const function<R(ArgTypes...)>& f ) noexcept { return false; }

template< class R, class... ArgTypes >
bool operator!=( const function<R(ArgTypes...)>& f, std::nullptr_t ) noexcept { return false; }

template< class R, class... ArgTypes >
bool operator!=( std::nullptr_t, const function<R(ArgTypes...)>& f ) noexcept { return false; }

}


namespace nodecpp {

template <typename>
class function_owned_arg0; // no definition but needed

template< class R, class... Args >
class function_owned_arg0<R(Args...)> {
	std::function<R(Args...)> stdf;
public:
	typedef typename std::function<R(Args...)>::result_type result_type;

	function_owned_arg0() noexcept {}
	function_owned_arg0( std::nullptr_t np ) noexcept :stdf(np) {}

	function_owned_arg0( const function_owned_arg0& other ) :stdf(other.stdf) {}

	function_owned_arg0( function_owned_arg0&& other ) : stdf(std::move(other.stdf)) {}

	template< class F > 
	function_owned_arg0( F f ) :stdf(std::move(f)) {}

	function_owned_arg0& operator=( const function_owned_arg0& other ) { stdf.operator=(other.stdf); return *this; }
	function_owned_arg0& operator=( function_owned_arg0&& other ) { stdf.operator=(std::move(other.stdf)); return *this; }
	function_owned_arg0& operator=( std::nullptr_t np ) { stdf.operator=(np); return *this; }

	template< class F > 
	function_owned_arg0& operator=( F&& f ) { stdf.operator=(std::move(f)); return *this; }

	template< class F > 
	function_owned_arg0& operator=( std::reference_wrapper<F> f ) { stdf.operator=(f); return *this; }

	void swap( function_owned_arg0& other ) noexcept { stdf.swap(other.stdf); }
	explicit operator bool() const noexcept { return static_cast<bool>(stdf); }
	R operator()( Args... args ) const { return stdf.operator()(std::forward<Args...>(args...)); }

	const std::type_info& target_type() const noexcept { return stdf.target_type(); }

	template< class T > 
	T* target() noexcept { return stdf.target<T>(); }

	template< class T > 
	const T* target() const noexcept { return stdf.target<T>(); }
};

template< class R, class... Args >
void swap( function_owned_arg0<R(Args...)> &lhs, function_owned_arg0<R(Args...)> &rhs ) { lhs.swap(rhs); }

template< class R, class... ArgTypes >
bool operator==( const function_owned_arg0<R(ArgTypes...)>& f, std::nullptr_t ) noexcept { return !f; }

template< class R, class... ArgTypes >
bool operator==( std::nullptr_t, const function_owned_arg0<R(ArgTypes...)>& f ) noexcept { return !f; }

template< class R, class... ArgTypes >
bool operator!=( const function_owned_arg0<R(ArgTypes...)>& f, std::nullptr_t ) noexcept { return (bool) f; }

template< class R, class... ArgTypes >
bool operator!=( std::nullptr_t, const function_owned_arg0<R(ArgTypes...)>& f ) noexcept { return (bool) f; }

}

#endif //FUNCTION_OWNED_H
