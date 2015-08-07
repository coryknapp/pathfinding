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

	//An adaptor can have a constructor, we just need to pass the parameters
	//types into the Search templates, and the arguments into the Search
	//constructor

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

template<typename ADAPTOR, typename... ARGS>
class Search{

public:
	typedef typename ADAPTOR::node_t node_t;

	Search(	const node_t& start,const node_t& end, ARGS... args){
		m_adaptor = std::unique_ptr<ADAPTOR>( new ADAPTOR( args... ) );
		//create the first internal node, the ancestor to all nodes, and
		//initialize it.
		InternalNode * startNodePtr = newInternalNode();
		//copy the address of the user defined node.
		//TODO there could be cases where the user's node may be freed. so we
		//put in an option to copy nodes if necessary.
		startNodePtr->externalNode = &start;
		//clear the scores for the new node.
		startNodePtr->h = 0;
		startNodePtr->g = 0;
		startNodePtr->f = 0;
		//keep track of how long the paths are to ease allocating a vector to
		//hold the path later
		startNodePtr->graphLength = 1;
		//put node in the list of candidate nodes.
		m_openList.push_back( startNodePtr );

		while( !m_openList.empty() ){
			// find the node on the open list with the lowest 'f' score.
			std::sort( m_openList.begin(), m_openList.end(),
				//TODO we don't really need to sort this list, we just need to
				//find the smallest f value.
				[]( const InternalNode* a, const InternalNode* b ) -> bool{
					return a->f > b->f;
				});
    		//pop q off the open list
			auto q = m_openList[ m_openList.size()-1 ];
			m_openList.pop_back();
			//generate q's successors
			auto successorList =
				m_adaptor->getAdjacentNodes(*(q->externalNode));
			for( auto &externalNodeSuccessor : successorList ) {
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
				// create and initialize a new internal node for each of q's
				// successors
				InternalNode * newNodePtr = newInternalNode();
				//copy address of the external node. 
				newNodePtr->externalNode = externalNodeSuccessor;
				//calculate the new node's scored
				newNodePtr->g = q->g +
					m_adaptor->heuristicDistanceBetweenAdjacentNodes(
						*newNodePtr->externalNode, *q->externalNode );
				newNodePtr->h =
					m_adaptor->heuristicDistanceBetweenAdjacentNodes(
						*newNodePtr->externalNode, end );
				newNodePtr->f = newNodePtr->g + newNodePtr->h;
				newNodePtr->graphLength = q->graphLength+1;
				newNodePtr->parent = q;
				//addFlag can be changed by the two conditionals below if the
				//new node is unsuitable.
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
		// we've exhausted the open list and never find the end node, so there
		// is no path there.
		// When the user tries to call path(), the class will see that
		// m_lastNode is still set to nullptr, and return an empty path.
		clean();
	}

	std::vector<const node_t*> path(){
		// the path is not yet in a usable form.  We need to copy it into a
		// nice vector, without all of our internal node garbage.
		// Firstly, did we even find a path?
		if( m_lastNode ){
			//if so allocated enough room in a vector and copy in the pointers
			//back to front as we travel our graph from end to start
			std::vector<const node_t*> returnList;
			returnList.resize( m_lastNode->graphLength );
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

private:
	//Internally, we wrap each user node into an InternalNode.  We need to keep
	//track of some stuff for our algorithm to work.
	struct InternalNode{
		
		const node_t * externalNode;
		InternalNode * parent = nullptr;
		unsigned f, g, h;
		unsigned graphLength;
		
	};
	
	//Internally, get new nodes here.  Every new node as added to this list and
	//wrapped in a unique_ptr, so when we call clean, or the Search goes out of
	//scope, we can be sure we didn't leak any nodes out of what can end up
	//being a complicated set of graphs.  Poor man's garbage collector.
	InternalNode * newInternalNode(){
		m_nodeList.push_back( std::unique_ptr<InternalNode>( new InternalNode ) );
		return m_nodeList[m_nodeList.size()-1].get();
	}
	void clean(){
		m_nodeList.clear();
	}

	//we have to use a list of pointers, rather then contiguous memory because
	//of the graph structure.  We can't have InternalNodes moving around in
	//memory.
	std::vector<InternalNode*> m_openList;
	std::vector<InternalNode*> m_closedList;

	//for the poor man's garbage collector. 
	std::vector<std::unique_ptr<InternalNode>> m_nodeList;

	InternalNode * m_lastNode = nullptr;
	
	std::unique_ptr<ADAPTOR> m_adaptor;
};

} /* pf */


#endif /* end of include guard: PATHFINDING_HPP_DHFQEVLB */
