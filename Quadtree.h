#pragma once
#include <list>
#include <type_traits>


// quadtree to manage collisions
// totally overkill for the game this will be on
// but I want to do it anyway

// at max 5 * 4**10 elements
constexpr int MAX_QUAD_DEPTH = 10;
constexpr int MAX_ELEMENTS_QUAD = 4;


// the way the quad will insert it's elements
enum class QuadInsertMode 
{
	POINT,			// a [x, y] point in space
	REGION_CENTER,  // a region in space defined by a [x, y] point that it's the center of the region and its width and height
	REGION_CORNER	// a region in space defined by a [x, y] point that it's the top left corner of the region and its width and height 
};


//! This may be changed to just a clas you inherit from in the future idk, just wanted to try c++ concepts :')
// this made me have all in the header file ;-;
// templates do be dumb
//! IMPORTANT: for an element to be inserted in the quadtree it has to have an x,y position and a size with width and height
//! if you are using points just set the w and h to 0 and use QuadInsertMode::POINT
template <typename T>
concept QuadInsertable = requires(T t) 
{
	{t.x} -> std::same_as<int&>;
	{t.y} -> std::same_as<int&>;
	{t.w} -> std::same_as<int&>;
	{t.h} -> std::same_as<int&>;
};

// contains the stored information of a quad
// might add more things idk
template <QuadInsertable Q>
class Node
{
public:
	// in case I add more data structures inside the node, who knows
	uint32_t Length()    { return elements.size(); }
	void add(Q* element) { elements.push_back(element); }
	void clear()         { elements.clear(); }

	std::list<Q*> elements;

};

// quad class that will manage the operations in the quadtree
template <QuadInsertable Q>
class Quad
{
public:
	Quad(int x, int y, int w, int h, QuadInsertMode mode, int depth=0)
		: m_x(x), m_y(y), m_w(w), m_h(h), m_insertMode(mode), m_depth(depth)
	{
		m_node = new Node<Q>();
	}

	bool Insert(Q* element)
	{
		// the element doesn't correspond to this quad
		if (!rectCollision(m_x, m_y, m_w, m_h, element->x, element->y, element->w, element->h)) return false;


		// check if the element fits in this quad
		// if the quad ahs been divided we can't insert even if we don't have 
		// any elements in the quad
		if (!m_divided && m_node->Length() < MAX_ELEMENTS_QUAD) {
			m_node->add(element);
			return true;
		}
		// if it does not fit subdivide if possible
		// if already subdivided then we can continue    
		else if (m_divided || m_depth < MAX_QUAD_DEPTH) {
			// if not divided divide
			if (!m_divided) {
				divide();
				// move all elements to children nodes
				for (const auto& e : m_node->elements) {
					m_TopLeft->Insert(e);
					m_TopRight->Insert(e);
					m_BotLeft->Insert(e);
					m_BotRight->Insert(e);
				}
				// clear the node after dividing
				m_node->elements.clear();
			}
			// add element to children
			bool tl = m_TopLeft->Insert(element);
			bool tr = m_TopRight->Insert(element);
			bool bl = m_BotLeft->Insert(element);
			bool br = m_BotRight->Insert(element);
			// if inserted on any quad return true
			return tl | tr | bl | br;
		}
		// we don't have space and we reach the max depth so we can
		// not divide anymore
		return false;
	}

	std::list<Q*> Query(int rx, int ry, int rw, int rh)
	{
		return queryImpl(rx, ry, rw, rh );
	}

	~Quad()
	{
		delete m_node;
		if (m_divided)
		{
			delete m_TopLeft;
			delete m_TopRight;
			delete m_BotLeft;
			delete m_BotRight;
		}
	}

private:

	std::list<Q*> queryImpl(int rx, int ry, int rw, int rh)
	{
		// check if quad is to the left of the rect or inverse
		std::list<Q*> res;

		if (rx > (m_x + m_w) || m_x > (rx + rw)) return res;
		// check if quad is top to the rect or inverse
		if (ry > (m_y + m_h) || m_y > (ry + rh)) return res;

		// rect and quad are touching and quad is divided
		if (m_divided) {
			auto tl = m_TopLeft->queryImpl(rx, ry, rw, rh);
			auto tr = m_TopRight->queryImpl(rx, ry, rw, rh);
			auto bl = m_BotLeft->queryImpl(rx, ry, rw, rh);
			auto br = m_BotRight->queryImpl(rx, ry, rw, rh);

			res.splice(res.end(), tl);
			res.splice(res.end(), tr);
			res.splice(res.end(), bl);
			res.splice(res.end(), br);

			return res;
		}
		for (const auto& element : m_node->elements) {
			if (rectCollision(rx, ry, rw, rh, element->x, element->y, element->w, element->h )) {
				bool inList = std::find(res.begin(), res.end(), element) != res.end();
				if (!inList) res.push_back(element);
			}
		}

		return res;
	}

	void divide()
	{
		// precalculate size
		int width  = m_w / 2;
		int height = m_h / 2;

		m_TopLeft  = new Quad(m_x,         m_y,          width, height, m_insertMode, m_depth + 1);
		m_TopRight = new Quad(m_x + width, m_y,          width, height, m_insertMode, m_depth + 1);
		m_BotLeft  = new Quad(m_x,         m_y + height, width, height, m_insertMode, m_depth + 1);
		m_BotRight = new Quad(m_x + width, m_y + height, width, height, m_insertMode, m_depth + 1);


		m_divided = true;
	}

	bool rectCollision(int r1x, int r1y, int r1w, int r1h,  
		               int r2x, int r2y, int r2w, int r2h)
	{
		// corners of the quad
		int qxl = r1x;
		int qyl = r1y;
		int qxr = r1x + r1w;
		int qyr = r1y + r1h;
		// corners of the rect we are inserting
		int rxl, ryl, rxr, ryr;
		int whalf;
		int hhalf;

		switch (m_insertMode) {
		case QuadInsertMode::POINT:
			rxl = r2x;
			ryl = r2y;
			rxr = rxl;
			ryr = ryl;
			break;
		case QuadInsertMode::REGION_CENTER:
			// TODO: FIX THIS, the rounding error is making this imprecise close to the edges
			whalf = r2w / 2;
			hhalf = r2h / 2;
			rxl = r2x - whalf;
			ryl = r2y - hhalf;
			rxr = r2x + whalf;
			ryr = r2y + hhalf;
			break;
		case QuadInsertMode::REGION_CORNER:
			rxl = r2x;
			ryl = r2y;
			rxr = r2x + r2w;
			ryr = r2y + r2h;
			break;
		default:
			return false;
		}

		// rect is to the left of the other
		if (qxl > rxr || rxl > qxr) return false;
		// rect is on top of the other
		if (qyl > ryr || ryl > qyr) return false;


		return true;
	}

	// to be implemented in other place in order to draw
	friend void DrawQT(Quad* qt);
	// private memebers
private:

	// bounding box
	int m_x;
	int m_y;
	int m_w;
	int m_h;

	QuadInsertMode m_insertMode;

	bool m_divided = false;

	uint32_t m_depth;

	Node<Q>* m_node = nullptr;
	// child quads
	Quad* m_TopLeft  = nullptr;
	Quad* m_TopRight = nullptr;
	Quad* m_BotLeft  = nullptr;
	Quad* m_BotRight = nullptr;
};

// type alias for clarity
template <QuadInsertable Q>
using QuadTree = Quad<Q>;

