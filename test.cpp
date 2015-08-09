//qbr clang++ -std=c++11 test.cpp
//qbr ./a.out --force-colour -b

#include <iostream>

#include "pathfinding.hpp"

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <vector>

using namespace pf;

TEST_CASE( "SimpleGraph", "[graph]" ) {

	//There are two requirements for a 'node' class
	//1.Your adaptor class must be able to find a node's adjacent nodes
	//	using only it's members.  For example, SimpleNode below stores pointers
	//	to it's adjacent nodes in a vector.
	//2.The class must implement the '==' operator.  I've done so below by
	//	comparing the address of both objects, which should be fine if the nodes
	//	can be expected to be unique and never moved.  You can define your
	//	node's equality however you want: comparing positions, some kind of id
	//	code, etc. 
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

	SECTION( "chain" ) {
		//
		//  a--b--c
		//	
		SimpleNode a('a');
		SimpleNode b('b');
		SimpleNode c('c');

		a.adjacentNodes.push_back( &b );
		b.adjacentNodes.push_back( &c );

		Search<SimpleAdaptor> search( a, c );
		auto results = search.path();
		REQUIRE( results.size() == 3 );
		REQUIRE( results[0]->name == 'a' );
		REQUIRE( results[1]->name == 'b' );
		REQUIRE( results[2]->name == 'c' );
	}
	
	SECTION( "3-node path vs. 2-node path" ) {
		//	  __u__	
		//	 /	   \
		//	s		e
		//	 \d1_d2/
		//
		SimpleNode s('s');
		SimpleNode u('u');
		SimpleNode d1('d');
		SimpleNode d2('D');
		SimpleNode e('e');
		
		s.adjacentNodes.push_back( &u );
		s.adjacentNodes.push_back( &d1 );
	
		d1.adjacentNodes.push_back( &d2 );
		d2.adjacentNodes.push_back( &e );
		
		u.adjacentNodes.push_back( &e );
		
		Search<SimpleAdaptor> search( s, e );
		auto results = search.path();
		REQUIRE( results.size() == 3 );
		REQUIRE( results[0]->name == 's' );
		REQUIRE( results[1]->name == 'u' );
		REQUIRE( results[2]->name == 'e' );
	}

}

TEST_CASE( "Generating node's on the fly", "[graph]" ) {

	using namespace std;

	//std::pair is a perfectly fine node
	//it defines '=='.
	typedef std::pair<int,int> MyNode;

	class MyAdaptor : Adaptor<MyNode>
	{
	public:
		using Adaptor::node_t;
	 	const bool * grid;
		
		vector<unique_ptr<node_t> >garbageCollecter;
		MyAdaptor( const bool * grid ) : grid(grid){
		}
		
		bool validNode( int i, int j ){
			if( ( i >= 5 )||( i < 0 ) )
				return false;
			if( ( j >= 5 )||( j < 0 ) )
				return false;
			return grid[i+j*5];
		}
			
		node_t * makeNode( int i, int j ){
			return new pair<int,int>(i,j);
			garbageCollecter.push_back(
				unique_ptr<std::pair<int, int>>(
					new pair<int, int>(i,j)
					)
				);
			return garbageCollecter[ garbageCollecter.size() - 1].get();
		}

		std::vector<node_t*> getAdjacentNodes(const node_t& node){
			std::cout << "getAdjacentNodes("<<node.first<<','<<node.second<<")\n";
			std::vector<std::pair<int,int>*> returnVector;
			int i = node.first;
			int j = node.second;
			//north
			if( validNode( i-1, j ) )
				returnVector.push_back( makeNode( i-1, j ) );
			if( validNode( i+1, j ) )
				returnVector.push_back( makeNode( i+1, j ) );
			if( validNode( i, j-1 ) )
				returnVector.push_back( makeNode( i, j-1 ) );
			if( validNode( i, j ) )
				returnVector.push_back( makeNode( i-1, j ) );
	
			return returnVector;
		}

		unsigned heuristicDistanceBetweenAdjacentNodes(const node_t& a,
													   const node_t& b){
			return abs(a.first - b.first + a.second - b.second);
		}
	};

	SECTION( "open grid" ) {
		const bool grid[25] =
				{	0,	0,	0,	0,	0,
					0,	0,	0,	0,	0,
					0,	0,	0,	0,	0,
					0,	0,	0,	0,	0,
					0,	0,	0,	0,	0 };
		//Search<MyAdaptor, const bool *> search(
		//	std::pair<int,int>(0,0), std::pair<int,int>(4,4), grid );
		//auto path = search.path();
		//REQUIRE( path.size() == 9 );
	}

}
