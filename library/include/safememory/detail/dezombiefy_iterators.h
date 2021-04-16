/* -------------------------------------------------------------------------------
* Copyright (c) 2021, OLogN Technologies AG
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

#ifndef SAFE_MEMORY_DETAIL_DEZOMBIEFY_ITERATORS_H
#define SAFE_MEMORY_DETAIL_DEZOMBIEFY_ITERATORS_H

#include <EASTL/unordered_set.h>
#include <safememory/detail/allocator_to_eastl.h>

namespace safememory::detail {

#ifdef SAFEMEMORY_DEZOMBIEFY_ITERATORS
#define SAFEMEMORY_DEZOMBIEFY_ITERATORS_REGISTRY , public detail::iterator_registry
#else
#define SAFEMEMORY_DEZOMBIEFY_ITERATORS_REGISTRY
#endif

class iterator_dezombiefier; //fwd


/**
 * \brief A registry to keeps track of iteratos.
 * 
 * When we try to dezombiefy iterators, \c vector and \c string need to know all still existing
 * iterators they have.
 * This is because dezombiefy iterators need to call \c size() method on the container before
 * each access to validate the item trying to dereference is still valid inside the container.
 * 
 * The problem shows when we \c move the container, as the heap array remains valid, but we can't
 * no longer call \c size() on the previous instance, as it may have been deallocated, or
 * stack space reused.
 * 
 */

class iterator_registry {

	//mb: we use unordered_set because we have already customized to our allocators
	template<class T>
	using set_type = eastl::unordered_set<T, eastl::hash<T>, eastl::equal_to<T>, allocator_to_eastl_hashtable<memory_safety::none>>;

	set_type<iterator_dezombiefier*> itRegistry{allocator_to_eastl_hashtable<memory_safety::none>()};

public:
	iterator_registry() {}

	iterator_registry(const iterator_registry&) {} //don't copy registry
	
	iterator_registry& operator=(const iterator_registry& other) {
		if(this == std::addressof(other))
			return *this;

		// our iterators are now invalid
		invalidateAllIterators();
		return *this;
	}

	iterator_registry(iterator_registry&& other) {
		// don't copy registry
		other.invalidateAllIterators();
	} 

	iterator_registry& operator=(iterator_registry&& other) {
		if(this == std::addressof(other))
			return *this;

		// our iterators are now invalid
		invalidateAllIterators();
		// other iterators are also invalid, because her array is now ours
		other.invalidateAllIterators();

		return *this;
	}

	~iterator_registry() {
		invalidateAllIterators();
	}

	void invalidateAllIterators() noexcept;

	void registerIterator(iterator_dezombiefier* it) noexcept { itRegistry.insert(it); }
	void unregisterIterator(iterator_dezombiefier* it) noexcept { itRegistry.erase(it); }
};





class iterator_dezombiefier {
	std::function<std::size_t()> sz;
	iterator_registry* registry = nullptr;
public:
	iterator_dezombiefier(int) {}

	template<class Cont>
	iterator_dezombiefier(Cont* container) :sz(std::bind(&Cont::size, container)), registry(container) {

		registry->registerIterator(this);
	}

	iterator_dezombiefier(const iterator_dezombiefier& other) : sz(other.sz), registry(other.registry) {

		if(registry)
			registry->registerIterator(this);
	}

	iterator_dezombiefier& operator=(const iterator_dezombiefier& other) {
		if(this == std::addressof(other))
			return *this;

		if(registry)
			registry->unregisterIterator(this);

		sz = other.sz;
		registry = other.registry;

		if(registry)
			registry->registerIterator(this);

		return *this;
	}

	void invalidate() {
		if(registry) {
			sz = nullptr;
			registry = nullptr;
		}
	}

	~iterator_dezombiefier() {
		if(registry) {
			registry->unregisterIterator(this);
			sz = nullptr;
			registry = nullptr;
			forcePreviousChangesToThisInDtor(this);
		}
	}

	operator bool() const noexcept { return static_cast<bool>(registry); }
	std::size_t size() const noexcept { return sz(); }
};


inline
void iterator_registry::invalidateAllIterators() noexcept {
	for(auto each : itRegistry)
		each->invalidate();
	
	itRegistry.clear();
}


} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_DEZOMBIEFY_ITERATORS_H
