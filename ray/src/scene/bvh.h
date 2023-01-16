#ifndef BVH_H__
#define BVH_H__

#include <fstream>

#include "ray.h"
#include "bbox.h"

class Geometry;
class TrimeshFace;

class BVH {
    BVH *left;
    BVH *right;
    bool isLeafNode;
    BoundingBox thisBB;
    Geometry *leafGeom;
    TrimeshFace *leafFace;
    int counter;

public:
    BVH(std::vector<Geometry*> &objList);
    BVH(std::vector<TrimeshFace*> &objList);

    bool intersect(ray &r, isect &i);

    bool isLeaf() const { return isLeafNode; }
    const BoundingBox& getBoundingBox() const { return thisBB; }
};

#endif // BVH_H__
