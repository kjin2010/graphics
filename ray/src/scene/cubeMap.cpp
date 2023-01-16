#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
#include "../scene/material.h"
extern TraceUI* traceUI;

glm::dvec3 CubeMap::getColor(ray r) const
{
	// YOUR CODE HERE
	// FIXME: Implement Cube Map here

	// given a cube of textures and a ray pointing from the center, determine
	// the color that it intersects with 
	//
	// find which plane it intersects with then find that intersection point and call
	// trimesh.intersect
  
  const glm::dvec3 dir = r.getDirection();
  const double x = dir[0], y = dir[1], z = dir[2];
  const double absx = fabs(x), absy = fabs(y), absz = fabs(z);
  const double maxAxis = fmax(absx, fmax(absy, absz));
  const bool isXP = x > 0, isYP = y > 0, isZP = z > 0;
  
  int boxI;
  double u, v;
  if (maxAxis == absx) {
    // left - right
    boxI = isXP ? 0 : 1;
    u = isXP ? z : -z;
    v = y;
  } else if (maxAxis == absy) {
    // top - down
    boxI = isYP ? 2 : 3;
    u = x;
    v = isYP ? z : -z;
  } else if (maxAxis == absz) {
    // front - back
    boxI = isZP ? 5 : 4;
    u = isZP ? -x : x;
    v = y;
  }
  u = ((u / maxAxis) + 1) / 2;
  v = ((v / maxAxis) + 1) / 2;
  return tMap[boxI]->getMappedValue(glm::dvec2(u, v));
}

CubeMap::CubeMap()
{
}

CubeMap::~CubeMap()
{
}

void CubeMap::setNthMap(int n, TextureMap* m)
{
	if (m != tMap[n].get())
		tMap[n].reset(m);
}
