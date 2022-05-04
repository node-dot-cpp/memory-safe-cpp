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

#include <platform_base.h>
#include <safe_memory_error.h>
#include <safememory/detail/safe_ptr_common.h>
#include <EASTL/internal/config.h> // for eastl_size_t


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
 * The problem shows when we \c move the container, as the heap array remains valid, but we
 * no longer can call \c size() on the previous instance, as it may have been deallocated, or
 * stack space reused.
 * 
 */



class iterator_registry {

	// mb: we don't use a normal container here, because we don't want dezombifing to make
	// allocations that didn't exist on the non-dezombifing
	// Because of that we use an intrusive list, and since iterators are expected to be
	// only a few we use a single linked list.

	iterator_dezombiefier* head = nullptr;

public:
	iterator_registry() {}

	iterator_registry(const iterator_registry&) {} //don't copy registry
	
	iterator_registry(iterator_registry&& other) noexcept {
		// don't copy registry
		other.invalidateAllIterators();
	} 

	iterator_registry& operator=(const iterator_registry& other) {
		
		if(this == std::addressof(other))
			return *this;

		// our iterators are now invalid
		invalidateAllIterators();

		return *this;
	}

	iterator_registry& operator=(iterator_registry&& other) noexcept {

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
		forcePreviousChangesToThisInDtor(this);
	}

	void addIterator(iterator_dezombiefier* it) noexcept;
	void removeIterator(iterator_dezombiefier* it) noexcept;
	void invalidateAllIterators() noexcept;
};



class iterator_dezombiefier {
public:
	iterator_dezombiefier* next = nullptr;
	iterator_registry* registry = nullptr;
	std::function<eastl_size_t()> sz;

	[[noreturn]] static void ThrowZombieException() { throw nodecpp::error::early_detected_zombie_pointer_access; }

	iterator_dezombiefier(int) {}

	template<class Cont>
	iterator_dezombiefier(Cont* container) :registry(container), sz(std::bind(&Cont::size, container)) {

		if(registry)
			registry->addIterator(this);
	}

	iterator_dezombiefier(const iterator_dezombiefier& other) : registry(other.registry), sz(other.sz) {

		if(registry)
			registry->addIterator(this);
	}

	iterator_dezombiefier& operator=(const iterator_dezombiefier& other) {

		if(this == std::addressof(other))
			return *this;

		if(registry)
			registry->removeIterator(this);

		this->registry = other.registry;
		this->sz = other.sz;

		if(registry)
			registry->addIterator(this);

		return *this;
	}

	void invalidate() noexcept {

		registry = nullptr;
		next = nullptr;
		sz = nullptr;
	}

	~iterator_dezombiefier() {

		if(registry)
			registry->removeIterator(this);

		invalidate();
		forcePreviousChangesToThisInDtor(this);
	}

	eastl_size_t size() const {

		if ( NODECPP_UNLIKELY( !registry ) )
			ThrowZombieException();

		return sz();
	}
};


inline
void iterator_registry::addIterator(iterator_dezombiefier* it) noexcept {

	// assert(it != nullptr);
	// assert(it->next == nullptr);
	it->next = head;
	head = it;
}

inline
void iterator_registry::removeIterator(iterator_dezombiefier* it) noexcept {

	// assert(it != nullptr);
	if(it == head) {
		head = it->next;
		it->next = nullptr;
		return;
	}
	else {
		iterator_dezombiefier* current = head;
		while(current != nullptr && current->next != it)
			current = current->next;

		if(current == nullptr)
			return; // not found, error?

		// assert(current->next == it);
		current->next = it->next;
		it->next = nullptr;
		return;
	}
}

inline
void iterator_registry::invalidateAllIterators() noexcept {

	while(head != nullptr) {
		iterator_dezombiefier* current = head;
		head = current->next;
		current->invalidate();
	}
}

} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_DEZOMBIEFY_ITERATORS_H
