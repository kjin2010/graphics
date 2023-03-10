// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"
#include "scene/bvh.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <iostream>
#include <fstream>
#include <random>

using namespace std;
extern TraceUI* traceUI;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

glm::dvec3 RayTracer::trace(double x, double y)
{
	// Clear out the ray cache in the scene for debugging purposes,
	if (TraceUI::m_debug)
	{
		scene->clearIntersectCache();	
	}

	ray r(glm::dvec3(0,0,0), glm::dvec3(0,0,0), glm::dvec3(1,1,1), ray::VISIBILITY);
	scene->getCamera().rayThrough(x,y,r);
	double dummy;
	glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), traceUI->getDepth(), dummy);
	ret = glm::clamp(ret, 0.0, 1.0);
	return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j)
{
	glm::dvec3 col(0,0,0);

	if( ! sceneLoaded() ) return col;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
	col = trace(x, y);

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
	return col;
}

#define VERBOSE 0

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, const glm::dvec3& thresh, int depth, double& t )
{
	// if recursion depth exceeded
	if (depth < 0) {
		return glm::dvec3(0, 0, 0);
	}

	isect i;
	glm::dvec3 colorC;
#if VERBOSE
	std::cerr << "== current depth: " << depth << std::endl;
#endif

	if(scene->intersect(r, i)) {
		// YOUR CODE HERE

		// An intersection occurred!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.

		// material of intersection
		const Material &m = i.getMaterial();
		// direct phong color
		colorC = m.shade(scene.get(), r, i);
		// point camera ray hits surface
		glm::dvec3 hitPoint = r.at(i.getT());

		// if exiting object
		bool exit = glm::dot(i.getN(), r.getDirection()) > RAY_EPSILON;
		// normal facing opposite of camera ray
		glm::dvec3 normDir = exit ? -i.getN() : i.getN();

		// REFLECTIONS
		if (m.Refl()) {
			// shifted intersection point for floating point errors
			glm::dvec3 shiftPoint = hitPoint - RAY_EPSILON * r.getDirection();
			// direction of reflected camera ray will be direction of 'new camera ray'
			glm::dvec3 reflDir = glm::normalize(r.getDirection() - 
				2 * glm::dot(normDir, r.getDirection()) * normDir);
			// reflect ray
			ray reflRay(shiftPoint, reflDir, glm::dvec3(1.0, 1.0, 1.0), ray::REFLECTION, r.getInd());
			double reflT;
			glm::dvec3 reflColor = traceRay(reflRay, thresh, depth - 1, reflT);
			colorC += reflColor * m.kr(i);
		}

		// REFRACTIONS
		if (m.Trans()) {
			double newInd = m.index(i);
			bool overlapped = false;
			isect overlapIsect;
			if (exit) {
				// the index of refraction of refracted ray 
				glm::dvec3 overlapShiftPoint = hitPoint + RAY_EPSILON * r.getDirection();
				ray overlapRay(overlapShiftPoint, r.getDirection(), glm::dvec3(1, 1, 1), ray::VISIBILITY);
				if (scene->intersect(overlapRay, overlapIsect) &&
					(glm::dot(overlapRay.getDirection(), overlapIsect.getN()) > RAY_EPSILON) &&
					overlapIsect.getMaterial().Trans()) {
						overlapped = true;
						newInd = overlapIsect.getMaterial().index(overlapIsect);
				}
				else {
					newInd = 1.0001;
				}
			}
			// incoming index / outgoing index
			double mu = r.getInd() / newInd;
			// for total internal reflection
			double tir = glm::length(glm::cross(normDir, -r.getDirection())) * mu;
			// TOTAL INTERNAL REFLECTION
			if (1 < tir) {
				// double reflections for tir in reflective objects ? design decision
				// if (!m.Refl()) {
					// shifted intersection point
					glm::dvec3 shiftPoint = hitPoint - RAY_EPSILON * r.getDirection();
					glm::dvec3 reflDir = glm::normalize(r.getDirection() - 
						2 * glm::dot(normDir, r.getDirection()) * normDir);
					// total internal reflection ray
					ray intReflRay(shiftPoint, reflDir, glm::dvec3(1.0, 1.0, 1.0), ray::REFRACTION, r.getInd());
					double intReflT;
					glm::dvec3 intReflColor = traceRay(intReflRay, thresh, depth - 1, intReflT);
					glm::dvec3 factor = m.transPow(intReflT, i);
					colorC += intReflColor * m.kr(i) * factor;
				// }
			}
			// PASS THROUGH REFRACTION
			else {
				// shifted point to inside of object
				glm::dvec3 shiftPoint = hitPoint + RAY_EPSILON * r.getDirection();
				double cosThetaIn = glm::dot(normDir, -r.getDirection());
				double cosThetaOut = sqrt(1 - pow(mu, 2) * (1 - pow(cosThetaIn, 2)));
				// direction of ray after refraction
				glm::dvec3 refrDir = glm::normalize((mu * cosThetaIn - cosThetaOut) * normDir + 
					mu * r.getDirection());
				ray refrRay(shiftPoint, refrDir, glm::dvec3(1.0, 1.0, 1.0), ray::REFRACTION, newInd);
				double refrT;
				glm::dvec3 refrColor = traceRay(refrRay, thresh, depth - 1, refrT);
				glm::dvec3 factor = exit ? glm::dvec3(1.0, 1.0, 1.0) : 
											m.transPow(refrT, i);
				if (overlapped) {
					factor = overlapIsect.getMaterial().transPow(refrT, overlapIsect);
				}
				colorC += refrColor * factor;	
			}
		}

		// WHAT TO DO WITH THRESH ???????
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		//
		// FIXME: Add CubeMap support here.
		// TIPS: CubeMap object can be fetched from traceUI->getCubeMap();
		//       Check traceUI->cubeMap() to see if cubeMap is loaded
		//       and enabled.
    
    const glm::dvec3 black = glm::dvec3(0, 0, 0);
    colorC = traceUI->cubeMap() ? traceUI->getCubeMap()->getColor(r) : black;
	}
#if VERBOSE
	std::cerr << "== depth: " << depth+1 << " done, returning: " << colorC << std::endl;
#endif
	// set current time (actual time if hit, 1000 if infinite)
	t = i.getT();
	return colorC;
}

RayTracer::RayTracer()
	: scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0), m_bBufferReady(false)
{
}

RayTracer::~RayTracer()
{
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer.data();
	w = buffer_width;
	h = buffer_height;
}

void RayTracer::getAdaptiveBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = adaptive_buffer.data();
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char* fn)
{
	ifstream ifs(fn);
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}

	// Strip off filename, leaving only the path:
	string path( fn );
	if (path.find_last_of( "\\/" ) == string::npos)
		path = ".";
	else
		path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
	Parser parser( tokenizer, path );
	try {
		scene.reset(parser.parseScene());
	}
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	} catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	} catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if (!sceneLoaded())
		return false;

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	size_t newBufferSize = w * h * 3;
	if (newBufferSize != buffer.size()) {
		bufferSize = newBufferSize;
		buffer.resize(bufferSize);
		adaptive_buffer.resize(bufferSize);
	}
	buffer_width = w;
	buffer_height = h;
	std::fill(buffer.begin(), buffer.end(), 0);
	m_bBufferReady = true;

	/*
	 * Sync with TraceUI
	 */

	threads = traceUI->getThreads();
	block_size = traceUI->getBlockSize();
	thresh = traceUI->getThreshold();
	samples = traceUI->getSuperSamples();
	aaThresh = traceUI->getAaThreshold();

	std::vector<Geometry*> sceneObjs;
	for (auto objIter = scene->beginObjects(); objIter != scene->endObjects(); objIter++) {
		sceneObjs.push_back((*objIter).get());
	}
	BVH *temp = new BVH(sceneObjs);

	scene->setBVH(temp);

	// YOUR CODE HERE
	// FIXME: Additional initializations
	/*
	plan:
		queue of threads
		spawn n threads in queue
		for i in range(h):
			for j in range(w):
				get next thread
				color = tracepixel(i, j)
				buffer[f(i, j)] = color
				add to queue
	*/
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h)
{
	// Always call traceSetup before rendering anything.
	traceSetup(w,h);

	// YOUR CODE HERE
	// FIXME: Start one or more threads for ray tracing
	//
	// TIPS: Ideally, the traceImage should be executed asynchronously,
	//       i.e. returns IMMEDIATELY after working threads are launched.
	//
	//       An asynchronous traceImage lets the GUI update your results
	//       while rendering.

	/*
	Plan:
		spawn one thread that calls trace setup ?????
	*/

	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			tracePixel(x, y);
		}
	}
}

int RayTracer::adaptSample(double thresh, int depth, double minx, double miny, double maxx, double maxy, glm::dvec3 &color) {
	glm::dvec3 ll = trace(minx, miny);
	glm::dvec3 ul = trace(minx, maxy);
	glm::dvec3 lr = trace(maxx, miny);
	glm::dvec3 ur = trace(maxx, maxy);
	glm::dvec3 mid = trace((minx + maxx) / 2, (miny + maxy) / 2);
	glm::dvec3 avg = (ll + ul + lr + ur + mid) * .2;
	double offset = glm::length(ll - avg) + glm::length(ul - avg) + 
						glm::length(lr - avg) + glm::length(ur - avg) + 
						glm::length(mid - avg);
	offset = offset * .2;
	if (offset < thresh || depth == 0) {
		color = avg;
		return 5;
	}
	else {
		double midx = (minx + maxx) / 2;
		double midy = (miny + maxy) / 2;
		glm::dvec3 a, b, c, d;
		int counter = adaptSample(thresh, depth - 1, minx, miny, midx, midy, a);
		counter += adaptSample(thresh, depth - 1, minx, midy, midx, maxy, b);
		counter += adaptSample(thresh, depth - 1, midx, miny, maxx, midy, c);
		counter += adaptSample(thresh, depth - 1, midx, midy, maxx, maxy, d);
		color = (a + b + c + d) * .25;
		return counter + 5;
	}
}

int RayTracer::adaptiveImage(double thresh) {
	for (int i = 0; i < buffer_width; i++) {
		for (int j = 0; j < buffer_height; j++) {
			double minx = (double) i / buffer_width;
			double maxx = (double) (i + 1) / buffer_width;
			double miny = (double) j / buffer_height;
			double maxy = (double) (j + 1) / buffer_height;
			glm::dvec3 sampleCol;
			int count = adaptSample(thresh, 5, minx, miny, maxx, maxy, sampleCol);
			unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
			unsigned char *densityPixel = adaptive_buffer.data() + ( i + j * buffer_width ) * 3;
			pixel[0] = (int)( 255.0 * sampleCol[0]);
			pixel[1] = (int)( 255.0 * sampleCol[1]);
			pixel[2] = (int)( 255.0 * sampleCol[2]);
			densityPixel[1] = min(count, 255);
		}
	}
	return 0;
}

int RayTracer::aaImage()
{
	// YOUR CODE HERE
	// FIXME: Implement Anti-aliasing here
	//
	// TIP: samples and aaThresh have been synchronized with TraceUI by
	//      RayTracer::traceSetup() function

	// loop through actual screen
	for (int i = 0; i < buffer_width; i++) {
		for (int j = 0; j < buffer_height; j++) {
			// cumulative 'box sum' of filter
			glm::dvec3 sampleCol(0, 0, 0);

			// upsample based off of current pixel in actual screen
			for (int iShift = 0; iShift < samples; iShift++) {
				for (int jShift = 0; jShift < samples; jShift++) {
					// stochastic supersamples shifts
					double x_shift = ((double) rand()) / ((double) RAND_MAX);
					double y_shift = ((double) rand()) / ((double) RAND_MAX);
					double x = double(samples * i + iShift + x_shift) / double(samples * buffer_width);
					double y = double(samples * j + jShift + y_shift) / double(samples * buffer_height);
					// add to box sum
					sampleCol += trace(x, y);
				}
			}
			// average out box sum of sample x sample filter
			sampleCol *= 1 / (pow(samples, 2));
			// filling in actual buffer
			unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
			pixel[0] = (int)( 255.0 * sampleCol[0]);
			pixel[1] = (int)( 255.0 * sampleCol[1]);
			pixel[2] = (int)( 255.0 * sampleCol[2]);
		}
	}
	
	return pow(samples, 2) * buffer_width * buffer_height;
}

bool RayTracer::checkRender()
{
	// YOUR CODE HERE
	// FIXME: Return true if tracing is done.
	//        This is a helper routine for GUI.
	//
	// TIPS: Introduce an array to track the status of each worker thread.
	//       This array is maintained by the worker threads.
	return true;
}

void RayTracer::waitRender()
{
	// YOUR CODE HERE
	// FIXME: Wait until the rendering process is done.
	//        This function is essential if you are using an asynchronous
	//        traceImage implementation.
	//
	// TIPS: Join all worker threads here.
}


glm::dvec3 RayTracer::getPixel(int i, int j)
{
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
	return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color)
{
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * color[0]);
	pixel[1] = (int)( 255.0 * color[1]);
	pixel[2] = (int)( 255.0 * color[2]);
}

