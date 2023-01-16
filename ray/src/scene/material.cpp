#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI* traceUI;

#include <glm/gtx/io.hpp>
#include <iostream>
#include "../fileio/images.h"

using namespace std;
extern bool debugMode;

Material::~Material()
{
}

glm::dvec3 Material::transPow(double d, const isect &i) const {
	if (!Trans()) return glm::dvec3(0, 0, 0);
	return glm::dvec3(pow(kt(i)[0], d), 
					  pow(kt(i)[1], d), 
					  pow(kt(i)[2], d));
}

glm::dvec3 Material::shade(Scene* scene, const ray& r, const isect& i) const {
  // material of object
  const Material &m = i.getMaterial();
  glm::dvec3 norm = i.getN();
  if (normals(i) != glm::dvec3( -9999.0, -9999.0, -9999.0 )) {
    norm *= glm::normalize(-normals(i) * 2.0 - 1.0);
  }
  
  // exiting the object (i.e. backface)
  bool exit = glm::dot(r.getDirection(), norm) > RAY_EPSILON;
  // normal facing opposite of camera ray
  glm::dvec3 normDir = exit ? -norm : norm;
  // point shifted for floating point error
  glm::dvec3 shiftPoint = r.at(i.getT() - RAY_EPSILON);
  
  // specular coefficient
  double specHardness = m.shininess(i);
  // initial ambient and emissive color
  glm::dvec3 color = ka(i) * scene->ambient() + ke(i);
  // add contributions of every light
  for (const auto &pLight : scene->getAllLights()) {
    glm::dvec3 pointAdjust = shiftPoint;
    glm::dvec3 lightDir = pLight->getDirection(pointAdjust);
    glm::dvec3 lightNorm = glm::dot(lightDir, norm) > RAY_EPSILON ? norm : -norm;
    glm::dvec3 lightDirRefl = 2 * glm::dot(lightDir, lightNorm) * lightNorm - lightDir;
    if (!m.Trans()) lightNorm = normDir;
    
    if (m.Trans() && lightNorm != normDir) {
      pointAdjust += 2 * RAY_EPSILON * r.getDirection();
      lightDir = pLight->getDirection(pointAdjust);
    }
    // diffuse and specular terms
    glm::dvec3 difTerm = max(glm::dot(lightDir, lightNorm), 0.0) * kd(i);
    glm::dvec3 specTerm = ks(i) *
    pow(max(max(-glm::dot(lightDirRefl, r.getDirection()), glm::dot(lightDir, r.getDirection())), 0.0), specHardness);
    
    ray shadowRay(pointAdjust, lightDir, glm::dvec3(1, 1, 1), ray::SHADOW);
    // factor in attenuations and light color
    color += (difTerm + specTerm)
            * pLight->getColor()
            * pLight->distanceAttenuation(pointAdjust)
            * pLight->shadowAttenuation(shadowRay, pointAdjust);
  }
  
  return color;
}

TextureMap::TextureMap(string filename)
{
	// FILLS THIS.DATA WITH TEXTURE MAPPING AND WIDTH/HEIGHT FILLED BY REF
	// FLATTENED ROW-MAJOR (R, G, B)
	data = readImage(filename.c_str(), width, height);
	if (data.empty()) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2& coord) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.
	// What this function should do is convert from
	// parametric space which is the unit square
	// [0, 1] x [0, 1] in 2-space to bitmap coordinates,
	// and use these to perform bilinear interpolation
	// of the values.
  
  const int x = (int) (coord[0] * width);
  const int y = (int) (coord[1] * height);
	const glm::dvec3 ll = getPixelAt(x, y);
	const glm::dvec3 ul = getPixelAt(x, y - 1);
	const glm::dvec3 lr = getPixelAt(x - 1, y);
	const glm::dvec3 ur = getPixelAt(x - 1, y - 1);
	const glm::dvec3 bp = ll * (1 - coord[0]) + lr * (coord[0]);  // bottom interp point
	const glm::dvec3 tp = ul * (1 - coord[0]) + ur * (coord[0]);  // top interp point
	const glm::dvec3 fp = bp * (1 - coord[1]) + tp * (coord[1]);  // final interp point
	return fp;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function
  
  const int offset = (x + y * width) * 3;
  const bool xInBounds = x >= 0 || x < width;
  const bool yInBounds = y >= 0 || y < height;
  const bool isInBounds = xInBounds && yInBounds && (offset < data.size());
  if (!isInBounds) {
    return glm::dvec3(0, 0, 0);
  }
  return glm::dvec3(data[offset], data[offset + 1], data[offset + 2]) / 255.0;
}

glm::dvec3 MaterialParameter::value(const isect& is) const
{
  if (_textureMap != nullptr) {
		return _textureMap->getMappedValue(is.getUVCoordinates());
  }
  return _value;
}

double MaterialParameter::intensityValue(const isect& is) const
{
	if (_textureMap != nullptr) {
		glm::dvec3 value(_textureMap->getMappedValue(is.getUVCoordinates()));
		return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
	}
  return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}
