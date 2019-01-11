#ifndef SAFE_PTR_H
#define SAFE_PTR_H

namespace bad {

	void* memset( void* dest, int ch, int count );
}

namespace nodecpp {

template<class T>
class owning_ptr {
	T* ptr;
public:
	owning_ptr(T* ptr = nullptr) :ptr(ptr) {}

	owning_ptr(const owning_ptr&) = delete;
	owning_ptr& operator=(const owning_ptr&) = delete;

	owning_ptr(owning_ptr&&) = default;
	owning_ptr& operator=(owning_ptr&&) = default;

	void reset(T* ptr) {}

	T* get() { return ptr; }
	const T* get() const { return ptr; }
 	T* operator->() { return ptr; }
 	const T* operator->() const { return ptr; }
	T& operator*() { return *ptr; }
	const T& operator*() const { return *ptr; }
};

template<class T>
class soft_ptr {
	T* ptr;
public:
	soft_ptr(T* ptr = nullptr) :ptr(ptr) {}

	soft_ptr(const soft_ptr&) = default;
	soft_ptr& operator=(const soft_ptr&) = default;

	soft_ptr(soft_ptr&&) = default;
	soft_ptr& operator=(soft_ptr&&) = default;

	soft_ptr(const owning_ptr<T>& ow) 
		:ptr(const_cast<T*>(ow.get())) {}
	soft_ptr& operator=(const owning_ptr<T>& ow) { 
		reset(const_cast<T*>(ow.get()));
		return *this;
	};

	void reset(T* ptr) {}

	T* get() { return ptr; }
	const T* get() const { return ptr; }
 	T* operator->() { return ptr; }
 	const T* operator->() const { return ptr; }
	T& operator*() { return *ptr; }
	const T& operator*() const { return *ptr; }
};


template <class T>
class naked_ptr {
	T* ptr;
public:
	naked_ptr(T* ptr = nullptr) :ptr(ptr) {}

	naked_ptr(const naked_ptr&) = default;
	naked_ptr(naked_ptr&&) = default;
	naked_ptr& operator=(const naked_ptr&) = default;
	naked_ptr& operator=(naked_ptr&&) = default;

	naked_ptr(const owning_ptr<T>& ow) 
		:ptr(const_cast<T*>(ow.get())) {}
	naked_ptr& operator=(const owning_ptr<T>& ow) { 
		reset(const_cast<T*>(ow.get()));
		return *this;
	};


	T* get() { return ptr; }
	const T* get() const { return ptr; }
 	T* operator->() { return ptr; }
 	const T* operator->() const { return ptr; }
	T& operator*() { return *ptr; }
	const T& operator*() const { return *ptr; }
};

template<class T, typename ... ARGS>
owning_ptr<T> make_owning(ARGS ... args) {
	// not good, but we can't use std here
	return owning_ptr<T>(new T(args...));
}

}

#endif //SAFE_PTR_H
