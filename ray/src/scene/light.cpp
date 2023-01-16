#include <cmath>
#include <iostream>

#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

extern TraceUI* traceUI;

using namespace std;

double DirectionalLight::distanceAttenuation(const glm::dvec3& P) const
{
	// distance to light is infinite, so f(di) goes to 0.  Return 1.
	return 1.0;
}

glm::dvec3 DirectionalLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const {
    isect shadowIsect;
    ray shadowRay(r);

	// if no initial hit, no shadow attenuation
    if (!scene->intersect(shadowRay, shadowIsect) || shadowIsect.getT() < RAY_EPSILON) {
		return glm::dvec3(1, 1, 1);
    }
	// loop through all intersections in direction of light from intersect point
    while (scene->intersect(shadowRay, shadowIsect) &&
           glm::length(shadowRay.getAtten()) > traceUI->getThreshold()) {
		// if translucent object, factor for shadow attenuation
        if (shadowIsect.getMaterial().Trans()) {
            glm::dvec3 shadowHitPoint = shadowRay.at(shadowIsect.getT() + RAY_EPSILON);
            const Material &m = shadowIsect.getMaterial();
            shadowRay.setPosition(shadowHitPoint);
			// if exiting object, attenuate with kt^d 
            if (glm::dot(r.getDirection(), shadowIsect.getN()) > RAY_EPSILON) {
				double d = shadowIsect.getT();
                glm::dvec3 multFactor = m.transPow(d, shadowIsect);
                shadowRay.scaleAtten(multFactor);
            }
        }
        else {
			// if non-transparent object, return 0 for full shadow
            return glm::dvec3(0, 0, 0);
        }
    }
    return shadowRay.getAtten();
}

glm::dvec3 DirectionalLight::getColor() const
{
	return color;
}

glm::dvec3 DirectionalLight::getDirection(const glm::dvec3& P) const
{
	return -orientation;
}

double DirectionalLight::getDistance(const glm::dvec3& P) const 
{
	return 1001;
}

double PointLight::distanceAttenuation(const glm::dvec3& P) const
{

	// YOUR CODE HERE

	// You'll need to modify this method to attenuate the intensity 
	// of the light based on the distance between the source and the 
	// point P.  For now, we assume no attenuation and just return 1.0

	double d = glm::length(P - position);
	double den = constantTerm + linearTerm * d + quadraticTerm * d * d;
	return min(1 / den, 1.0);
}

glm::dvec3 PointLight::getColor() const
{
	return color;
}

glm::dvec3 PointLight::getDirection(const glm::dvec3& P) const
{
	return glm::normalize(position - P);
}

glm::dvec3 PointLight::shadowAttenuation(const ray& r, const glm::dvec3& p) const {
    isect shadowIsect;
    ray shadowRay(r);
    double dist = getDistance(p);

	// if no initial hit, no shadow attenuation
    if (!scene->intersect(shadowRay, shadowIsect) || shadowIsect.getT() > dist || shadowIsect.getT() < RAY_EPSILON) {
		return glm::dvec3(1, 1, 1);
    }
	// loop through all intersections between intersect point and point light
    while (scene->intersect(shadowRay, shadowIsect) &&
            (dist -= shadowIsect.getT()) > 0 &&
            glm::length(shadowRay.getAtten()) > traceUI->getThreshold()) {
		// if translucent object, factor for shadow attenuation
        if (shadowIsect.getMaterial().Trans()) {
            glm::dvec3 shadowHitPoint = shadowRay.at(shadowIsect.getT() + RAY_EPSILON);
            const Material &m = shadowIsect.getMaterial();
            shadowRay.setPosition(shadowHitPoint);
			// if exiting object, attenuate with kt^d 
            if (glm::dot(r.getDirection(), shadowIsect.getN()) > RAY_EPSILON) {
                double d = shadowIsect.getT();
                glm::dvec3 multFactor = m.transPow(d, shadowIsect);
                shadowRay.scaleAtten(multFactor);
            }
        }
        else {
			// if non-transparent object, return 0 for full shadow
            return glm::dvec3(0, 0, 0);
        }
    }
    return shadowRay.getAtten();
}

double PointLight::getDistance(const glm::dvec3& P) const 
{
	return glm::length(position - P);
}

#define VERBOSE 0

