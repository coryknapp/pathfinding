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

	Search(	const node_t& start,const node_t& end, ARGS... args) :
		end( *&end ){
		m_adaptor = std::unique_ptr<ADAPTOR>( new ADAPTOR( args... ) );
		//create the first internal node, the ancestor to all nodes, and
		//initialize it.
		const InternalNode * startNodePtr = newInternalNode( nullptr, &start );
		m_openList.push_back( startNodePtr );
		const InternalNode * q = popInternalNode();

		while( !m_openList.empty() ){
			// find the node on the open list with the lowest 'f' score.
			//generate q's successors
			auto successorList =
				m_adaptor->getAdjacentNodes(*(q->externalNode));
			for( auto &externalNodeSuccessor : successorList ) {
				//check to see it this is the end node
				if( *externalNodeSuccessor == end ){
					//create a final internal node to make it easier when we go
					//to return our results
					m_lastNode = newInternalNode( q, &end);
					return;
				}
				// create and initialize a new internal node for each of q's
				// successors
				const InternalNode * newNodePtr = newInternalNode( q, externalNodeSuccessor);
				
				//addFlag can be changed by the two conditionals below if the
				//new node is unsuitable.
				bool addFlag = true;
				//if a node with the same position as successor is in the OPEN
				//list which has a lower f than successor, skip this successor
				auto internalWithSameExternal = std::find_if(
					m_openList.begin(), m_openList.end(),
					[&newNodePtr](const InternalNode * &oldNodePtr){
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
					[&newNodePtr](const InternalNode * &oldNodePtr){
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
			const InternalNode * runner = m_lastNode;
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
		
		InternalNode( const node_t * externalNode,
					  const InternalNode * parent,
					  unsigned f, unsigned g, unsigned h,
					  unsigned graphLength ) :
    		externalNode( externalNode ),
    		parent( parent ),
    		f( f ), g( g ), h( h ),
    		graphLength( graphLength ){}
		const node_t * externalNode;
		const InternalNode * parent;
		const unsigned f, g, h;
		const unsigned graphLength;
		
		~InternalNode(){
			std::cout << "deleting " << externalNode << std::endl;  
		}
	};
	
	//Internally, get new nodes here.  Every new node as added to this list and
	//wrapped in a unique_ptr, so when we call clean, or the Search goes out of
	//scope, we can be sure we didn't leak any nodes out of what can end up
	//being a complicated set of graphs.  Poor man's garbage collector.
	const InternalNode * newInternalNode( const InternalNode * parent,
								    const node_t * externalNode){

		InternalNode * newNode;
		
		std::cout << "new internal = " << externalNode << std::endl
		;
		
		if( parent == nullptr ){
			newNode = new InternalNode( externalNode, parent, 0, 0, 0, 1);
		} else {
			//calculate the new node's scored
			newNode = new InternalNode( externalNode, parent,
					newNode->g + newNode->h,
					parent->g + m_adaptor->heuristicDistanceBetweenAdjacentNodes(
						*externalNode, *parent->externalNode ),
    				m_adaptor->heuristicDistanceBetweenAdjacentNodes(
					*externalNode, end ),
					parent->graphLength+1 );

		}
		m_nodeList.push_back( std::unique_ptr<InternalNode>( newNode ) );
		return newNode;
	}
	void clean(){
		m_nodeList.clear();
	}

	const InternalNode * popInternalNode(){
		std::sort( m_openList.begin(), m_openList.end(),
			//TODO we don't really need to sort this list, we just need to
			//find the smallest f value.
			[]( const InternalNode* a, const InternalNode* b ) -> bool{
				return a->f > b->f;
			});
		//pop q off the open list
		const InternalNode * q = m_openList[ m_openList.size()-1 ];
		m_openList.pop_back();
		return q;
	}
	
	// we need to know the end node when we calculate scores in
	// newInternalNode()
	const node_t &end;
	
	//we have to use a list of pointers, rather then contiguous memory because
	//of the graph structure.  We can't have InternalNodes moving around in
	//memory.
	std::vector<const InternalNode*> m_openList;
	std::vector<const InternalNode*> m_closedList;

	//for the poor man's garbage collector. 
	std::vector<std::unique_ptr<InternalNode>> m_nodeList;

	const InternalNode * m_lastNode = nullptr;
	
	std::unique_ptr<ADAPTOR> m_adaptor;
};

} /* pf */


#endif /* end of include guard: PATHFINDING_HPP_DHFQEVLB */
