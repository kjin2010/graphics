#include <iostream>
#include <fstream>

#include <glm/gtx/io.hpp>

#include "bvh.h"
#include "scene.h"
#include "bbox.h"

#include "../SceneObjects/trimesh.h"

struct xSort {
    inline bool operator() (const Geometry *objA, const Geometry *objB) { 
        BoundingBox aBox = objA->getBoundingBox();
        BoundingBox bBox = objB->getBoundingBox();
        glm::dvec3 aBoxMid = aBox.getMax() + aBox.getMin();
        glm::dvec3 bBoxMid = bBox.getMax() + bBox.getMin();
        return aBoxMid[0] > bBoxMid[0];
    }
};

struct ySort {
    inline bool operator() (const Geometry *objA, const Geometry *objB) { 
        BoundingBox aBox = objA->getBoundingBox();
        BoundingBox bBox = objB->getBoundingBox();
        glm::dvec3 aBoxMid = aBox.getMax() + aBox.getMin();
        glm::dvec3 bBoxMid = bBox.getMax() + bBox.getMin();
        return aBoxMid[1] > bBoxMid[1];
    }
};

struct zSort {
    inline bool operator() (const Geometry *objA, const Geometry *objB) { 
        BoundingBox aBox = objA->getBoundingBox();
        BoundingBox bBox = objB->getBoundingBox();
        glm::dvec3 aBoxMid = aBox.getMax() + aBox.getMin();
        glm::dvec3 bBoxMid = bBox.getMax() + bBox.getMin();
        return aBoxMid[2] > bBoxMid[2];
    }
};

BVH::BVH(std::vector<Geometry*> &geomList)
        : left(nullptr), right(nullptr), isLeafNode(false) {
    leafFace = nullptr;
    BoundingBox currentBound(glm::dvec3(INT_MAX, INT_MAX, INT_MAX),
                             glm::dvec3(INT_MIN, INT_MIN, INT_MIN));
    for (auto geom : geomList) {
        BoundingBox curGeomBB = geom->getBoundingBox();
        currentBound.setMax(glm::max(currentBound.getMax(), curGeomBB.getMax()));
        currentBound.setMin(glm::min(currentBound.getMin(), curGeomBB.getMin()));
    }
    size_t n = geomList.size();
    if (n == 1) {
        isLeafNode = true;
        leafGeom = geomList[0];
    }
    else {
        double xLen = currentBound.getMax()[0] - currentBound.getMin()[0];
        double yLen = currentBound.getMax()[1] - currentBound.getMin()[1];
        double zLen = currentBound.getMax()[2] - currentBound.getMin()[2];

        std::vector<Geometry*> leftObjList(n / 2);
        std::vector<Geometry*> rightObjList(n - (n / 2));

        // x axis longest
        if (xLen >= yLen && xLen >= zLen) {
            std::sort(geomList.begin(), geomList.end(), xSort());
        }
        // y axis longest
        else if (yLen >= zLen) {
            std::sort(geomList.begin(), geomList.end(), ySort());
        }
        // z axis longest
        else {
            std::sort(geomList.begin(), geomList.end(), zSort());
        }
        for (int i = 0; i < n / 2; i++) leftObjList[i] = geomList[i];
        for (int i = n / 2; i < n; i++) rightObjList[i - (n / 2)] = geomList[i];

        left = new BVH(leftObjList);
        right = new BVH(rightObjList);
    }
    thisBB = currentBound;
}

BVH::BVH(std::vector<TrimeshFace*> &geomList)
        : left(nullptr), right(nullptr), isLeafNode(false) {
    BoundingBox currentBound(glm::dvec3(INT_MAX, INT_MAX, INT_MAX),
                             glm::dvec3(INT_MIN, INT_MIN, INT_MIN));
    counter = 0;
    for (auto geom : geomList) {
        const BoundingBox curGeomBB = geom->getBoundingBox();
        currentBound.setMax(glm::max(currentBound.getMax(), curGeomBB.getMax()));
        currentBound.setMin(glm::min(currentBound.getMin(), curGeomBB.getMin()));
    }

    size_t n = geomList.size();
    if (n == 1) {
        isLeafNode = true;
        leafFace = geomList[0];
    }
    else {
        double xLen = currentBound.getMax()[0] - currentBound.getMin()[0];
        double yLen = currentBound.getMax()[1] - currentBound.getMin()[1];
        double zLen = currentBound.getMax()[2] - currentBound.getMin()[2];

        std::vector<TrimeshFace*> leftObjList(n / 2);
        std::vector<TrimeshFace*> rightObjList(n - (n / 2));

        // x axis longest
        if (xLen >= yLen && xLen >= zLen) {
            std::sort(geomList.begin(), geomList.end(), xSort());
        }
        // y axis longest
        else if (yLen >= zLen) {
            std::sort(geomList.begin(), geomList.end(), ySort());
        }
        // z axis longest
        else {
            std::sort(geomList.begin(), geomList.end(), zSort());
        }
        for (int i = 0; i < n / 2; i++) leftObjList[i] = geomList[i];
        for (int i = n / 2; i < n; i++) rightObjList[i - (n / 2)] = geomList[i];

        left = new BVH(leftObjList);
        right = new BVH(rightObjList);
    }
    thisBB = currentBound;
}

bool BVH::intersect(ray &r, isect &i) {
    // assumes current bounding box is hit 
    if (isLeaf()) {
        if (leafFace) {
            return leafFace->intersect(r, i);
        }
        return leafGeom->intersect(r, i);
    }
    double leftTMin, leftTMax;
    if (left->getBoundingBox().intersect(r, leftTMin, leftTMax)) {
        double rightTMin, rightTMax;
        BVH *first = left, *last = right;
        double timeThresh;
        bool rightHit = right->getBoundingBox().intersect(r, rightTMin, rightTMax);
        if (rightHit && rightTMin < leftTMin) {
            first = right;
            last = left;
        }
        timeThresh = (first == right) ? leftTMin : rightTMin;
        bool firstHit = first->intersect(r, i);
        if (firstHit) {
            if (i.getT() < timeThresh) {
                return true;
            }
            else {
                isect lastI;
                if (last->intersect(r, lastI) && lastI.getT() < i.getT()) {
                    i = lastI;
                }
                return true;
            }
        }
        else {
            return last->intersect(r, i);
        }
    }
    else {
        return right->intersect(r, i);
    }
}
