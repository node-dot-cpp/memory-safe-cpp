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
	rbtree_node_base* RBTreeRotateLeft(rbtree_node_base* pNode, rbtree_node_base* pNodeRoot);
	rbtree_node_base* RBTreeRotateRight(rbtree_node_base* pNode, rbtree_node_base* pNodeRoot);



	/// RBTreeIncrement
	/// Returns the next item in a sorted red-black tree.
	///
	EASTL_API rbtree_node_base* RBTreeIncrement(const rbtree_node_base* pNode)
	{
		//mb: increment of end() is disallowed before getting here
		EASTL_ASSERT(pNode->mpNodeParent);
		
		if(pNode->mpNodeRight) 
		{
			pNode = pNode->mpNodeRight;

			while(pNode->mpNodeLeft)
				pNode = pNode->mpNodeLeft;
		}
		else 
		{
			rbtree_node_base* pNodeTemp = pNode->mpNodeParent;

			while(pNode == pNodeTemp->mpNodeRight) 
			{
				pNode = pNodeTemp;
				pNodeTemp = pNodeTemp->mpNodeParent;
			}

			//mb: what is this?
			// if(pNode->mpNodeRight != pNodeTemp)
				pNode = pNodeTemp;
		}

		return const_cast<rbtree_node_base*>(pNode);
	}



	/// RBTreeIncrement
	/// Returns the previous item in a sorted red-black tree.
	///
	EASTL_API rbtree_node_base* RBTreeDecrement(const rbtree_node_base* pNode)
	{
		// mb: decrement from end() works normally now
		// if((pNode->mpNodeParent->mpNodeParent == pNode) && (pNode->mColor == kRBTreeColorRed))
		// 	return pNode->mpNodeRight; 
		// else
		if(pNode->mpNodeLeft)
		{
			rbtree_node_base* pNodeTemp = pNode->mpNodeLeft;

			while(pNodeTemp->mpNodeRight)
				pNodeTemp = pNodeTemp->mpNodeRight;

			return pNodeTemp;
		}

		rbtree_node_base* pNodeTemp = pNode->mpNodeParent;

		while(pNodeTemp && pNode == pNodeTemp->mpNodeLeft) 
		{
			pNode     = pNodeTemp;
			pNodeTemp = pNodeTemp->mpNodeParent;
		}

		return const_cast<rbtree_node_base*>(pNodeTemp);
	}



	/// RBTreeGetBlackCount
	/// Counts the number of black nodes in an red-black tree, from pNode down to the given bottom node.  
	/// We don't count red nodes because red-black trees don't really care about
	/// red node counts; it is black node counts that are significant in the 
	/// maintenance of a balanced tree.
	///
	EASTL_API size_t RBTreeGetBlackCount(const rbtree_node_base* pNodeTop, const rbtree_node_base* pNodeBottom)
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
	rbtree_node_base* RBTreeRotateLeft(rbtree_node_base* pNode, rbtree_node_base* pNodeRoot)
	{
		rbtree_node_base* const pNodeTemp = pNode->mpNodeRight;

		pNode->mpNodeRight = pNodeTemp->mpNodeLeft;

		if(pNodeTemp->mpNodeLeft)
			pNodeTemp->mpNodeLeft->mpNodeParent = pNode;
		pNodeTemp->mpNodeParent = pNode->mpNodeParent;
		
		if(pNode == pNodeRoot)
			pNodeRoot = pNodeTemp;
		else if(pNode == pNode->mpNodeParent->mpNodeLeft)
			pNode->mpNodeParent->mpNodeLeft = pNodeTemp;
		else
			pNode->mpNodeParent->mpNodeRight = pNodeTemp;

		pNodeTemp->mpNodeLeft = pNode;
		pNode->mpNodeParent = pNodeTemp;

		return pNodeRoot;
	}



	/// RBTreeRotateRight
	/// Does a right rotation about the given node. 
	/// If you want to understand tree rotation, any book on algorithms will
	/// discussion the topic in good detail.
	rbtree_node_base* RBTreeRotateRight(rbtree_node_base* pNode, rbtree_node_base* pNodeRoot)
	{
		rbtree_node_base* const pNodeTemp = pNode->mpNodeLeft;

		pNode->mpNodeLeft = pNodeTemp->mpNodeRight;

		if(pNodeTemp->mpNodeRight)
			pNodeTemp->mpNodeRight->mpNodeParent = pNode;
		pNodeTemp->mpNodeParent = pNode->mpNodeParent;

		if(pNode == pNodeRoot)
			pNodeRoot = pNodeTemp;
		else if(pNode == pNode->mpNodeParent->mpNodeRight)
			pNode->mpNodeParent->mpNodeRight = pNodeTemp;
		else
			pNode->mpNodeParent->mpNodeLeft = pNodeTemp;

		pNodeTemp->mpNodeRight = pNode;
		pNode->mpNodeParent = pNodeTemp;

		return pNodeRoot;
	}




	/// RBTreeInsert
	/// Insert a node into the tree and rebalance the tree as a result of the 
	/// disturbance the node introduced.
	///
	EASTL_API void RBTreeInsert(rbtree_node_base* pNode,
								rbtree_node_base* pNodeParent,
														  rbtree_node_base* pNodeAnchor,
						rbtree_min_max_nodes* pMinMaxNodes,
								RBTreeSide insertionSide)
	{
		rbtree_node_base*& pNodeRootRef = pNodeAnchor->mpNodeLeft;

		// Initialize fields in new node to insert.
		pNode->mpNodeParent = pNodeParent;
		pNode->mpNodeRight  = NULL;
		pNode->mpNodeLeft   = NULL;
		pNode->mColor       = kRBTreeColorRed;

		// Insert the node.
		if(insertionSide == kRBTreeSideLeft)
		{
			pNodeParent->mpNodeLeft = pNode;

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
			pNodeParent->mpNodeRight = pNode;

			if(pNodeParent == pMinMaxNodes->mpMaxChild)
				pMinMaxNodes->mpMaxChild = pNode; // Maintain rightmost pointing to max node
		}

		// Rebalance the tree.
		while((pNode != pNodeRootRef) && (pNode->mpNodeParent->mColor == kRBTreeColorRed)) 
		{
			EA_ANALYSIS_ASSUME(pNode->mpNodeParent != NULL);
			rbtree_node_base* const pNodeParentParent = pNode->mpNodeParent->mpNodeParent;

			if(pNode->mpNodeParent == pNodeParentParent->mpNodeLeft) 
			{
				rbtree_node_base* const pNodeTemp = pNodeParentParent->mpNodeRight;

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
						pNodeRootRef = RBTreeRotateLeft(pNode, pNodeRootRef);
					}

					EA_ANALYSIS_ASSUME(pNode->mpNodeParent != NULL);
					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					pNodeRootRef = RBTreeRotateRight(pNodeParentParent, pNodeRootRef);
				}
			}
			else 
			{
				rbtree_node_base* const pNodeTemp = pNodeParentParent->mpNodeLeft;

				if(pNodeTemp && (pNodeTemp->mColor == kRBTreeColorRed)) 
				{
					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeTemp->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					pNode = pNodeParentParent;
				}
				else 
				{
					EA_ANALYSIS_ASSUME(pNode != NULL && pNode->mpNodeParent != NULL);

					if(pNode == pNode->mpNodeParent->mpNodeLeft) 
					{
						pNode = pNode->mpNodeParent;
						pNodeRootRef = RBTreeRotateRight(pNode, pNodeRootRef);
					}

					pNode->mpNodeParent->mColor = kRBTreeColorBlack;
					pNodeParentParent->mColor = kRBTreeColorRed;
					pNodeRootRef = RBTreeRotateLeft(pNodeParentParent, pNodeRootRef);
				}
			}
		}

		EA_ANALYSIS_ASSUME(pNodeRootRef != NULL);
		pNodeRootRef->mColor = kRBTreeColorBlack;

	} // RBTreeInsert




	/// RBTreeErase
	/// Erase a node from the tree.
	///
	EASTL_API void RBTreeErase(rbtree_node_base* pNode, rbtree_node_base* pNodeAnchor,
							rbtree_min_max_nodes* pMinMaxNodes)

	{
		rbtree_node_base*& pNodeRootRef      = pNodeAnchor->mpNodeLeft;
		rbtree_node_base*& pNodeLeftmostRef  = pMinMaxNodes->mpMinChild;
		rbtree_node_base*& pNodeRightmostRef = pMinMaxNodes->mpMaxChild;
		rbtree_node_base*  pNodeSuccessor    = pNode;
		rbtree_node_base*  pNodeChild        = NULL;
		rbtree_node_base*  pNodeChildParent  = NULL;

		if(pNodeSuccessor->mpNodeLeft == NULL)         // pNode has at most one non-NULL child.
			pNodeChild = pNodeSuccessor->mpNodeRight;  // pNodeChild might be null.
		else if(pNodeSuccessor->mpNodeRight == NULL)   // pNode has exactly one non-NULL child.
			pNodeChild = pNodeSuccessor->mpNodeLeft;   // pNodeChild is not null.
		else 
		{
			// pNode has two non-null children. Set pNodeSuccessor to pNode's successor. pNodeChild might be NULL.
			pNodeSuccessor = pNodeSuccessor->mpNodeRight;

			while(pNodeSuccessor->mpNodeLeft)
				pNodeSuccessor = pNodeSuccessor->mpNodeLeft;

			pNodeChild = pNodeSuccessor->mpNodeRight;
		}

		// Here we remove pNode from the tree and fix up the node pointers appropriately around it.
		if(pNodeSuccessor == pNode) // If pNode was a leaf node (had both NULL children)...
		{
			pNodeChildParent = pNodeSuccessor->mpNodeParent;  // Assign pNodeReplacement's parent.

			if(pNodeChild) 
				pNodeChild->mpNodeParent = pNodeSuccessor->mpNodeParent;

			if(pNode == pNodeRootRef) // If the node being deleted is the root node...
				pNodeRootRef = pNodeChild; // Set the new root node to be the pNodeReplacement.
			else 
			{
				if(pNode == pNode->mpNodeParent->mpNodeLeft) // If pNode is a left node...
					pNode->mpNodeParent->mpNodeLeft  = pNodeChild;  // Make pNode's replacement node be on the same side.
				else
					pNode->mpNodeParent->mpNodeRight = pNodeChild;
				// Now pNode is disconnected from the bottom of the tree (recall that in this pathway pNode was determined to be a leaf).
			}

			if(pNode == pNodeLeftmostRef) // If pNode is the tree begin() node...
			{
				// Because pNode is the tree begin(), pNode->mpNodeLeft must be NULL.
				// Here we assign the new begin() (first node).
				if(pNode->mpNodeRight && pNodeChild)
				{
					EASTL_ASSERT(pNodeChild != NULL); // Logically pNodeChild should always be valid.
					pNodeLeftmostRef = RBTreeGetMinChild(pNodeChild); 
				}
				else
					pNodeLeftmostRef = pNode->mpNodeParent; // This  makes (pNodeLeftmostRef == end()) if (pNode == root node)
			}

			if(pNode == pNodeRightmostRef) // If pNode is the tree last (rbegin()) node...
			{
				// Because pNode is the tree rbegin(), pNode->mpNodeRight must be NULL.
				// Here we assign the new rbegin() (last node)
				if(pNode->mpNodeLeft && pNodeChild)
				{
					EASTL_ASSERT(pNodeChild != NULL); // Logically pNodeChild should always be valid.
					pNodeRightmostRef = RBTreeGetMaxChild(pNodeChild);
				}
				else // pNodeChild == pNode->mpNodeLeft
					pNodeRightmostRef = pNode->mpNodeParent; // makes pNodeRightmostRef == &mAnchor if pNode == pNodeRootRef
			}
		}
		else // else (pNodeSuccessor != pNode)
		{
			// Relink pNodeSuccessor in place of pNode. pNodeSuccessor is pNode's successor.
			// We specifically set pNodeSuccessor to be on the right child side of pNode, so fix up the left child side.
			pNode->mpNodeLeft->mpNodeParent = pNodeSuccessor; 
			pNodeSuccessor->mpNodeLeft = pNode->mpNodeLeft;

			if(pNodeSuccessor == pNode->mpNodeRight) // If pNode's successor was at the bottom of the tree... (yes that's effectively what this statement means)
				pNodeChildParent = pNodeSuccessor; // Assign pNodeReplacement's parent.
			else
			{
				pNodeChildParent = pNodeSuccessor->mpNodeParent;

				if(pNodeChild)
					pNodeChild->mpNodeParent = pNodeChildParent;

				pNodeChildParent->mpNodeLeft = pNodeChild;

				pNodeSuccessor->mpNodeRight = pNode->mpNodeRight;
				pNode->mpNodeRight->mpNodeParent = pNodeSuccessor;
			}

			if(pNode == pNodeRootRef)
				pNodeRootRef = pNodeSuccessor;
			else if(pNode == pNode->mpNodeParent->mpNodeLeft)
				pNode->mpNodeParent->mpNodeLeft = pNodeSuccessor;
			else 
				pNode->mpNodeParent->mpNodeRight = pNodeSuccessor;

			// Now pNode is disconnected from the tree.

			pNodeSuccessor->mpNodeParent = pNode->mpNodeParent;
			using std::swap;
			swap(pNodeSuccessor->mColor, pNode->mColor);
		}

		// Here we do tree balancing as per the conventional red-black tree algorithm.
		if(pNode->mColor == kRBTreeColorBlack) 
		{ 
			while((pNodeChild != pNodeRootRef) && ((pNodeChild == NULL) || (pNodeChild->mColor == kRBTreeColorBlack)))
			{
				if(pNodeChild == pNodeChildParent->mpNodeLeft) 
				{
					rbtree_node_base* pNodeTemp = pNodeChildParent->mpNodeRight;

					if(pNodeTemp->mColor == kRBTreeColorRed) 
					{
						pNodeTemp->mColor = kRBTreeColorBlack;
						pNodeChildParent->mColor = kRBTreeColorRed;
						pNodeRootRef = RBTreeRotateLeft(pNodeChildParent, pNodeRootRef);
						pNodeTemp = pNodeChildParent->mpNodeRight;
					}

					if(((pNodeTemp->mpNodeLeft  == NULL) || (pNodeTemp->mpNodeLeft->mColor  == kRBTreeColorBlack)) &&
						((pNodeTemp->mpNodeRight == NULL) || (pNodeTemp->mpNodeRight->mColor == kRBTreeColorBlack))) 
					{
						pNodeTemp->mColor = kRBTreeColorRed;
						pNodeChild = pNodeChildParent;
						pNodeChildParent = pNodeChildParent->mpNodeParent;
					} 
					else 
					{
						if((pNodeTemp->mpNodeRight == NULL) || (pNodeTemp->mpNodeRight->mColor == kRBTreeColorBlack)) 
						{
							pNodeTemp->mpNodeLeft->mColor = kRBTreeColorBlack;
							pNodeTemp->mColor = kRBTreeColorRed;
							pNodeRootRef = RBTreeRotateRight(pNodeTemp, pNodeRootRef);
							pNodeTemp = pNodeChildParent->mpNodeRight;
						}

						pNodeTemp->mColor = pNodeChildParent->mColor;
						pNodeChildParent->mColor = kRBTreeColorBlack;

						if(pNodeTemp->mpNodeRight) 
							pNodeTemp->mpNodeRight->mColor = kRBTreeColorBlack;

						pNodeRootRef = RBTreeRotateLeft(pNodeChildParent, pNodeRootRef);
						break;
					}
				} 
				else 
				{   
					// The following is the same as above, with mpNodeRight <-> mpNodeLeft.
					rbtree_node_base* pNodeTemp = pNodeChildParent->mpNodeLeft;

					if(pNodeTemp->mColor == kRBTreeColorRed) 
					{
						pNodeTemp->mColor        = kRBTreeColorBlack;
						pNodeChildParent->mColor = kRBTreeColorRed;

						pNodeRootRef = RBTreeRotateRight(pNodeChildParent, pNodeRootRef);
						pNodeTemp = pNodeChildParent->mpNodeLeft;
					}

					if(((pNodeTemp->mpNodeRight == NULL) || (pNodeTemp->mpNodeRight->mColor == kRBTreeColorBlack)) &&
						((pNodeTemp->mpNodeLeft  == NULL) || (pNodeTemp->mpNodeLeft->mColor  == kRBTreeColorBlack))) 
					{
						pNodeTemp->mColor = kRBTreeColorRed;
						pNodeChild       = pNodeChildParent;
						pNodeChildParent = pNodeChildParent->mpNodeParent;
					} 
					else 
					{
						if((pNodeTemp->mpNodeLeft == NULL) || (pNodeTemp->mpNodeLeft->mColor == kRBTreeColorBlack)) 
						{
							pNodeTemp->mpNodeRight->mColor = kRBTreeColorBlack;
							pNodeTemp->mColor              = kRBTreeColorRed;

							pNodeRootRef = RBTreeRotateLeft(pNodeTemp, pNodeRootRef);
							pNodeTemp = pNodeChildParent->mpNodeLeft;
						}

						pNodeTemp->mColor = pNodeChildParent->mColor;
						pNodeChildParent->mColor = kRBTreeColorBlack;

						if(pNodeTemp->mpNodeLeft) 
							pNodeTemp->mpNodeLeft->mColor = kRBTreeColorBlack;

						pNodeRootRef = RBTreeRotateRight(pNodeChildParent, pNodeRootRef);
						break;
					}
				}
			}

			if(pNodeChild)
				pNodeChild->mColor = kRBTreeColorBlack;
		}

	} // RBTreeErase



} // namespace nodecpp

