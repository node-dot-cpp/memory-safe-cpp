#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include <assert.h>
#include "iibmalloc/iibmalloc.h"

#if defined __GNUC__
#define NODECPP_LIKELY(x)       __builtin_expect(!!(x),1)
#define NODECPP_UNLIKELY(x)     __builtin_expect(!!(x),0)
#else
#define NODECPP_LIKELY(x) (x)
#define NODECPP_UNLIKELY(x) (x)
#endif

enum class MemorySafety {none, partial, full};

#ifdef NODECPP_MEMORYSAFETY_NONE
#define NODECPP_ISSAFE_MODE MemorySafety::none
#define NODECPP_ISSAFE_DEFAULT false
#elif defined NODECPP_MEMORYSAFETY_PARTIAL
#define NODECPP_ISSAFE_MODE MemorySafety::partial
#define NODECPP_ISSAFE_DEFAULT true
#elif defined NODECPP_MEMORYSAFETY_FULL
#define NODECPP_ISSAFE_MODE MemorySafety::full
#define NODECPP_ISSAFE_DEFAULT true
#else
#define NODECPP_ISSAFE_MODE MemorySafety::full
#define NODECPP_ISSAFE_DEFAULT true
#endif


template<class T, bool isSafe> class SoftPtr; // forward declaration

template<class T>
struct SoftPtrBase
{
	SoftPtrBase* prev;
	SoftPtrBase* next;
	T* t;
};

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class OwningPtr
{
	friend class SoftPtr<T, isSafe>;
	SoftPtrBase<T> head;

	void updatePtrForListItems( T* t_ )
	{
		SoftPtrBase<T>* next = head.next;
		while( next )
		{
			next->t = t_;
			next = next->next;
		}
	}

	void dbgValidateList() const
	{
		const SoftPtrBase<T>* tmp = &head;
		assert( tmp == nullptr || tmp->prev == nullptr );
		assert( tmp == nullptr || tmp->t == head.t );
		const SoftPtrBase<T>* tmpPrev = tmp;
		tmp = tmp->next;
		while( tmp )
		{
			assert( tmp->prev == tmpPrev );
			assert( tmp->t == head.t );
			tmpPrev = tmp;
			tmp = tmp->next;
		}
	}

public:
	OwningPtr()
	{
		head.prev = nullptr;
		head.next = nullptr;
		head.t = nullptr;
	}
	OwningPtr( T* t_ )
	{
		head.prev = nullptr;
		head.next = nullptr;
		head.t = t_;
	}
	OwningPtr( OwningPtr& other ) = delete;
	OwningPtr( OwningPtr&& other )
	{
		head.next = other.head.next;
		other.head.next = nullptr;
		head.t = other.head.t;
		other.head.t = nullptr;
		dbgValidateList();
	}
	~OwningPtr()
	{
		dbgValidateList();
		if ( NODECPP_LIKELY(head.t) )
		{
			delete head.t;
			updatePtrForListItems( nullptr );
		}
	}

	void reset()
	{
		if ( NODECPP_LIKELY(head.t) )
		{
			delete head.t;
			head.t = nullptr;
			updatePtrForListItems( nullptr );
		}
	}

	void reset( T* t_ )
	{
		T* tmp = head.t;
		head.t = t_;
		if ( NODECPP_LIKELY(head.t) )
		{
			delete head.t;
			if ( NODECPP_LIKELY(head.t != t_) )
			{
				head.t = t_;
				updatePtrForListItems( t_ );
			}
			else
			{
				head.t = nullptr;
				updatePtrForListItems( nullptr );
			}
		}
		else
		{
			head.t = t_;
			updatePtrForListItems( t_ );
		}
	}

	void swap( OwningPtr& other )
	{
		T* tmp = head.t;
		head.t = other.head.t;
		other.head.t = tmp;
		auto tmpHead = head.next;
		head.next = other.head.next;
		other.head.next = tmpHead->next;
		dbgValidateList();
	}

	T* get() const
	{
		// if zero guard page... if constexpr( sizeof(T)<4096); then add signal handler
		//todo: pair<ptr,sz> realloc(ptr,minsise,desiredsize); also: size_t try_inplace_realloc(ptr,minsise,desiredsize)
		//<bool isSafe=NODECPP_ISSAFE()>
		assert( head.t != nullptr );
		return head.t;
	}

	T* release() // TODO: check necessity
	{
		assert( head.t != nullptr );
		T* ret =  head.t;
		head.t = nullptr;
		updatePtrForListItems( nullptr );
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return head.t != nullptr;
	}
};

template<class T>
class OwningPtr<T, false>
{
	friend class SoftPtr<T, false>;
	T* t;

public:
	OwningPtr()
	{
		t = nullptr;
	}
	OwningPtr( T* t_ )
	{
		t = t_;
	}
	OwningPtr( OwningPtr& other ) = delete;
	OwningPtr( OwningPtr&& other )
	{
		t = other.t;
		other.head.t = nullptr;
	}
	~OwningPtr()
	{
		if ( NODECPP_LIKELY(head.t) )
		{
			delete head.t;
		}
	}

	void reset( T* t_ = t )
	{
		T* tmp = head.t;
		head.t = t_;
		if ( NODECPP_LIKELY(tmp) )
			delete head.t;
	}

	void swap( OwningPtr& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		return t;
	}

	T* release() // TODO: check necessity
	{
		T* ret =  head.t;
		head.t = nullptr;
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return head.t != nullptr;
	}
};

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class SoftPtr : protected SoftPtrBase<T>
{
	friend class OwningPtr<T>;

	void removeFromList()
	{
		if ( this->next )
			this->next->prev = this->prev;
		if ( this->prev )
			this->prev->next = this->next;
	}

	void dbgValidateList() const
	{
		const SoftPtrBase<T>* tmp = this->next;
		assert( tmp == nullptr || tmp->prev == this );
		assert( tmp == nullptr || tmp->t == this->t );

		if ( tmp )
		{
			const SoftPtrBase<T>* tmpPrev = tmp;
			tmp = tmp->next;
			while( tmp )
			{
				assert( tmp->prev == tmpPrev );
				assert( tmp->t == this->t );
				tmpPrev = tmp;
				tmp = tmp->next;
			}
		}

		tmp = this->prev;
		if ( tmp )
		{
			const SoftPtrBase<T>* tmpPrev = tmp;
			tmp = tmp->prev;
			while( tmp )
			{
				assert( tmp->next == tmpPrev );
				assert( tmp->t == this->t );
				tmpPrev = tmp;
				tmp = tmp->prev;
			}
		}
	}

public:
	SoftPtr()
	{
		this->t = nullptr;
		this->next = nullptr;
		this->prev = nullptr;
	}
	SoftPtr( OwningPtr<T>& owner )
	{
		this->t = owner.head.t;
		this->next = owner.head.next;
		if ( owner.head.next )
			owner.head.next->prev = this;
		this->prev = &(owner.head);
		owner.head.next = this;
		dbgValidateList();
	}
	SoftPtr( SoftPtr<T>& other )
	{
		this->t = other.t;
		this->next = &other;
		this->prev = other.prev;
		other.prev->next = this;
		other.prev = this;
		dbgValidateList();
	}
	SoftPtr( SoftPtr<T>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		this->next = other.next;
		this->prev = other.prev;
		if ( other.prev )
			other.prev.next = this;
		if ( other.next )
			other.next.prev = this;
		other.prev = nullptr;
		other.next = nullptr;
		dbgValidateList();
	}
	void swap( SoftPtr& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
		auto tmpLP = this->prev;
		this->prev = other.prev;
		other.prev = tmpLP;
		tmpLP = this->next;
		this->next = other.next;
		other.next = tmpLP;
		if ( other.prev )
			other.prev->next = &other;
		if ( other.next )
			other.next->prev = &other;
		if ( this->prev )
			this->prev->next = this;
		if ( this->next )
			this->next->prev = this;
		dbgValidateList();
	}
	~SoftPtr()
	{
		dbgValidateList();
		this->t = nullptr;
		removeFromList();
	}

	T* get() const
	{
		assert( this->t != nullptr );
		return this->t;
	}

	T* release() // TODO: check necessity
	{
		assert( this->t != nullptr );
		T* ret =  this->t;
		this->t = nullptr;
		removeFromList();
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}
};

template<class T>
class SoftPtr<T,false>
{
	friend class OwningPtr<T,false>;

public:
	SoftPtr()
	{
		this->t = nullptr;
	}
	SoftPtr( OwningPtr<T>& owner )
	{
		this->t = owner.head.t;
	}
	SoftPtr( SoftPtr<T>& other )
	{
		this->t = other.t;
	}
	SoftPtr( SoftPtr<T>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
	}
	void swap( SoftPtr& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
	}
	~SoftPtr()
	{
	}

	T* get() const
	{
		return this->t;
	}

	T* release() // TODO: check necessity
	{
		T* ret =  this->t;
		this->t = nullptr;
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}
};

#endif
