#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include <assert.h>

#if defined __GNUC__
#define NODECPP_LIKELY(x)       __builtin_expect(!!(x),1)
#define NODECPP_UNLIKELY(x)     __builtin_expect(!!(x),0)
#else
#define NODECPP_LIKELY(x) (x)
#define NODECPP_UNLIKELY(x) (x)
#endif

enum class MemorySafety {none, partial, full};

//#define NODECPP_MEMORYSAFETY_NONE
#define NODECPP_MEMORYSAFETY_EARLY_DETECTION

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

#ifndef NODECPP_MEMORYSAFETY_NONE
#ifdef NODECPP_MEMORYSAFETY_EARLY_DETECTION
//constexpr void* invalid_ptr = (void*)(1);
#endif
#endif


//template<class T> class SoftPtr; // forward declaration

struct Ptr2PtrWishFlags {
private:
	uintptr_t ptr;
	//enum WHICH_BLOCK {IN_1ST_BLOCK = 0, IN_2ND_BLOCK = 1 };
public:
	void set( void* ptr_ ) { ptr = (uintptr_t)ptr_; assert( !isUsed() ); }// reasonable default
	void* getPtr() { return (void*)( ptr & ~((uintptr_t)3) ); }
	void setUsed() { ptr |= 1; }
	void setUnused() { ptr &= ~((uintptr_t)1); }
	bool isUsed() { return ptr & 1; }
	void set1stBlock() { ptr |= 2; }
	void set2ndBlock() { ptr &= ~((uintptr_t)2); }
//	WHICH_BLOCK is1stBlock() { return (WHICH_BLOCK)((ptr & 2)>>1); }
//	static WHICH_BLOCK is1stBlock( uintptr_t ptr ) { return (WHICH_BLOCK)((ptr & 2)>>1); }
	bool is1stBlock() { return (ptr & 2)>>1; }
	static bool is1stBlock( uintptr_t ptr ) { return (ptr & 2)>>1; }
};
static_assert( sizeof(Ptr2PtrWishFlags) == 8 );

struct Ptr2PtrWishData {
//private:
	uintptr_t ptr;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
	static constexpr uintptr_t upperDataMask_ = ~(0xFFFFFFFFFFFFULL);
	static constexpr uintptr_t lowerDataMask_ = 0x7ULL;
	static constexpr uintptr_t upperDataMaskInData_ = 0x7FFF8ULL;
	static constexpr size_t upperDataSize_ = 16;
	static constexpr size_t lowerDataSize_ = 3;
	static constexpr size_t upperDataShift_ = 45;
	static_assert ( (upperDataMaskInData_ << upperDataShift_ ) == upperDataMask_ );
	static_assert ( (ptrMask_ & upperDataMask_) == 0 );
	static_assert ( (ptrMask_ >> (upperDataShift_ + lowerDataSize_)) == 0 );
	static_assert ( (ptrMask_ & lowerDataMask_) == 0 );
	static_assert ( (upperDataMask_ & lowerDataMask_) == 0 );
	static_assert ( (ptrMask_ | upperDataMask_ | lowerDataMask_) == 0xFFFFFFFFFFFFFFFFULL );
public:
	void init( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = (uintptr_t)ptr_; 
	}
	void* getPtr() { return (void*)( ptr & ptrMask_ ); }
	size_t getData() { return ( (ptr & upperDataMask_) >> 45 ) | (ptr & lowerDataMask_); }
	void updatePtr( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr &= ~ptrMask_; 
		ptr |= (uintptr_t)ptr_; }
	void updateData( size_t data ) { 
		assert( data < (1<<(upperDataSize_+lowerDataSize_)) ); 
		ptr &= ptrMask_; 
		ptr |= data & lowerDataMask_; 
		ptr |= (data & upperDataMaskInData_) << upperDataShift_; 
	}
};
static_assert( sizeof(Ptr2PtrWishData) == 8 );

static_assert( sizeof(void*) == 8 );
struct FirstControlBlock // not reallocatable
{
	
	static constexpr size_t maxSlots = 5;
	Ptr2PtrWishFlags* firstFree;
	size_t otherAllockedCnt; // TODO: try to rely on our allocator on deriving this value
	Ptr2PtrWishFlags* otherAllockedSlots;
	Ptr2PtrWishFlags slots[maxSlots];

	void dbgCheckFreeList() {
		Ptr2PtrWishFlags* start = firstFree;
		while( start ) {
			assert( !start->isUsed() );
			assert( ( start->getPtr() == 0 || start->is1stBlock() && (size_t)(start - slots) < maxSlots ) || ( (!start->is1stBlock()) && (size_t)(start - otherAllockedSlots) < otherAllockedCnt ) );
			start = ((Ptr2PtrWishFlags*)(start->getPtr()));
		}
	}

	void init() {
		firstFree = slots;
		for ( size_t i=0; i<maxSlots-1; ++i ) {
			slots[i].set(slots + i + 1);
			slots[i].set1stBlock();
		}
		slots[maxSlots-1].set(nullptr);
		otherAllockedCnt = 0;
		otherAllockedSlots = nullptr;
		assert( !firstFree->isUsed() );
		dbgCheckFreeList();
	}
	size_t insert( void* ptr ) {
		assert( firstFree == nullptr || !firstFree->isUsed() );
		if ( firstFree != nullptr ) {
			Ptr2PtrWishFlags* tmp = ((Ptr2PtrWishFlags*)(firstFree->getPtr()));
			assert( !firstFree->isUsed() );
			size_t idx;
			if ( firstFree->is1stBlock() )
				idx = firstFree - slots;
			else
				idx = maxSlots + otherAllockedSlots - firstFree;
			firstFree->set(ptr);
			firstFree->setUsed();
			firstFree = tmp;
			assert( firstFree == nullptr || !firstFree->isUsed() );
			dbgCheckFreeList();
			return idx;
		}
		else {
			// TODO: reallocate
			assert(false); // TODO+++++
			return -1;
		}
	}
	void resetPtr( size_t idx, void* newPtr ) {
		if ( idx < maxSlots ) {
			slots[idx].set( newPtr );
			slots[idx].setUsed();
			slots[idx].set1stBlock();
		}
		else {
			assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots[idx].set( newPtr );
			otherAllockedSlots[idx].setUsed();
			otherAllockedSlots[idx].set1stBlock();
		}
		dbgCheckFreeList();
		assert( firstFree == nullptr || !firstFree->isUsed() );
	}
	void remove( size_t idx ) {
		assert( firstFree == nullptr || !firstFree->isUsed() );
		if ( idx < maxSlots ) {
			slots[idx].set( firstFree );
			firstFree = slots + idx;
			firstFree->setUnused();
			firstFree->set1stBlock();
		}
		else {
			assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots[idx].set( firstFree );
			firstFree = otherAllockedSlots + idx;
			firstFree->setUnused();
			firstFree->set2ndBlock();
		}
		assert( firstFree == nullptr || !firstFree->isUsed() );
		dbgCheckFreeList();
	}
	void clear() {
	}
};
static_assert( sizeof(FirstControlBlock) == 64 );

template<class T, bool isSafe> class SoftPtr; // forward declaration

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class OwningPtr
{
	friend class SoftPtr<T, isSafe>;

	T* t;
	FirstControlBlock* getControlBlock() { return reinterpret_cast<FirstControlBlock*>(t) - 1; }

	void updatePtrForListItems( T* t_ )
	{
		FirstControlBlock* cb = getControlBlock();
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
				reinterpret_cast<SoftPtr<T, isSafe>*>(cb->slots[i].getPtr())->t = t_;
		for ( size_t i=0; i<cb->otherAllockedCnt; ++i )
			if ( cb->otherAllockedSlots[i].isUsed() )
				reinterpret_cast<SoftPtr<T, isSafe>*>(cb->otherAllockedSlots[i].getPtr())->t = t_;
	}

	void updatePtrForListItemsWithInvalidPtr() { updatePtrForListItems( nullptr ); }

public:
	OwningPtr()
	{
		t = nullptr;
	}
	OwningPtr( T* t_ )
	{
		t = t_;
		getControlBlock()->init();
	}
	OwningPtr( OwningPtr<T, isSafe>& other ) = delete;
	OwningPtr( OwningPtr<T, isSafe>&& other ) = default;
	OwningPtr& operator = ( OwningPtr<T, isSafe>& other ) = delete;
	OwningPtr& operator = ( OwningPtr<T, isSafe>&& other ) = default;
	~OwningPtr()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
			updatePtrForListItemsWithInvalidPtr();
			delete [] getControlBlock();
#ifdef NODECPP_MEMORYSAFETY_EARLY_DETECTION
			t = nullptr;
#endif
		}
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
			updatePtrForListItemsWithInvalidPtr();
			delete [] getControlBlock();
			t = nullptr;
		}
	}

	void reset( T* t_ ) // Q: what happens to safe ptrs?
	{
		T* tmp = t;
		t = t_;
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
			delete [] getControlBlock();
			if ( NODECPP_LIKELY(t != t_) )
			{
				t = t_;
				updatePtrForListItems( t_ );
			}
			else
			{
				t = nullptr;
				updatePtrForListItemsWithInvalidPtr();
			}
		}
		else
		{
			t = t_;
			updatePtrForListItems( t_ );
		}
	}

	void swap( OwningPtr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		// if zero guard page... if constexpr( sizeof(T)<4096); then add signal handler
		//todo: pair<ptr,sz> realloc(ptr,minsise,desiredsize); also: size_t try_inplace_realloc(ptr,minsise,desiredsize)
		//<bool isSafe=NODECPP_ISSAFE()>
		assert( t != nullptr );
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};

#if 0
template<class T>
class OwningPtr<T>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class SoftPtr<T>;
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
	OwningPtr( OwningPtr<T, false>& other ) = delete;
	OwningPtr( OwningPtr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
	}
	~OwningPtr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: being placed at the level of class definition, the codition may be checked whether or not this specialization is instantiated (see, for instance, https://stackoverflow.com/questions/5246049/c11-static-assert-and-template-instantiation)
		if ( NODECPP_LIKELY(t) )
		{
			delete t;
		}
	}

	OwningPtr& operator = ( OwningPtr<T, false>& other ) = delete;
	OwningPtr& operator = ( OwningPtr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
		return *this;
	}

	void reset( T* t_ = t )
	{
		T* tmp = t;
		t = t_;
		// if ( NODECPP_LIKELY(tmp) ) : we do not need this check
		delete tmp;
	}

	void swap( OwningPtr<T, false>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};
#endif // 0

template<bool _Test,
	class _Ty = void>
	using enable_if_t = typename std::enable_if<_Test, _Ty>::type;

template<class _Ty>
	_INLINE_VAR constexpr bool is_array_v = std::is_array<_Ty>::value;

template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int> = 0>
	_NODISCARD inline OwningPtr<_Ty> make_owning(_Types&&... _Args)
	{	// make a unique_ptr
	uint8_t* data = new uint8_t[ sizeof(FirstControlBlock) + sizeof(_Ty) ];
	_Ty* objPtr = new ( data + sizeof(FirstControlBlock) ) _Ty(_STD forward<_Types>(_Args)...);
	return OwningPtr<_Ty>(objPtr);
//	return (OwningPtr<_Ty>(new _Ty(_STD forward<_Types>(_Args)...)));
	}


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class SoftPtr
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class OwningPtr<T, isSafe>;

	T* t;
	size_t idx;
	FirstControlBlock* getControlBlock() { return reinterpret_cast<FirstControlBlock*>(t) - 1; }

	void dbgValidateList() const
	{
#if 0
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
#endif // 0
	}

public:
	SoftPtr()
	{
		this->t = nullptr;
		idx = (size_t)(-1);
	}
	SoftPtr( OwningPtr<T, isSafe>& owner )
	{
		this->t = owner.t;
		idx = getControlBlock()->insert(this);
	}
	SoftPtr( SoftPtr<T, isSafe>& other )
	{
		this->t = other.t;
		idx = getControlBlock()->insert(this);
	}
	SoftPtr( SoftPtr<T, isSafe>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		this->idx = other.idx;
		other.idx = (size_t)(-1);
		getControlBlock()->resetPtr(this->idx, this);
	}

	SoftPtr& operator = ( SoftPtr<T, isSafe>& other )
	{
		this->t = other.t;
		idx = getControlBlock()->insert(this);
		return *this;
	}
	SoftPtr& operator = ( SoftPtr<T, isSafe>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		this->idx = other.idx;
		other.idx = (size_t)(-1);
		getControlBlock()->resetPtr(this->idx, this);
		return *this;
	}

	void swap( SoftPtr<T, isSafe>& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
		size_t idx = this->idx;
		this->idx = other.idx;
		other.idx = idx;
		if ( this->t )
			getControlBlock()->resetPtr(this->idx, this);
		if ( other.t )
			other.getControlBlock()->resetPtr(other.idx, &other);
	}

	T* get() const
	{
		assert( this->t != nullptr );
		return this->t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	~SoftPtr()
	{
		if( this->t != nullptr ) {
			assert( this->idx != (size_t)(-1) );
			getControlBlock()->remove(idx);
			this->t = nullptr;
		}
	}
};

#if 0
template<class T>
class SoftPtr<T,false>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class OwningPtr<T,false>;
	T* t;

public:
	SoftPtr()
	{
		this->t = nullptr;
	}
	SoftPtr( OwningPtr<T,false>& owner )
	{
		this->t = owner.t;
	}
	SoftPtr( SoftPtr<T,false>& other )
	{
		this->t = other.t;
	}
	SoftPtr( SoftPtr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
	}

	SoftPtr& operator = ( SoftPtr<T,false>& other )
	{
		this->t = other.t;
		return *this;
	}
	SoftPtr& operator = ( SoftPtr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		return *this;
	}

	void swap( SoftPtr<T, false>& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		return this->t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	~SoftPtr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial );
	}
};
#endif // 0

#endif
