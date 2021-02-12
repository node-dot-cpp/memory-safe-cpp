#ifndef DUMMY_TEST_OBJECTS_H
#define DUMMY_TEST_OBJECTS_H

// For various testing purposes we will need misc objects (classes, ...) doing nothing but representing certain constructions to be tested
// Such objects are gathered in the namespace safememory::testing::dummy_objects
namespace safememory::testing::dummy_objects {

// prereqs for testing soft_this_ptr
class SomethingLarger; //forward declaration
class Something
{
public:
	soft_this_ptr<Something> myThis;
	owning_ptr<int> m;
	soft_ptr<SomethingLarger> prtToOwner;
	Something( int k) { m = make_owning<int>(); *m = k; }
	Something(soft_ptr<SomethingLarger> prtToOwner_, int k);
	Something(bool, soft_ptr<SomethingLarger> prtToOwner_, int k);
	void setOwner(soft_ptr<SomethingLarger> prtToOwner_) { prtToOwner = prtToOwner_ ; }
};
class SomethingLarger
{
public:
	soft_this_ptr<SomethingLarger> myThis;
	soft_ptr<Something> softpS;
	owning_ptr<Something> opS;
	SomethingLarger(int k) : opS( make_owning<Something>( k ) ) {
		soft_ptr<SomethingLarger> sp = myThis.getSoftPtr( this );		
		opS->setOwner( sp );
		softpS = opS;
	}
	SomethingLarger(int k, bool) {
		soft_ptr<SomethingLarger> sp = myThis.getSoftPtr( this );		
		opS = make_owning<Something>( sp, k );
	}
	SomethingLarger(int k, bool, bool) {
		soft_ptr<SomethingLarger> sp = myThis.getSoftPtr( this );		
		opS = make_owning<Something>( false, sp, k );
	}
	void doBackRegistration( soft_ptr<Something> s ) { softpS = s; }
};
Something::Something(soft_ptr<SomethingLarger> prtToOwner_, int k) {
	prtToOwner = prtToOwner_;
	soft_ptr<Something> sp = myThis.getSoftPtr( this );
	m = make_owning<int>(); 
	*m = k; 
	prtToOwner->doBackRegistration( sp );
}
Something::Something(bool, soft_ptr<SomethingLarger> prtToOwner_, int k) {
	prtToOwner = prtToOwner_;
	soft_ptr<Something> sp = soft_ptr_in_constructor( this );
	m = make_owning<int>(); 
	*m = k; 
	prtToOwner->doBackRegistration( sp );
}

struct StructureWithSoftIntPtr { soft_ptr<int> sp; int n; };
struct StructureWithSoftDoublePtr { soft_ptr<double> sp; double d;};
struct StructureWithSoftPtrDeclaredUnsafe { soft_ptr<int> sp; double d;};

struct StructWithDtorRequiringValidSoftPtrsToItself; // forward declaration
struct StructWithSoftPtr
{
	int dummy;
	soft_ptr<StructWithDtorRequiringValidSoftPtrsToItself> sp;
	StructWithSoftPtr( int val ) { dummy = val; }
};
struct StructWithDtorRequiringValidSoftPtrsToItself
{
	safememory::soft_this_ptr<StructWithDtorRequiringValidSoftPtrsToItself> myThis;
	int dummy = 0;
	soft_ptr<StructWithSoftPtr> sp;
	StructWithDtorRequiringValidSoftPtrsToItself( int val, soft_ptr<StructWithSoftPtr> sp_ ) : sp( sp_ ) { 
		dummy = val;
		sp->sp = myThis.getSoftPtr<StructWithDtorRequiringValidSoftPtrsToItself>(this); 
	}
	~StructWithDtorRequiringValidSoftPtrsToItself() {
		if ( sp != nullptr && sp->sp != nullptr )
			sp->dummy = sp->sp->dummy;
	}
};

template<size_t minSz, size_t alignExp=0>
struct LargeObjectWithControllableAlignment
{
	alignas(1<<alignExp) int i;
	uint8_t dummyBytes[minSz];
};

} // namespace safememory::testing::dummy_objects


#endif // DUMMY_TEST_OBJECTS_H