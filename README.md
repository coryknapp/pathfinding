# pathfinding
Easy A* pathfinding on your exisiting graph structures in C++

All you need to do is implement a sub class of the adaptor below

```c++
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
	virtual unsigned heuristicDistanceBetweenAdjacentNodes(const node_t&,
														   const node_t&) = 0;
};

```

## Who should use this?

 pathfinding library is a good fit for your projects if:
 1.	Each node has knowlage of it's adjacent nodes, rather then maintaining those
 relationships in a seperate graph object
 2. The memory management of nodes is handled by your program.  Meaning any
 nodes returned by getAdjacentNodes must later be deleted by you.

## Example

```c++
	struct SimpleNode{
		
		SimpleNode( char name ) : name(name) {}

		char name;
		std::vector<SimpleNode*>adjacentNodes;

		//you can define the == operator pretty much anyway you'd like
		//even comparing memory locations works
		bool operator==(const SimpleNode& rhs) const{
			return ( this == &rhs );
		}
		
	};

	class SimpleAdaptor : Adaptor<SimpleNode>
	{
	public:
		using Adaptor::node_t;
		
		std::vector<node_t*> getAdjacentNodes(const node_t& node){
			return node.adjacentNodes;
		}

		unsigned heuristicDistanceBetweenAdjacentNodes(const node_t& a,
													   const node_t& b){
			return 1;
		}
	};
```c++

