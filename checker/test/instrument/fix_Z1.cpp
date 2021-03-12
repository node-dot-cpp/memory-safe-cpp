// RUN: %check_safememory_instrument --fix-only %s %t %p

#include <safememory/safe_ptr.h>

#include <safe_types.h>

using namespace safememory;

int release() {	return 0; }


struct Bad {

	owning_ptr<SafeType> StPtr;



	Bad() {
		StPtr = make_owning<SafeType>(); 
	}

	// int release() {
	// 	StPtr.reset();
	// 	return 0;
	// }


	void verifyZombie(SafeType& StRef) {

		//safeFunction may be eating a zombie
		while(safeFunction(*StPtr) + release() != 0);
// CHECK-FIXES: while(safememory::detail::dz_ne(safememory::detail::dz_add(safeFunction(*StPtr) , release()) , 0));
	}
};
