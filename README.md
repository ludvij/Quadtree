# Quadtree

A quadrtree made in c++20 to use in c++ projects

Uses c++ concepts to define what can be inserted in the quadtree, might be changed to inheritance in the future.
Right now for an object to be inserted of the quadtree it has to have a itn x, itn y, int w, int h to define its bounds


# Features

Has 3 different modes of inserting and storing information: 
- POINT, can be used for particles
- REGION_CENTER, for rects, the [x, y] pair is the center of the region
- REGION_CORNER, for rectos, the [x, y] pair is the top left corner of the region

If you use any of the region modes a element can be in various quads simulataneously

The tree can be queried with the Quad::Query() method, it will return all the unfirmation that is inside the rect passed, then is up to you to
use tht information for collisions


