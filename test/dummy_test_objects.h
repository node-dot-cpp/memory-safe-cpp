#ifndef DUMMY_TEST_OBJECTS_H
#define DUMMY_TEST_OBJECTS_H

// For various testing purposes we will need misc objects (classes, ...) doing nothing but representing certain constructions to be tested
// Such objects are gathered in the namespace nodecpp::safememory::testing::dummy_objects
namespace nodecpp::safememory::testing::dummy_objects {

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
	SomethingLarger(int k) : opS( std::move( make_owning<Something>( k ) ) ) {
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


} // namespace nodecpp::safememory::testing::dummy_objects


#endif // DUMMY_TEST_OBJECTS_H