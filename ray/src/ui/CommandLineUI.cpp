#include <stdarg.h>
#include <time.h>
#include <iostream>
#ifndef _MSC_VER
#include <unistd.h>
#else
extern char* optarg;
extern int optind, opterr, optopt;
extern int getopt(int argc, char** argv, const char* optstring);
#endif

#include <assert.h>

#include "../fileio/images.h"
#include "CommandLineUI.h"

#include "../RayTracer.h"

using namespace std;

// The command line UI simply parses out all the arguments off
// the command line and stores them locally.
CommandLineUI::CommandLineUI(int argc, char** argv) : TraceUI()
{
	int i;
	progName = argv[0];
	const char* jsonfile = nullptr;
	string cubemap_file;
	double adaptive_thresh = 0;
	while ((i = getopt(argc, argv, "tr:w:hj:c:a:")) != EOF) {
		switch (i) {
			case 'r':
				m_nDepth = atoi(optarg);
				break;
			case 'w':
				m_nSize = atoi(optarg);
				break;
			case 'j':
				jsonfile = optarg;
				break;
			case 'c':
				cubemap_file = optarg;
				break;
			case 'a':
				adaptive_thresh = stod(optarg);
				break;
			case 'h':
				usage();
				exit(1);
			default:
				// Oops; unknown argument
				std::cerr << "Invalid argument: '" << i << "'."
				          << std::endl;
				usage();
				exit(1);
		}
	}
	if (jsonfile) {
		loadFromJson(jsonfile);
	}
	if (!cubemap_file.empty()) {
		smartLoadCubemap(cubemap_file);
	}

	if (optind >= argc - 1) {
		std::cerr << "no input and/or output name." << std::endl;
		exit(1);
	}

	if (adaptive_thresh != 0) {
		if (adaptive_thresh < 0) {
			std::cerr << "cant have negative threshold for adaptive supersampling\n";
			exit(1);
		}
		m_adaptive = true;
		m_adaptive_thresh = adaptive_thresh;
	}

	rayName = argv[optind];
	imgName = argv[optind + 1];
	// adaptiveImgName = argv[optind + 1];
}

int CommandLineUI::run()
{
	assert(raytracer != 0);
	raytracer->loadScene(rayName);

	if (raytracer->sceneLoaded()) {
		int width = m_nSize;
		int height = (int)(width / raytracer->aspectRatio() + 0.5);

		raytracer->traceSetup(width, height);

		clock_t start, end;
		start = clock();

		raytracer->traceImage(width, height);
		raytracer->waitRender();
		if (m_adaptive) {
			raytracer->adaptiveImage(m_adaptive_thresh);
			raytracer->waitRender();
		}
		else if (aaSwitch()) {
			raytracer->aaImage();
			raytracer->waitRender();
		}

		end = clock();

		// save image
		unsigned char* buf;

		raytracer->getBuffer(buf, width, height);

		if (buf)
			writeImage(imgName, width, height, buf);
		if (m_adaptive) {
			unsigned char* adaptive_buf;
			raytracer->getAdaptiveBuffer(adaptive_buf, width, height);
			string tempName = "adaptive-density." + string(imgName);
			writeImage(tempName.c_str(), width, height, adaptive_buf);
		}

		double t = (double)(end - start) / CLOCKS_PER_SEC;
		//		int totalRays = TraceUI::resetCount();
		//		std::cout << "total time = " << t << " seconds,
		// rays traced = " << totalRays << std::endl;
		return 0;
	} else {
		std::cerr << "Unable to load ray file '" << rayName << "'"
		          << std::endl;
		return (1);
	}
}

void CommandLineUI::alert(const string& msg)
{
	std::cerr << msg << std::endl;
}

void CommandLineUI::usage()
{
	using namespace std;
	cerr << "usage: " << progName
	     << " [options] [input.ray output.png]" << endl
	     << "  -r <#>      set recursion level (default " << m_nDepth << ")" << endl
	     << "  -w <#>      set output image width (default " << m_nSize << ")" << endl
	     << "  -a <#>      threshold for adaptive antialiasing" << endl
	     << "  -j <FILE>   set parameters from JSON file" << endl
	     << "  -c <FILE>   one Cubemap file, the remainings will be detected automatically" << endl;
}
