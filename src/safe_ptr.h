#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include <memory>
#include <stdint.h>
#include <assert.h>

#define USE_IIBMALLOC

#ifdef USE_IIBMALLOC
#include "iibmalloc/src/iibmalloc.h"
FORCE_INLINE void* allocate( size_t sz ) { return g_AllocManager.allocate( sz ); }
FORCE_INLINE void deallocate( void* ptr ) { g_AllocManager.deallocate( ptr ); }
FORCE_INLINE void* zombieAllocate( size_t sz ) { return g_AllocManager.zombieableAllocate( sz ); }
FORCE_INLINE void zombieDeallocate( void* ptr ) { g_AllocManager.zombieableDeallocate( ptr ); }
FORCE_INLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return g_AllocManager.isZombieablePointerInBlock( allocatedPtr, ptr ); }
FORCE_INLINE constexpr size_t getPrefixByteCount() { return guaranteed_prefix_size; }
#else
inline void* allocate( size_t sz ) { return new uint8_t[ sz ]; }
inline void deallocate( void* ptr ) { delete [] ptr; }
inline void* zombieAllocate( size_t sz ) { uint8_t* ret = new uint8_t[ sizeof(uint64_t) + sz ]; *reinterpret_cast<uint64_t*>(ret) = sz; return ret + sizeof(uint64_t);}
inline void zombieDeallocate( void* ptr ) { delete [] (reinterpret_cast<uint8_t*>(ptr) - sizeof(uint64_t)); }
inline bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return ptr >= allocatedPtr && reinterpret_cast<uint8_t*>(allocatedPtr) + *(reinterpret_cast<uint64_t*>(allocatedPtr) - 1) > reinterpret_cast<uint8_t*>(ptr); }
inline constexpr size_t getPrefixByteCount() { return 0; }
#endif


#if defined __GNUC__
#define NODECPP_LIKELY(x)       __builtin_expect(!!(x),1)
#define NODECPP_UNLIKELY(x)     __builtin_expect(!!(x),0)
#else
#define NODECPP_LIKELY(x) (x)
#define NODECPP_UNLIKELY(x) (x)
#endif

//#define NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST

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

#define CONTROL_BLOCK_SIZE 4096 // TODO: make platform-dependent consideration

template<class T>
bool isGuaranteedOnStack( T* ptr )
{
	if constexpr ( sizeof(T) <= CONTROL_BLOCK_SIZE )
	{
		int a;
		constexpr uintptr_t upperBitsMask = ~( CONTROL_BLOCK_SIZE - 1 );
		return ( ( ((uintptr_t)(ptr)) ^ ((uintptr_t)(&a)) ) & upperBitsMask ) == 0;
	}
	else {
		return false;
	}
}

template<class T>
void checkNotNullLargeSize( T* ptr )
{
	if constexpr ( sizeof(T) <= CONTROL_BLOCK_SIZE ) ;
	else {
		if ( ptr == nullptr )
			throw std::bad_alloc();
	}
}

template<class T>
void checkNotNullAllSizes( T* ptr )
{
	if ( ptr == nullptr )
		throw std::bad_alloc();
}

inline
void throwPointerOutOfRange()
{
	// TODO: actual implementation
	throw std::bad_alloc();
}


struct Ptr2PtrWishFlags {
private:
	uintptr_t ptr;
public:
	void set( void* ptr_ ) { ptr = (uintptr_t)ptr_; assert( !isUsed() ); }// reasonable default
	void* getPtr() { return (void*)( ptr & ~((uintptr_t)3) ); }
	void setUsed() { ptr |= 1; }
	void setUnused() { ptr &= ~((uintptr_t)1); }
	bool isUsed() { return ptr & 1; }
	void set1stBlock() { ptr |= 2; }
	void set2ndBlock() { ptr &= ~((uintptr_t)2); }
	bool is1stBlock() { return (ptr & 2)>>1; }
	static bool is1stBlock( uintptr_t ptr ) { return (ptr & 2)>>1; }
};
static_assert( sizeof(Ptr2PtrWishFlags) == 8 );

struct Ptr2PtrWishDataBase {
//private:
	uintptr_t ptr;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
	static constexpr uintptr_t upperDataMask_ = ~(0xFFFFFFFFFFFFULL);
	static constexpr uintptr_t lowerDataMask_ = 0x7ULL;
	static constexpr uintptr_t upperDataMaskInData_ = 0x7FFF8ULL;
	static constexpr size_t upperDataSize_ = 16;
	static constexpr size_t lowerDataSize_ = 3;
	static constexpr size_t upperDataShift_ = 45;
	static constexpr size_t invalidData = ( lowerDataMask_ | upperDataMaskInData_ );
	static_assert ( (upperDataMaskInData_ << upperDataShift_ ) == upperDataMask_ );
	static_assert ( (ptrMask_ & upperDataMask_) == 0 );
	static_assert ( (ptrMask_ >> (upperDataShift_ + lowerDataSize_)) == 0 );
	static_assert ( (ptrMask_ & lowerDataMask_) == 0 );
	static_assert ( (upperDataMask_ & lowerDataMask_) == 0 );
	static_assert ( (ptrMask_ | upperDataMask_ | lowerDataMask_) == 0xFFFFFFFFFFFFFFFFULL );
public:
	void init( void* ptr_, size_t data ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = (uintptr_t)ptr_; 
		assert( data < (1<<(upperDataSize_+lowerDataSize_)) ); 
		ptr |= data & lowerDataMask_; 
		ptr |= (data & upperDataMaskInData_) << upperDataShift_; 
	}
	void* getPtr() const { return (void*)( ptr & ptrMask_ ); }
	size_t getData() const { return ( (ptr & upperDataMask_) >> 45 ) | (ptr & lowerDataMask_); }
	size_t get3bitBlockData() const { return ptr & lowerDataMask_; }
	uintptr_t getLargerBlockData() const { return ptr & upperDataMask_; }
	void updatePtr( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr &= ~ptrMask_; 
		ptr |= (uintptr_t)ptr_; 
	}
	void updateData( size_t data ) { 
		assert( data < (1<<(upperDataSize_+lowerDataSize_)) ); 
		ptr &= ptrMask_; 
		ptr |= data & lowerDataMask_; 
		ptr |= (data & upperDataMaskInData_) << upperDataShift_; 
	}
	void update3bitBlockData( size_t data ) { 
		assert( data < 8 ); 
		ptr &= ~lowerDataMask_; 
		ptr |= data & lowerDataMask_; 
	}
	void updateLargerBlockData( size_t data ) { 
		assert( (data & ~upperDataMaskInData_) == 0 ); 
		ptr &= ~upperDataMaskInData_; 
		ptr |= data & lowerDataMask_; 
		ptr |= data & upperDataMask_; 
	}
	void swap( Ptr2PtrWishDataBase& other ) { uintptr_t tmp = ptr; ptr = other.ptr; other.ptr = tmp; }
};


#ifdef NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST
struct Ptr2PtrWishData {
	static constexpr size_t invalidData = (size_t)(-1);
	void* ptr;
	size_t data;
	void init( void* ptr_, size_t data_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = ptr_; 
		data = data_;
	}
	void* getPtr() const { return ptr; }
	size_t getData() const { return data; }
	void updatePtr( void* ptr_ ) { ptr = ptr_ }
	void updateData( size_t data ) { data = data_; }
	void swap( Ptr2PtrWishData& other ) { 
		void* tmpPtr = ptr; ptr = other.ptr; other.ptr = tmpPtr; 
		size_t tmpData = data; data = other.data; other.data = tmpData; 
	}
};
	size_t idx;
	T* getPtr_() const { return t; }
	void setPtr_( T* t_ ) { t = t_; }
	size_t getIdx_() const { return idx; }
#else
struct Ptr2PtrWishData : public Ptr2PtrWishDataBase {};
static_assert( sizeof(Ptr2PtrWishData) == 8 );
#endif

static_assert( sizeof(void*) == 8 );

struct FirstControlBlock // not reallocatable
{
	struct SecondCBHeader
	{
		static constexpr size_t secondBlockStartSize = 8;	
		Ptr2PtrWishFlags* firstFree;
		size_t otherAllockedCnt;
		Ptr2PtrWishFlags slots[1];
		void addToFreeList( Ptr2PtrWishFlags* begin, size_t count ) {
			//assert( firstFree == nullptr );
			firstFree = begin;
			for ( size_t i=0; i<count-1; ++i ) {
				begin[i].set(begin + i + 1);
				begin[i].set2ndBlock();
			}
			begin[count-1].set(nullptr);
			begin[count-1].set2ndBlock();
			//dbgCheckFreeList();
		}
		size_t insert( void* ptr ) {
			assert( firstFree != nullptr );
			Ptr2PtrWishFlags* tmp = ((Ptr2PtrWishFlags*)(firstFree->getPtr()));
			assert( !firstFree->isUsed() );
			size_t idx;
			assert( !firstFree->is1stBlock() );
			idx = firstFree - slots;
			firstFree->set(ptr);
			firstFree->setUsed();
			firstFree = tmp;
			assert( firstFree == nullptr || !firstFree->isUsed() );
			//dbgCheckFreeList();
			assert( idx < (1<<19) ); // TODO
			return idx;
		}
		void resetPtr( size_t idx, void* newPtr ) {
			assert( idx < otherAllockedCnt );
			slots[idx].set( newPtr );
			slots[idx].setUsed();
			slots[idx].set2ndBlock();
			//dbgCheckFreeList();
			//assert( firstFree == nullptr || !firstFree->isUsed() );
		}
		void remove( size_t idx ) {
			assert( firstFree == nullptr || !firstFree->isUsed() );
			assert( idx < otherAllockedCnt );
			slots[idx].set( firstFree );
			firstFree = slots + idx;
			firstFree->setUnused();
			firstFree->set2ndBlock();
			assert( firstFree == nullptr || !firstFree->isUsed() );
			//dbgCheckFreeList();
		}
		static SecondCBHeader* reallocate(SecondCBHeader* present )
		{
			if ( present != nullptr ) {
				assert( present->otherAllockedCnt != 0 );
				size_t newSize = (present->otherAllockedCnt << 1) + 2;
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (newSize + 2) * sizeof(Ptr2PtrWishFlags) ) );
				//Ptr2PtrWishFlags* newOtherAllockedSlots = 
				memcpy( ret->slots, present->slots, sizeof(Ptr2PtrWishFlags) * present->otherAllockedCnt );
				deallocate( present );
				//otherAllockedSlots.setPtr( newOtherAllockedSlots );
				ret->addToFreeList( ret->slots + present->otherAllockedCnt, newSize - present->otherAllockedCnt );
				present->otherAllockedCnt = newSize;
				return ret;
			}
			else {
				//assert( otherAllockedCnt == 0 );
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (secondBlockStartSize + 2) * sizeof(Ptr2PtrWishFlags) ) );
				ret->otherAllockedCnt = secondBlockStartSize;
				//present->firstFree->set(nullptr);
				//otherAllockedSlots.setPtr( reinterpret_cast<Ptr2PtrWishFlags*>( allocate( otherAllockedCnt * sizeof(Ptr2PtrWishFlags) ) ) );
				ret->addToFreeList( ret->slots, secondBlockStartSize );
				return ret;
			}
		}
		void dealloc()
		{
			deallocate( this );
		}
	};

	struct PtrWithMaskAndFlag : protected Ptr2PtrWishDataBase
	{
		void init() { Ptr2PtrWishDataBase::init( nullptr, 0 ); }
		void setPtr(SecondCBHeader* ptr) { Ptr2PtrWishDataBase::updatePtr( ptr ); }
		SecondCBHeader* getPtr() { return reinterpret_cast<SecondCBHeader*>( Ptr2PtrWishDataBase::getPtr() ); }
		size_t getMask() { return Ptr2PtrWishDataBase::get3bitBlockData(); }
		void setMask(size_t mask) { return Ptr2PtrWishDataBase::update3bitBlockData(mask); }
		void setZombie() { Ptr2PtrWishDataBase::init( nullptr, Ptr2PtrWishDataBase::invalidData ); }
		bool isZombie() { return Ptr2PtrWishDataBase::getLargerBlockData() != 0; }
	};

	static constexpr size_t maxSlots = 3;
	Ptr2PtrWishFlags slots[maxSlots];
	static constexpr size_t secondBlockStartSize = 8;	
	//Ptr2PtrWishFlags* firstFree;
	//size_t otherAllockedCnt = 0; // TODO: try to rely on our allocator on deriving this value
	PtrWithMaskAndFlag otherAllockedSlots;

	void dbgCheckFreeList() {
		/*Ptr2PtrWishFlags* start = firstFree;
		while( start ) {
			assert( !start->isUsed() );
			assert( ( start->getPtr() == 0 || start->is1stBlock() && (size_t)(start - slots) < maxSlots ) || ( (!start->is1stBlock()) && (size_t)(start - otherAllockedSlots.getPtr()) < otherAllockedCnt ) );
			start = ((Ptr2PtrWishFlags*)(start->getPtr()));
		}*/
	}

	void init() {
		//firstFree = slots;
		for ( size_t i=0; i<maxSlots-1; ++i ) {
			slots[i].set(slots + i + 1);
			slots[i].set1stBlock();
		}
		slots[maxSlots-1].set(nullptr);
		slots[maxSlots-1].set1stBlock();

		//otherAllockedCnt = 0;
		otherAllockedSlots.init();
		//assert( !firstFree->isUsed() );
		dbgCheckFreeList();
	}
	/*void deinit() {
		if ( otherAllockedSlots.getPtr() != nullptr ) {
			assert( otherAllockedCnt != 0 );
			//delete [] otherAllockedSlots;
			deallocate( otherAllockedSlots.getPtr() );
			otherAllockedCnt = 0;
		}
		else {
			assert( otherAllockedCnt == 0 );
		}
	}*/
	void addToFreeList( Ptr2PtrWishFlags* begin, size_t count ) {
		//assert( firstFree == nullptr );
		//firstFree = begin;
		for ( size_t i=0; i<count-1; ++i ) {
			begin[i].set(begin + i + 1);
			begin[i].set2ndBlock();
		}
		begin[count-1].set(nullptr);
		begin[count-1].set2ndBlock();
		dbgCheckFreeList();
	}
	void enlargeSecondBlock() {
		otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
	}
	size_t insert( void* ptr ) {
		size_t mask = otherAllockedSlots.getMask();
		//if ( mask != 0x7 )
		{
			// TODO: optimize!
			for ( size_t i=0; i<3; ++i )
				if ( ( mask & (((size_t)1)<<i)) == 0 )
				{
					mask |= (((size_t)1)<<i);
					slots[i].set(ptr);
					slots[i].setUsed();
					otherAllockedSlots.setMask( mask );
					return i;
				}
		}
		//else
		{
			if ( otherAllockedSlots.getPtr() == nullptr || otherAllockedSlots.getPtr()->firstFree == nullptr )
				otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
			assert ( otherAllockedSlots.getPtr() && otherAllockedSlots.getPtr()->firstFree );
			return maxSlots + otherAllockedSlots.getPtr()->insert( ptr );
		}
	}
	void resetPtr( size_t idx, void* newPtr ) {
		if ( idx < maxSlots ) {
			slots[idx].set( newPtr );
			slots[idx].setUsed();
			slots[idx].set1stBlock();
		}
		else {
			//assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots.getPtr()->resetPtr( idx, newPtr );
		}
		//dbgCheckFreeList();
	}
	void remove( size_t idx ) {
		if ( idx < maxSlots ) {
			assert( slots[idx].isUsed() );
			slots[idx].setUnused();
			slots[idx].set1stBlock();
			otherAllockedSlots.setMask( otherAllockedSlots.getMask() & ~(1<<idx) );
		}
		else {
			//assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			assert( otherAllockedSlots.getPtr() != nullptr );
			otherAllockedSlots.getPtr()->remove( idx );
		}
		//assert( firstFree == nullptr || !firstFree->isUsed() );
		//dbgCheckFreeList();
	}
	void clear() {
		if ( otherAllockedSlots.getPtr() != nullptr )
			otherAllockedSlots.getPtr()->dealloc();
		otherAllockedSlots.setZombie();
	}
};
static_assert( sizeof(FirstControlBlock) == 32 );



inline
FirstControlBlock* getControlBlock_(void* t) { return reinterpret_cast<FirstControlBlock*>(t) - 1; }
inline
uint8_t* getAllocatedBlock_(void* t) { return reinterpret_cast<uint8_t*>(getControlBlock_(t)) + getPrefixByteCount(); }


template<class T, bool isSafe> class soft_ptr; // forward declaration
template<class T, bool isSafe> class naked_ptr; // forward declaration

template<bool _Test,
	class _Ty = void>
	using enable_if_t = typename std::enable_if<_Test, _Ty>::type;

template<class _Ty>
	_INLINE_VAR constexpr bool is_array_v = std::is_array<_Ty>::value;

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class owning_ptr
{
template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int> = 0>
	friend owning_ptr<_Ty> make_owning(_Types&&... _Args);
	template<class TT, bool isSafe1>
	friend class soft_ptr;

	T* t;
	FirstControlBlock* getControlBlock() { return getControlBlock_(t); }
	uint8_t* getAllocatedBlock() {return getAllocatedBlock_(t); }

	void updatePtrForListItemsWithInvalidPtr()
	{
		FirstControlBlock* cb = getControlBlock();
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
				reinterpret_cast<soft_ptr<T, isSafe>*>(cb->slots[i].getPtr())->invalidatePtr();
		if ( cb->otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
					reinterpret_cast<soft_ptr<T, isSafe>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->invalidatePtr();
	}

	owning_ptr( T* t_ )
	{
		t = t_;
		getControlBlock()->init();
	}

public:
	owning_ptr()
	{
		t = nullptr;
	}
	owning_ptr( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr( owning_ptr<T, isSafe>&& other )
	{
		t = other.t;
		other.t = nullptr;
	}
	owning_ptr& operator = ( owning_ptr<T, isSafe>&& other )
	{
		t = other.t;
		other.t = nullptr;
		return *this;
	}
	~owning_ptr()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t) )
		{
			updatePtrForListItemsWithInvalidPtr();
			getControlBlock()->clear();
			t->~T();
			//delete [] getAllocatedBlock();
			//zombieDeallocate( reinterpret_cast<uint8_t*>(getAllocatedBlock()) + getPrefixByteCount() );
			zombieDeallocate( getAllocatedBlock_(t) );
			t = nullptr;
		}
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t) )
		{
			updatePtrForListItemsWithInvalidPtr();
			t->~T();
			//delete [] getAllocatedBlock();
//			zombieDeallocate( reinterpret_cast<uint8_t*>(getAllocatedBlock()) + getPrefixByteCount() );
			zombieDeallocate( getAllocatedBlock_(t) );
			t = nullptr;
		}
	}

	void reset( T* t_ ) // Q: what happens to safe ptrs?
	{
		assert( t == nullptr );
		reset();
	}

	void swap( owning_ptr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	naked_ptr<T, isSafe> get() const
	{
		naked_ptr<T, isSafe> ret;
		ret.t = t;
		return ret;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( t );
		return *t;
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( t );
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
class owning_ptr<T>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class soft_ptr<T>;
	T* t;

public:
	owning_ptr()
	{
		t = nullptr;
	}
	owning_ptr( T* t_ )
	{
		t = t_;
	}
	owning_ptr( owning_ptr<T, false>& other ) = delete;
	owning_ptr( owning_ptr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
	}
	~owning_ptr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: being placed at the level of class definition, the codition may be checked whether or not this specialization is instantiated (see, for instance, https://stackoverflow.com/questions/5246049/c11-static-assert-and-template-instantiation)
		if ( NODECPP_LIKELY(t) )
		{
			delete t;
		}
	}

	owning_ptr& operator = ( owning_ptr<T, false>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, false>&& other )
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

	void swap( owning_ptr<T, false>& other )
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

template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int> = 0>
	_NODISCARD inline owning_ptr<_Ty> make_owning(_Types&&... _Args)
	{	// make a unique_ptr
	uint8_t* data = reinterpret_cast<uint8_t*>( zombieAllocate( sizeof(FirstControlBlock) - getPrefixByteCount() + sizeof(_Ty) ) );
	_Ty* objPtr = new ( data + sizeof(FirstControlBlock) - getPrefixByteCount() ) _Ty(_STD forward<_Types>(_Args)...);
	return owning_ptr<_Ty>(objPtr);
	}


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr;
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_static_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );

	Ptr2PtrWishData td;
	T* t;

	void invalidatePtr() { td.init( nullptr, Ptr2PtrWishData::invalidData ); t = nullptr; }
	T* getPtr_() const { return t; }
	size_t getIdx_() const { return td.getData(); }
	FirstControlBlock* getControlBlock() const { return getControlBlock_(td.getPtr()); }
	static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

public:
	soft_ptr()
	{
		td.init(nullptr, Ptr2PtrWishData::invalidData);
		t = nullptr;
	}


	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner )
	{
		t = owner.t; // automatic type conversion (if at all possible)
		td.init( owner.t, getControlBlock(owner.t)->insert(this) );
	}
	//template<>
	soft_ptr( const owning_ptr<T, isSafe>& owner )
	{
		t = owner.t;
		td.init( owner.t, getControlBlock(owner.t)->insert(this) );
	}
	template<class T1>
	soft_ptr<T>& operator = ( const owning_ptr<T1, isSafe>& owner )
	{
		t = owner.t; // automatic type conversion (if at all possible)
		td.init( owner.t, getControlBlock(owner.t)->insert(this) );
		return *this;
	}
	soft_ptr<T>& operator = ( const owning_ptr<T, isSafe>& owner )
	{
		t = owner.t;
		td.init( owner.t, getControlBlock(owner.t)->insert(this) );
		return *this;
	}


	template<class T1>
	soft_ptr( const soft_ptr<T1, isSafe>& other )
	{
		t = other.t; // automatic type conversion (if at all possible)
		td.init( other.getPtr_(), getControlBlock(other.getPtr_())->insert(this) );
	}
	template<class T1>
	soft_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other )
	{
		t = other.t; // automatic type conversion (if at all possible)
		td.init( other.getPtr_(), getControlBlock(other.getPtr_())->insert(this) );
		return *this;
	}
	soft_ptr( const soft_ptr<T, isSafe>& other )
	{
		t = other.t;
		td.init( other.getPtr_(), getControlBlock(other.getPtr_())->insert(this) );
	}
	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>& other )
	{
		t = other.t;
		td.init( other.getPtr_(), getControlBlock(other.getPtr_())->insert(this) );
		return *this;
	}


	soft_ptr( soft_ptr<T, isSafe>&& other )
	{
		t = other.t;
		other.t = nullptr;
		td = other.td;
		getControlBlock()->resetPtr(getIdx_(), this);
		other.td.init( nullptr, Ptr2PtrWishData::invalidData );
		// TODO: think about pointer-like move semantic: td.init( other.getPtr_(), getControlBlock()->insert(this) );
	}

	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>&& other )
	{
		td = other.td;
		other.t = nullptr;
		td.init( other.getPtr_(), getControlBlock()->resetPtr(getIdx_(), this) );
		other.td.init(nullptr,Ptr2PtrWishData::invalidData);
		// TODO: think about pointer-like move semantic: td.init( other.getPtr_(), getControlBlock()->insert(this) );
		return *this;
	}

	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner, T* t_ )
	{
		//if ( reinterpret_cast<uint8_t*>(t_) < reinterpret_cast<uint8_t*>(owner.t) || reinterpret_cast<uint8_t*>(owner.t) + getAllocSize(owner.t) < reinterpret_cast<uint8_t*>(t_) + sizeof(T) )
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t), t_ ) )
			throwPointerOutOfRange();
		t = t_;
		td.init( owner.t, getControlBlock(owner.t)->insert(this) );
	}
	soft_ptr( const owning_ptr<T, isSafe>& owner, T* t_ )
	{
		//if ( reinterpret_cast<uint8_t*>(t_) < reinterpret_cast<uint8_t*>(owner.t) || reinterpret_cast<uint8_t*>(owner.t) + getAllocSize(owner.t) < reinterpret_cast<uint8_t*>(t_) + sizeof(T) )
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t), t_ ) )
			throwPointerOutOfRange();
		t = t_;
		td.init( owner.t, getControlBlock(owner.t)->insert(this) );
	}

	template<class T1>
	soft_ptr( const soft_ptr<T1, isSafe>& other, T* t_ )
	{
		//if ( reinterpret_cast<uint8_t*>(t_) < reinterpret_cast<uint8_t*>(other.t) || reinterpret_cast<uint8_t*>(other.t) + getAllocSize(other.t) < reinterpret_cast<uint8_t*>(t_) + sizeof(T) )
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.td.getPtr()), t_ ) )
			throwPointerOutOfRange();
		t = t_;
		td.init( other.t, getControlBlock(other.t)->insert(this) );
	}
	soft_ptr( const soft_ptr<T, isSafe>& other, T* t_ )
	{
		//if ( reinterpret_cast<uint8_t*>(t_) < reinterpret_cast<uint8_t*>(other.t) || reinterpret_cast<uint8_t*>(other.t) + getAllocSize(other.t) < reinterpret_cast<uint8_t*>(t_) + sizeof(T) )
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.td.getPtr()), t_ ) )
			throwPointerOutOfRange();
		t = t_;
		td.init( other.t, getControlBlock(other.t)->insert(this) );
	}

	void swap( soft_ptr<T, isSafe>& other )
	{
		auto tmp = t;
		t = other.t;
		other.t = tmp;
		td.swap( other.td );
		if ( getPtr_() )
			getControlBlock()->resetPtr(getIdx_(), this);
		if ( other.getPtr_() )
			other.getControlBlock()->resetPtr(other.getIdx_(), &other);
	}

	naked_ptr<T, isSafe> get() const
	{
		assert( getPtr_() != nullptr );
		naked_ptr<T, isSafe> ret;
		ret.t = getPtr_();
		return ret;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( getPtr_() );
		return *getPtr_();
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( getPtr_() );
		return getPtr_();
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return getPtr_() != nullptr;
	}

	~soft_ptr()
	{
		if( getPtr_() != nullptr ) {
			assert( getIdx_() != Ptr2PtrWishData::invalidData );
			getControlBlock()->remove(getIdx_());
			invalidatePtr();
		}
	}
};

#if 0
template<class T>
class soft_ptr<T,false>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class owning_ptr<T,false>;
	T* t;

public:
	soft_ptr()
	{
		this->t = nullptr;
	}
	soft_ptr( owning_ptr<T,false>& owner )
	{
		this->t = owner.t;
	}
	soft_ptr( soft_ptr<T,false>& other )
	{
		this->t = other.t;
	}
	soft_ptr( soft_ptr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
	}

	soft_ptr& operator = ( soft_ptr<T,false>& other )
	{
		this->t = other.t;
		return *this;
	}
	soft_ptr& operator = ( soft_ptr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		return *this;
	}

	void swap( soft_ptr<T, false>& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
	}


	T& operator * () const
	{
		return *t;
	}

	T* operator -> () const 
	{
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	~soft_ptr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial );
	}
};
#endif // 0

template<class T, class T1, bool isSafe = NODECPP_ISSAFE_DEFAULT>
soft_ptr<T, isSafe> soft_ptr_static_cast( soft_ptr<T1, isSafe> p ) {
/*	soft_ptr<T, isSafe> ret;
	ret.t = static_cast<T*>(p.getPtr_());
	ret.td.init( p.getPtr_(), p.getControlBlock()->insert(&ret) );*/
	soft_ptr<T, isSafe> ret(p,static_cast<T*>(p.getPtr_()));
	return ret;
}

template<class T, class T1, bool isSafe = NODECPP_ISSAFE_DEFAULT>
soft_ptr<T, isSafe> soft_ptr_reinterpret_cast( soft_ptr<T1, isSafe> p ) {
//	soft_ptr<T, isSafe> ret;
//	ret.t = reinterpret_cast<T*>(p.getPtr_());
//	ret.td.init( p.getPtr_(), p.getControlBlock()->insert(&ret) );
	soft_ptr<T, isSafe> ret(p,reinterpret_cast<T*>(p.getPtr_()));
	return ret;
}


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class naked_ptr
{
	friend class owning_ptr<T, isSafe>;
	friend class soft_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	template<class TT, bool isSafe1>
	friend class soft_ptr;

	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above

	T* t;

public:
	naked_ptr() { t = nullptr; }

	naked_ptr(T& t_) { t = &t_; }

	template<class T1>
	naked_ptr( const owning_ptr<T1, isSafe>& owner ) { t = owner.get(); }
	naked_ptr( const owning_ptr<T, isSafe>& owner ) { t = owner.get(); }
	template<class T1>
	naked_ptr<T>& operator = ( const owning_ptr<T1, isSafe>& owner ) { t = owner.get(); return *this; }
	naked_ptr<T>& operator = ( const owning_ptr<T, isSafe>& owner ) { t = owner.get(); return *this; }

	template<class T1>
	naked_ptr( const soft_ptr<T1, isSafe>& other ) { t = other.get(); }
	naked_ptr( const soft_ptr<T, isSafe>& other ) { t = other.get(); }
	template<class T1>
	naked_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other ) { t = other.get(); return *this; }
	naked_ptr<T>& operator = ( const soft_ptr<T, isSafe>& other ) { t = other.get(); return *this; }

	template<class T1>
	naked_ptr( const naked_ptr<T1, isSafe>& other ) { t = other.t; }
	template<class T1>
	naked_ptr<T>& operator = ( const naked_ptr<T1, isSafe>& other ) { t = other.t; return *this; }
	naked_ptr( const naked_ptr<T, isSafe>& other ) = default;
	naked_ptr<T, isSafe>& operator = ( naked_ptr<T, isSafe>& other ) = default;

	naked_ptr( naked_ptr<T, isSafe>&& other ) = default;
	naked_ptr<T, isSafe>& operator = ( naked_ptr<T, isSafe>&& other ) = default;

	void swap( naked_ptr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( t );
		return *t;
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( t );
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}

	bool operator == ( const naked_ptr<T, isSafe>& other ) const { return t == other.t; } // TODO: revise necessity

	~naked_ptr()
	{
		t = nullptr;
	}
};

#endif
