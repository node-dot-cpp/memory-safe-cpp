/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
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

// Initial vesion from:
// https://github.com/electronicarts/EASTL/blob/3.15.00/source/red_black_tree.cpp

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// The tree insert and erase functions below are based on the original 
// HP STL tree functions. Use of these functions was been approved by
// EA legal on November 4, 2005 and the approval documentation is available
// from the EASTL maintainer or from the EA legal deparatment on request.
// 
// Copyright (c) 1994
// Hewlett-Packard Company
// 
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation. Hewlett-Packard Company makes no
// representations about the suitability of this software for any
// purpose. It is provided "as is" without express or implied warranty.
///////////////////////////////////////////////////////////////////////////////



#include <EABase/config/eacompilertraits.h>
#include <EASTL/internal/config.h>
#include <EASTL/internal/red_black_tree.h>
#include <stddef.h>



namespace nodecpp
{
	// Forward declarations
	void RBTreeRotateLeft(rbtree_soft_ptr pNode);
	void RBTreeRotateRight(rbtree_soft_ptr pNode);


	inline void RBTreeSetLeftChild(rbtree_soft_ptr pNode, rbtree_owning_ptr pNodeChild) {
		EASTL_ASSERT(pNode);
//		EASTL_ASSERT(pNodeChild);
		EASTL_ASSERT(!pNode->mpNodeLeft);

		if(pNodeChild) {
		pNodeChild->mpNodeParent = pNode;
		pNode->mpNodeLeft = std::move(pNodeChild);
	}
	}

	inline void RBTreeSetRightChild(rbtree_soft_ptr pNode, rbtree_owning_ptr pNodeChild) {
		EASTL_ASSERT(pNode);
//		EASTL_ASSERT(pNodeChild);
		EASTL_ASSERT(!pNode->mpNodeRight);

		if(pNodeChild) {
		pNodeChild->mpNodeParent = pNode;
		pNode->mpNodeRight = std::move(pNodeChild);
	}
	}


	/// RBTreeIncrement
	/// Returns the next item in a sorted red-black tree.
	///
	EASTL_API rbtree_soft_ptr RBTreeIncrement(rbtree_soft_ptr pNode)
	{
		// check iterator is alive
        pNode->assert_is_alive();

		if(pNode->mpNodeRight) 
		{
			pNode = pNode->mpNodeRight;

			while(pNode->mpNodeLeft)
				pNode = pNode->mpNodeLeft;
		}
		else 
		{
			rbtree_soft_ptr pNodeTemp = pNode->mpNodeParent;

		    //mb: while pNodeTemp will never be null for any normal
			// iteration, we check it anyway because I am paranoid
			while(pNodeTemp && pNode == pNodeTemp->mpNodeRight) 
			{
				pNode = pNodeTemp;
				pNodeTemp = pNodeTemp->mpNodeParent;
			}

			pNode = pNodeTemp;
		}

		return pNode;
	}



	/// RBTreeIncrement
	/// Returns the previous item in a sorted red-black tree.
	///
	EASTL_API rbtree_soft_ptr RBTreeDecrement(rbtree_soft_ptr pNode)
	{
		// check iterator is alive
        pNode->assert_is_alive();

		// mb: decrement from end() works normally now
		// if((pNode->mpNodeParent->mpNodeParent == pNode) && (pNode->mColor == kRBTreeColorRed))
		// 	return pNode->mpNodeRight; 
		// else
		if(pNode->mpNodeLeft)
		{
			rbtree_soft_ptr pNodeTemp = pNode->mpNodeLeft;

			while(pNodeTemp->mpNodeRight)
				pNodeTemp = pNodeTemp->mpNodeRight;

			return pNodeTemp;
		}

		rbtree_soft_ptr pNodeTemp = pNode->mpNodeParent;

		while(pNodeTemp && pNode == pNodeTemp->mpNodeLeft) 
		{
			pNode     = pNodeTemp;
			pNodeTemp = pNodeTemp->mpNodeParent;
		}

		return pNodeTemp;
	}



	/// RBTreeGetBlackCount
	/// Counts the number of black nodes in an red-black tree, from pNode down to the given bottom node.  
	/// We don't count red nodes because red-black trees don't really care about
	/// red node counts; it is black node counts that are significant in the 
	/// maintenance of a balanced tree.
	///
	EASTL_API size_t RBTreeGetBlackCount(rbtree_soft_ptr pNodeTop, rbtree_soft_ptr pNodeBottom)
	{
		size_t nCount = 0;

		for(; pNodeBottom; pNodeBottom = pNodeBottom->mpNodeParent)
		{
			if(pNodeBottom->mColor == kRBTreeColorBlack) 
				++nCount;

			if(pNodeBottom == pNodeTop) 
				break;
		}

		return nCount;
	}


	/// RBTreeRotateLeft
	/// Does a left rotation about the given node. 
	/// If you want to understand tree rotation, any book on algorithms will
	/// discussion the topic in good detail.
	void RBTreeRotateLeft(rbtree_soft_ptr pNode)
	{
		rbtree_owning_ptr pNodeTemp = pNode->take_node_right();

		if(pNodeTemp->mpNodeLeft)
			RBTreeSetRightChild(pNode, pNodeTemp->take_node_left());
		// pNode->mpNodeRight = std::move(pNodeTemp->mpNodeLeft);

		// if(pNode->mpNodeRight)
		// 	pNode->mpNodeRight->mpNodeParent = pNode;
//		pNodeTemp->mpNodeParent = pNode->mpNodeParent;
		
		// if(pNode == pNodeRoot)
		// 	pNodeRoot = pNodeTemp;
		// else
		rbtree_soft_ptr pNodeParent = pNode->mpNodeParent;

		if(pNode == pNodeParent->mpNodeLeft) {
			rbtree_owning_ptr pNodeOwner = pNodeParent->take_node_left();
			RBTreeSetLeftChild(pNodeParent, std::move(pNodeTemp));
//			pNodeParent->mpNodeLeft = std::move(pNodeTemp);
			RBTreeSetLeftChild(pNodeParent->mpNodeLeft, std::move(pNodeOwner));
			// pNodeParent->mpNodeLeft->mpNodeLeft = ;
			// pNodeParent->mpNodeLeft->mpNodeLeft->mpNodeParent = pNodeParent->mpNodeLeft;
		}
		else {
			rbtree_owning_ptr pNodeOwner = pNodeParent->take_node_right();
			RBTreeSetRightChild(pNodeParent, std::move(pNodeTemp));
			RBTreeSetLeftChild(pNodeParent->mpNodeRight, std::move(pNodeOwner));
			// pNodeParent->mpNodeRight = std::move(pNodeTemp);
			// pNodeParent->mpNodeRight->mpNodeLeft = std::move(pNodeOwner);
			// pNodeParent->mpNodeRight->mpNodeLeft->mpNodeParent = pNodeParent->mpNodeRight;
		}

//		pNodeTemp->mpNodeLeft = pNode;
//		pNode->mpNodeParent = pNodeTemp;

//		return pNodeRoot;
	}



	/// RBTreeRotateRight
	/// Does a right rotation about the given node. 
	/// If you want to understand tree rotation, any book on algorithms will
	/// discussion the topic in good detail.
	void RBTreeRotateRight(rbtree_soft_ptr pNode)
	{
		// rbtree_node_base* const pNodeTemp = pNode->mpNodeLeft;

		// pNode->mpNodeLeft = pNodeTemp->mpNodeRight;

		// if(pNodeTemp->mpNodeRight)
		// 	pNodeTemp->mpNodeRight->mpNodeParent = pNode;
		// pNodeTemp->mpNodeParent = pNode->mpNodeParent;

		// if(pNode == pNodeRoot)
		// 	pNodeRoot = pNodeTemp;
		// else if(pNode == pNode->mpNodeParent->mpNodeRight)
		// 	pNode->mpNodeParent->mpNodeRight = pNodeTemp;
		// else
		// 	pNode->mpNodeParent->mpNodeLeft = pNodeTemp;

		// pNodeTemp->mpNodeRight = pNode;
		// pNode->mpNodeParent = pNodeTemp;

		////////////////
		rbtree_owning_ptr pNodeTemp = pNode->take_node_left();

		if(pNodeTemp->mpNodeRight)
			RBTreeSetLeftChild(pNode, pNodeTemp->take_node_right());

		rbtree_soft_ptr pNodeParent = pNode->mpNodeParent;
		rbtree_owning_ptr pNodeOwner = nullptr; 

		if(pNode == pNodeParent->mpNodeRight) {
			pNodeOwner = pNodeParent->take_node_right();
			RBTreeSetRightChild(pNodeParent, std::move(pNodeTemp));
			RBTreeSetRightChild(pNodeParent->mpNodeRight, std::move(pNodeOwner));
		}
		else {
			pNodeOwner = pNodeParent->take_node_left();
			RBTreeSetLeftChild(pNodeParent, std::move(pNodeTemp));
			RBTreeSetRightChild(pNodeParent->mpNodeLeft, std::move(pNodeOwner));
		}
	}




	/// RBTreeInsert
	/// Insert a node into the tree and rebalance the tree as a result of the 
	/// disturbance the node introduced.
	///
	EASTL_API void RBTreeInsert(rbtree_owning_ptr pNodeOwner,
								rbtree_soft_ptr pNodeParent,
								rbtree_soft_ptr pNodeAnchor,
								rbtree_min_max_nodes* pMinMaxNodes,
								RBTreeSide insertionSide)
	{
		// rbtree_node_base*& pNodeRootRef = pNodeAnchor->mpNodeLeft;
		rbtree_soft_ptr pNode = pNodeOwner;

		// Initialize fields in new node to insert.
		pNode->mpNodeParent = pNodeParent;
		pNode->mpNodeRight  = nullptr;
		pNode->mpNodeLeft   = nullptr;
		pNode->mColor       = kRBTreeColorRed;

		// Insert the node.
		if(insertionSide == kRBTreeSideLeft)
		{
			pNodeParent->mpNodeLeft = std::move(pNodeOwner);

			if(!pNodeParent->mpNodeParent) // first root node
			{
//				pNodeAnchor->mpNode = pNode;
				pMinMaxNodes->mpMaxChild = pNode;
			}
			if(pNodeParent == pMinMaxNodes->mpMinChild)
				pMinMaxNodes->mpMinChild = pNode; // Maintain leftmost pointing to min node
		}
		else
		{
			pNodeParent->mpNodeRight = std::move(pNodeOwner);

			if(pNodeParent == pMinMaxNodes->mpMaxChild)
				pMinMaxNodes->mpMaxChild = pNode; // Maintain rightmost pointing to max node
		}

		// Rebalance the tree.
		while((pNode != pNodeAnchor->mpNodeLeft) && (pNode->mpNodeParent->mColor == kRBTreeColorRed)) 
		{
			EA_ANALYSIS_ASSUME(pNode->mpNodeParent != nullptr);
			rbtree_soft_ptr pNodeParentParent = pNode->mpNodeParent->mpNodeParent;

			if(pNode->mpNodeParent == pNodeParentParent->mpNodeLeft) 
			{
				rbtree_soft_ptr pNodeTemp = pNodeParentParent->mpNodeRight;

				if(pNodeTemp && (pNodeTemp->mColor == kRBTreeColorRed)) 
				{
					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeTemp->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					pNode = pNodeParentParent;
				}
				else 
				{
					if(pNode->mpNodeParent && pNode == pNode->mpNodeParent->mpNodeRight) 
					{
						pNode = pNode->mpNodeParent;
						// pNodeRootRef = RBTreeRotateLeft(pNode, pNodeRootRef);
						RBTreeRotateLeft(pNode);
					}

					EA_ANALYSIS_ASSUME(pNode->mpNodeParent != nullptr);
					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					// pNodeRootRef = RBTreeRotateRight(pNodeParentParent, pNodeRootRef);
					RBTreeRotateRight(pNodeParentParent);
				}
			}
			else 
			{
				rbtree_soft_ptr pNodeTemp = pNodeParentParent->mpNodeLeft;

				if(pNodeTemp && (pNodeTemp->mColor == kRBTreeColorRed)) 
				{
					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeTemp->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					pNode = pNodeParentParent;
				}
				else 
				{
					EA_ANALYSIS_ASSUME(pNode != nullptr && pNode->mpNodeParent != nullptr);

					if(pNode == pNode->mpNodeParent->mpNodeLeft) 
					{
						pNode = pNode->mpNodeParent;
						// pNodeRootRef = RBTreeRotateRight(pNode, pNodeRootRef);
						RBTreeRotateRight(pNode);
					}

					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					// pNodeRootRef = RBTreeRotateLeft(pNodeParentParent, pNodeRootRef);
					RBTreeRotateLeft(pNodeParentParent);
				}
			}
		}

		EA_ANALYSIS_ASSUME(pNodeAnchor->mpNodeLeft != nullptr);
		pNodeAnchor->mpNodeLeft->mColor = kRBTreeColorBlack;

	} // RBTreeInsert




	/// RBTreeErase
	/// Erase a node from the tree.
	///
	EASTL_API rbtree_owning_ptr RBTreeErase(rbtree_soft_ptr pNode,
								rbtree_soft_ptr pNodeAnchor,
								rbtree_min_max_nodes* pMinMaxNodes)

	{
		// rbtree_node_base*& pNodeRootRef      = pNodeAnchor->mpNodeLeft;
		// rbtree_node_base*& pNodeLeftmostRef  = pMinMaxNodes->mpMinChild;
		// rbtree_node_base*& pNodeRightmostRef = pMinMaxNodes->mpMaxChild;
		// rbtree_node_base*  pNodeSuccessor    = pNode;
		rbtree_soft_ptr    pNodeChild        = nullptr;
		rbtree_soft_ptr    pNodeChildParent  = nullptr;
		
		rbtree_owning_ptr  pNodeOwning 		 = nullptr;

		if(pNode == pMinMaxNodes->mpMinChild) // If pNode is the tree begin() node...
		{
			// Because pNode is the tree begin(), pNode->mpNodeLeft must be NULL.
			// Here we assign the new begin() (first node).
			if(pNode->mpNodeRight)
				pMinMaxNodes->mpMinChild = RBTreeGetMinChild(pNode->mpNodeRight); 
			else
				pMinMaxNodes->mpMinChild = pNode->mpNodeParent; // This  makes (pNodeLeftmostRef == end()) if (pNode == root node)
		}

		if(pNode == pMinMaxNodes->mpMaxChild) // If pNode is the tree last (rbegin()) node...
		{
			// Because pNode is the tree rbegin(), pNode->mpNodeRight must be NULL.
			// Here we assign the new rbegin() (last node)
			if(pNode->mpNodeLeft)
				pMinMaxNodes->mpMaxChild = RBTreeGetMaxChild(pNode->mpNodeLeft);
			else
				pMinMaxNodes->mpMaxChild = pNode->mpNodeParent; // makes pNodeRightmostRef == &mAnchor if pNode == pNodeRootRef
		}

		if(!pNode->mpNodeLeft && !pNode->mpNodeRight) {
			if(pNode == pNode->mpNodeParent->mpNodeLeft)
				pNodeOwning = pNode->mpNodeParent->take_node_left();  // Make pNode's replacement node be on the same side.
			else
				pNodeOwning = pNode->mpNodeParent->take_node_right();

			pNodeChildParent = pNode->mpNodeParent;
		}
		else if(!pNode->mpNodeLeft) {
			EASTL_ASSERT(pNode->mpNodeRight);

			pNodeChildParent = pNode->mpNodeParent;
			rbtree_owning_ptr pNodeChildOwn = pNode->take_node_right();
			pNodeChild = pNodeChildOwn;
			if(pNode == pNode->mpNodeParent->mpNodeLeft) {// If pNode is a left node...
				pNodeOwning = pNode->mpNodeParent->take_node_left();
				RBTreeSetLeftChild(pNode->mpNodeParent, std::move(pNodeChildOwn));  // Make pNode's replacement node be on the same side.
			}
			else {
				pNodeOwning = pNode->mpNodeParent->take_node_right();
				RBTreeSetRightChild(pNode->mpNodeParent, std::move(pNodeChildOwn));
			}

		}
		else if(!pNode->mpNodeRight) {
			EASTL_ASSERT(pNode->mpNodeLeft);

			pNodeChildParent = pNode->mpNodeParent;
			rbtree_owning_ptr pNodeChildOwn = pNode->take_node_left(); 
			pNodeChild = pNodeChildOwn;
			if(pNode == pNode->mpNodeParent->mpNodeLeft) {// If pNode is a left node...
				pNodeOwning = pNode->mpNodeParent->take_node_left();
				RBTreeSetLeftChild(pNode->mpNodeParent, std::move(pNodeChildOwn));  // Make pNode's replacement node be on the same side.
			}
			else {
				pNodeOwning = pNode->mpNodeParent->take_node_right();
				RBTreeSetRightChild(pNode->mpNodeParent, std::move(pNodeChildOwn));
			}
		
		}
		//both childs are non-null
		else if(!pNode->mpNodeRight->mpNodeLeft) {
			//the node at the right doesn't have a left child,
			pNodeChildParent = pNode->mpNodeRight;


			rbtree_owning_ptr pNodeLeft = pNode->take_node_left();
			rbtree_owning_ptr pNodeSuccessor = pNode->take_node_right();
			RBTreeSetLeftChild(pNodeSuccessor, std::move(pNodeLeft));
			pNodeChild = pNodeSuccessor->mpNodeRight;

			using std::swap;
			swap(pNodeSuccessor->mColor, pNode->mColor);

			if(pNode == pNode->mpNodeParent->mpNodeLeft) {// If pNode is a left node...
				pNodeOwning = pNode->mpNodeParent->take_node_left();
				RBTreeSetLeftChild(pNode->mpNodeParent, std::move(pNodeSuccessor));  // Make pNode's replacement node be on the same side.
			}
			else {
				pNodeOwning = pNode->mpNodeParent->take_node_right();
				RBTreeSetRightChild(pNode->mpNodeParent, std::move(pNodeSuccessor));
			}
		}
		else {
			// if we are here, pNode has two childs and the child on the right
			// is not the succesors, go down the right child to find next


			rbtree_soft_ptr pNodeSuccessorSoft = RBTreeIncrement(pNode);
			rbtree_soft_ptr pNodeSuccessorParent = pNodeSuccessorSoft->mpNodeParent;
			pNodeChildParent = pNodeSuccessorParent;
//			rbtree_soft_ptr pNodeChildParent = RBTreeIncrement(pNode)->mpNodeParent;

			EASTL_ASSERT(pNodeSuccessorParent != pNode);
			rbtree_owning_ptr pNodeSuccessor = pNodeSuccessorParent->take_node_left();

			EASTL_ASSERT(!pNodeSuccessorOwn->mpNodeLeft);
			if(pNodeSuccessor->mpNodeRight) {
				pNodeChild = pNodeSuccessor->mpNodeRight;

				rbtree_owning_ptr pNodeSuccessorRight = pNodeSuccessor->take_node_right();
				RBTreeSetLeftChild(pNodeSuccessorParent, std::move(pNodeSuccessorRight));  // Make pNode's replacement node be on the same side.

			}
			rbtree_owning_ptr pNodeLeft = pNode->take_node_left();
			RBTreeSetLeftChild(pNodeSuccessor, std::move(pNodeLeft));  // Make pNode's replacement node be on the same side.

			rbtree_owning_ptr pNodeRight = pNode->take_node_right();
			RBTreeSetRightChild(pNodeSuccessor, std::move(pNodeRight));

			using std::swap;
			swap(pNodeSuccessor->mColor, pNode->mColor);

			if(pNode == pNode->mpNodeParent->mpNodeLeft) {// If pNode is a left node...
				pNodeOwning = pNode->mpNodeParent->take_node_left();
				RBTreeSetLeftChild(pNode->mpNodeParent, std::move(pNodeSuccessor));  // Make pNode's replacement node be on the same side.
			}
			else {
				pNodeOwning = pNode->mpNodeParent->take_node_right();
				RBTreeSetRightChild(pNode->mpNodeParent, std::move(pNodeSuccessor));
			}
		}


		// if(pNodeSuccessor->mpNodeLeft == NULL)         // pNode has at most one non-NULL child.
		// 	pNodeChild = pNodeSuccessor->mpNodeRight;  // pNodeChild might be null.
		// else if(pNodeSuccessor->mpNodeRight == NULL)   // pNode has exactly one non-NULL child.
		// 	pNodeChild = pNodeSuccessor->mpNodeLeft;   // pNodeChild is not null.
		// else 
		// {
		// 	// pNode has two non-null children. Set pNodeSuccessor to pNode's successor. pNodeChild might be NULL.
		// 	pNodeSuccessor = pNodeSuccessor->mpNodeRight;

		// 	while(pNodeSuccessor->mpNodeLeft)
		// 		pNodeSuccessor = pNodeSuccessor->mpNodeLeft;

		// 	pNodeChild = pNodeSuccessor->mpNodeRight;
		// }

		// // Here we remove pNode from the tree and fix up the node pointers appropriately around it.
		// if(pNodeSuccessor == pNode) // If pNode was a leaf node (had both NULL children)...
		// {
		// 	pNodeChildParent = pNodeSuccessor->mpNodeParent;  // Assign pNodeReplacement's parent.

		// 	if(pNodeChild) 
		// 		pNodeChild->mpNodeParent = pNodeSuccessor->mpNodeParent;

		// 	if(pNode == pNodeAnchor->mpNodeLeft) // If the node being deleted is the root node...
		// 		pNodeAnchor->mpNodeLeft = pNodeChild; // Set the new root node to be the pNodeReplacement.
		// 	else 
		// 	{
		// 		if(pNode == pNode->mpNodeParent->mpNodeLeft) // If pNode is a left node...
		// 			pNode->mpNodeParent->mpNodeLeft  = pNodeChild;  // Make pNode's replacement node be on the same side.
		// 		else
		// 			pNode->mpNodeParent->mpNodeRight = pNodeChild;
		// 		// Now pNode is disconnected from the bottom of the tree (recall that in this pathway pNode was determined to be a leaf).
		// 	}

		// 	if(pNode == pNodeLeftmostRef) // If pNode is the tree begin() node...
		// 	{
		// 		// Because pNode is the tree begin(), pNode->mpNodeLeft must be NULL.
		// 		// Here we assign the new begin() (first node).
		// 		if(pNode->mpNodeRight && pNodeChild)
		// 		{
		// 			EASTL_ASSERT(pNodeChild != NULL); // Logically pNodeChild should always be valid.
		// 			pNodeLeftmostRef = RBTreeGetMinChild(pNodeChild); 
		// 		}
		// 		else
		// 			pNodeLeftmostRef = pNode->mpNodeParent; // This  makes (pNodeLeftmostRef == end()) if (pNode == root node)
		// 	}

		// 	if(pNode == pNodeRightmostRef) // If pNode is the tree last (rbegin()) node...
		// 	{
		// 		// Because pNode is the tree rbegin(), pNode->mpNodeRight must be NULL.
		// 		// Here we assign the new rbegin() (last node)
		// 		if(pNode->mpNodeLeft && pNodeChild)
		// 		{
		// 			EASTL_ASSERT(pNodeChild != NULL); // Logically pNodeChild should always be valid.
		// 			pNodeRightmostRef = RBTreeGetMaxChild(pNodeChild);
		// 		}
		// 		else // pNodeChild == pNode->mpNodeLeft
		// 			pNodeRightmostRef = pNode->mpNodeParent; // makes pNodeRightmostRef == &mAnchor if pNode == pNodeRootRef
		// 	}
		// }
		// else // else (pNodeSuccessor != pNode)
		// {
		// 	// Relink pNodeSuccessor in place of pNode. pNodeSuccessor is pNode's successor.
		// 	// We specifically set pNodeSuccessor to be on the right child side of pNode, so fix up the left child side.
		// 	pNode->mpNodeLeft->mpNodeParent = pNodeSuccessor; 
		// 	pNodeSuccessor->mpNodeLeft = pNode->mpNodeLeft;

		// 	if(pNodeSuccessor == pNode->mpNodeRight) // If pNode's successor was at the bottom of the tree... (yes that's effectively what this statement means)
		// 		pNodeChildParent = pNodeSuccessor; // Assign pNodeReplacement's parent.
		// 	else
		// 	{
		// 		pNodeChildParent = pNodeSuccessor->mpNodeParent;

		// 		if(pNodeChild)
		// 			pNodeChild->mpNodeParent = pNodeChildParent;

		// 		pNodeChildParent->mpNodeLeft = pNodeChild;

		// 		pNodeSuccessor->mpNodeRight = pNode->mpNodeRight;
		// 		pNode->mpNodeRight->mpNodeParent = pNodeSuccessor;
		// 	}

		// 	if(pNode == pNodeAnchor->mpNodeLeft)
		// 		pNodeAnchor->mpNodeLeft = pNodeSuccessor;
		// 	else if(pNode == pNode->mpNodeParent->mpNodeLeft)
		// 		pNode->mpNodeParent->mpNodeLeft = pNodeSuccessor;
		// 	else 
		// 		pNode->mpNodeParent->mpNodeRight = pNodeSuccessor;

		// 	// Now pNode is disconnected from the tree.

		// 	pNodeSuccessor->mpNodeParent = pNode->mpNodeParent;
		// 	using std::swap;
		// 	swap(pNodeSuccessor->mColor, pNode->mColor);
		// }


		// Here we do tree balancing as per the conventional red-black tree algorithm.
		if(pNode->mColor == kRBTreeColorBlack) 
		{ 
			while((pNodeChild != pNodeAnchor->mpNodeLeft) && ((pNodeChild == nullptr) || (pNodeChild->mColor == kRBTreeColorBlack)))
			{
				if(pNodeChild == pNodeChildParent->mpNodeLeft) 
				{
					rbtree_soft_ptr pNodeTemp = pNodeChildParent->mpNodeRight;

					if(pNodeTemp->mColor == kRBTreeColorRed) 
					{
						pNodeTemp->mColor = kRBTreeColorBlack;
						pNodeChildParent->mColor = kRBTreeColorRed;
						// pNodeRootRef = RBTreeRotateLeft(pNodeChildParent, pNodeRootRef);
						RBTreeRotateLeft(pNodeChildParent);
						pNodeTemp = pNodeChildParent->mpNodeRight;
					}

					if(((pNodeTemp->mpNodeLeft  == nullptr) || (pNodeTemp->mpNodeLeft->mColor  == kRBTreeColorBlack)) &&
						((pNodeTemp->mpNodeRight == nullptr) || (pNodeTemp->mpNodeRight->mColor == kRBTreeColorBlack))) 
					{
						pNodeTemp->mColor = kRBTreeColorRed;
						pNodeChild = pNodeChildParent;
						pNodeChildParent = pNodeChildParent->mpNodeParent;
					} 
					else 
					{
						if((pNodeTemp->mpNodeRight == nullptr) || (pNodeTemp->mpNodeRight->mColor == kRBTreeColorBlack)) 
						{
							pNodeTemp->mpNodeLeft->mColor = kRBTreeColorBlack;
							pNodeTemp->mColor = kRBTreeColorRed;
							// pNodeRootRef = RBTreeRotateRight(pNodeTemp, pNodeRootRef);
							RBTreeRotateRight(pNodeTemp);
							pNodeTemp = pNodeChildParent->mpNodeRight;
						}

						pNodeTemp->mColor = pNodeChildParent->mColor;
						pNodeChildParent->mColor = kRBTreeColorBlack;

						if(pNodeTemp->mpNodeRight) 
							pNodeTemp->mpNodeRight->mColor = kRBTreeColorBlack;

						// pNodeRootRef = RBTreeRotateLeft(pNodeChildParent, pNodeRootRef);
						RBTreeRotateLeft(pNodeChildParent);
						break;
					}
				} 
				else 
				{   
					// The following is the same as above, with mpNodeRight <-> mpNodeLeft.
					rbtree_soft_ptr pNodeTemp = pNodeChildParent->mpNodeLeft;

					if(pNodeTemp->mColor == kRBTreeColorRed) 
					{
						pNodeTemp->mColor        = kRBTreeColorBlack;
						pNodeChildParent->mColor = kRBTreeColorRed;

						// pNodeRootRef = RBTreeRotateRight(pNodeChildParent, pNodeRootRef);
						RBTreeRotateRight(pNodeChildParent);
						pNodeTemp = pNodeChildParent->mpNodeLeft;
					}

					if(((pNodeTemp->mpNodeRight == nullptr) || (pNodeTemp->mpNodeRight->mColor == kRBTreeColorBlack)) &&
						((pNodeTemp->mpNodeLeft  == nullptr) || (pNodeTemp->mpNodeLeft->mColor  == kRBTreeColorBlack))) 
					{
						pNodeTemp->mColor = kRBTreeColorRed;
						pNodeChild       = pNodeChildParent;
						pNodeChildParent = pNodeChildParent->mpNodeParent;
					} 
					else 
					{
						if((pNodeTemp->mpNodeLeft == nullptr) || (pNodeTemp->mpNodeLeft->mColor == kRBTreeColorBlack)) 
						{
							pNodeTemp->mpNodeRight->mColor = kRBTreeColorBlack;
							pNodeTemp->mColor              = kRBTreeColorRed;

							// pNodeRootRef = RBTreeRotateLeft(pNodeTemp, pNodeRootRef);
							RBTreeRotateLeft(pNodeTemp);
							pNodeTemp = pNodeChildParent->mpNodeLeft;
						}

						pNodeTemp->mColor = pNodeChildParent->mColor;
						pNodeChildParent->mColor = kRBTreeColorBlack;

						if(pNodeTemp->mpNodeLeft) 
							pNodeTemp->mpNodeLeft->mColor = kRBTreeColorBlack;

						// pNodeRootRef = RBTreeRotateRight(pNodeChildParent, pNodeRootRef);
						RBTreeRotateRight(pNodeChildParent);
						break;
					}
				}
			}

			if(pNodeChild)
				pNodeChild->mColor = kRBTreeColorBlack;
		}

		return pNodeOwning;
	} // RBTreeErase



} // namespace nodecpp

