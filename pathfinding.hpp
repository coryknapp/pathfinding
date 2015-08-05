#ifndef PATHFINDING_HPP_DHFQEVLB
#define PATHFINDING_HPP_DHFQEVLB


#include <limits>
#include <vector>
#include <algorithm>

namespace pf{

//Adaptors don't need to inherit from this Adaptor, but they need to define
//everything below:
//	node_t, getAdjacentNodes(...), and heuristicDistanceBetweenAdjacentNodes(..)
//See test.cpp for example.
template<typename NODE>	
class Adaptor{
public:
	//There are two requirements for a 'node' class
	//1.Your adaptor class must be able to find a node's adjacent nodes using
	//	only it's members.
	//2.The class must define the '==' operator.
	typedef NODE node_t;
	
	//getAdjacentNodes(...) should return a vector of nodes that connect to it's
	//augment.
	virtual std::vector<NODE*> getAdjacentNodes(const node_t&) = 0;

	//heuristicDistanceBetweenAdjacentNodes(...) should return the heuristic
	//estimate for the work done to travel between the supplied nodes.
	//TODO: there's no reason why it has to be unsigned, we could template in
	//any type the user might like.
	virtual unsigned heuristicDistanceBetweenAdjacentNodes(const node_t&,
														   const node_t&) = 0;
};

template<typename ADAPTOR>
class Search{

public:
	typedef typename ADAPTOR::node_t node_t;


	Search(	const node_t& start,const node_t& end){

		//create the first internal node, the ancestor to all nodes.
		InternalNode * startNodePtr = newInternalNode();
		startNodePtr->externalNode = &start;
		startNodePtr->h = 0;
		startNodePtr->g = 0;
		startNodePtr->f = 0;
		startNodePtr->graphLength = 1;
		m_openList.push_back( startNodePtr );
		while( !m_openList.empty() ){
			// find the openList with the lowest 'f' score.
			std::sort( m_openList.begin(), m_openList.end(),
				[]( const InternalNode* a, const InternalNode* b ) -> bool{
					return a->f > b->f;
				});
    		//pop q off the open lit
			auto q = m_openList[ m_openList.size()-1 ];
			m_openList.pop_back();
			//generate q's successors and set their parents to q
			auto successorList =
				adaptorFunctor.getAdjacentNodes(*(q->externalNode));
			for (auto &externalNodeSuccessor : successorList) {
				//check to see it this is the end node
				if( *externalNodeSuccessor == end ){
					//create a final internal node to make it easier when we go
					//to return our results
					m_lastNode = newInternalNode();
					m_lastNode->externalNode = &end;
					m_lastNode->parent = q;
					m_lastNode->graphLength = q->graphLength +1;
					return;
				}
				InternalNode * newNodePtr = newInternalNode();
				newNodePtr->externalNode = externalNodeSuccessor;
				newNodePtr->g = q->g +
					adaptorFunctor.heuristicDistanceBetweenAdjacentNodes(
						*newNodePtr->externalNode, *q->externalNode );
				newNodePtr->h =
					adaptorFunctor.heuristicDistanceBetweenAdjacentNodes(
						*newNodePtr->externalNode, end );
				newNodePtr->f = newNodePtr->g + newNodePtr->h;
				newNodePtr->graphLength = q->graphLength+1;
				newNodePtr->parent = q;
				bool addFlag = true;
				//if a node with the same position as successor is in the OPEN
				//list which has a lower f than successor, skip this successor
				auto internalWithSameExternal = std::find_if(
					m_openList.begin(), m_openList.end(),
					[&newNodePtr](InternalNode * &oldNodePtr){
						return *(newNodePtr->externalNode) ==
							*(oldNodePtr->externalNode);
					});
				if( internalWithSameExternal != m_openList.end() ){
					if( (*internalWithSameExternal)->f < newNodePtr->f )
						addFlag = false;
				}
				//if a node with the same position as successor is in the CLOSED
				// list which has a lower f than successor, skip this successor
        		internalWithSameExternal = std::find_if(
					m_closedList.begin(), m_closedList.end(),
					[&newNodePtr](InternalNode * &oldNodePtr){
						return *(newNodePtr->externalNode) ==
							*(oldNodePtr->externalNode);
					});
				if( internalWithSameExternal != m_closedList.end() ){
					if( (*internalWithSameExternal)->f < newNodePtr->f )
						addFlag = false;
				}
				//otherwise, add the node to the open list
				if( addFlag )
					m_openList.push_back( newNodePtr );
				else
					delete newNodePtr;
			}
			m_closedList.push_back( q );
		}
	}

	std::vector<const node_t*> path(){
		if( m_lastNode ){
			std::vector<const node_t*> returnList;
			returnList.resize
			( m_lastNode->graphLength );
			InternalNode * runner = m_lastNode;
			size_t index = m_lastNode->graphLength-1;
			while( runner ){
				returnList[index] = runner->externalNode;
				--index;
				runner = runner->parent;
			}
			clean();
			return returnList;
		}
		clean();
		return std::vector<const node_t*>();
	}
	
	void clean(){
		m_nodeList.clear();
	}

private:
	//Internally, 
	struct InternalNode{
		
		const node_t * externalNode;
		InternalNode * parent = nullptr;
		unsigned f, g, h;
		unsigned graphLength;
		
	};
	
	InternalNode * newInternalNode(){
		m_nodeList.push_back( std::unique_ptr<InternalNode>( new InternalNode ) );
		return m_nodeList[m_nodeList.size()-1].get();
	}
	
	//we have to use a list of pointers, rather then contiguous memory because
	//of the graph structure.  We can't have InternalNodes moving around in
	//memory.
	std::vector<InternalNode*> m_openList;
	std::vector<InternalNode*> m_closedList;
	std::vector<std::unique_ptr<InternalNode>> m_nodeList;

	InternalNode * m_lastNode = nullptr;
	
	ADAPTOR adaptorFunctor;
};

} /* pf */


#endif /* end of include guard: PATHFINDING_HPP_DHFQEVLB */
