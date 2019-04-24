
#include <stdio.h>
#include <safe_ptr.h>

using namespace nodecpp::safememory;

void sampleTestCall()
{
	struct Any { soft_ptr<int> softN; };
	owning_ptr<Any> any = make_owning<Any>();
	{
		owning_ptr<int> owningN = make_owning<int>( 5 );
		any->softN = owningN;
		printf( "so far, so good (we\'ve assigned a value of owning_ptr to soft_ptr)\n" );
		printf( "end of scope of owning_ptr...\n" );
		// what's pointed by owningN is deleted here
	}
	printf( "trying to access a deleted object...\n" );
	printf( "throwing call (control value: %d)\n", *(any->softN) );

	soft_ptr<int> softN;
	naked_ptr<int> x = nullptr;
	printf( "throwing call (control value: %d)\n", *x );

}

int main()
{
    printf( "Hello Memory Safe Cpp!\n" ); 
	try {
		sampleTestCall();
		printf( "Passed successfully!\n" );
	}
	catch (nodecpp::error::memory_error e)
	{
		printf( "Exception caught: \"%s\"\n", e.description().c_str() );
	}
}
