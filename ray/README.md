Name: Kevin Jin
UTEID: kj23743
Partner: James Jackson
UTEID: jsj999

==============================
Milestone 2:
==============================
Implementing the BVH sped up our running of dragon.ray from ~66 seconds originally to ~.45 seconds. Similarly, trimesh2.ray ran in ~22 seconds, whereas our old version failed to terminate after 5 minutes, so clearly, the BVH greatly improved our performance. 

# Extra Attributes:

## Creative Scene:
We built a scene that represents the metal sphere sculpture on UT campus (just next to Speedway). We also included a cube-map that shows the UT Tower in the background. We used geometries and shifted them around to represent different parts of the scene, from the stairs to the windows on the buildings. This scene file is custom_scenes/ut_spheres.ray, and the associated cubemap is custom_scenes/cubemaps/ut_tower/. Here is a link to the image produced by our raytracer: https://ibb.co/nwNjQWQ.  


## Bells and Whistle Features

### Adaptive Termination
Added adaptive termination when tracing shadow rays. Whenever a light ray is too weak (below a user supplied threshold), the tracing of that ray is terminated. Depending on the max recurse limit, this could drastically speed up performance and tracing. 

### Stochastic (jittered) supersampling
Changed original anti-aliasing supersampling to support jittered supersampling, which chooses a random point within subpixels rather than the center. Results in 

### Antialiasing by adaptive supersampling 
Implemented adaptive sampling as a command-line flag. The '-a <threshold>' flag allows for adaptive supersampling that terminates when the sum of the deviations from each sample to the average is less than the provided threshold. That is, when the sum(abs(sample - average)) < threshold. It also terminates when the maximum recursive supersampling depth of 5 is exceeded. This basically means that when the colors are close enough to each other, then no further sampling is needed. Whenever this flag is used, a density map of the number of recursive supersampling rays is produced at the file whose name is "adaptive-density." prepended to the provided output image file. Locations where more supersampling rays were necessary will be highlighted with a brighter green. Here is a link to a screenshot of an example of the behavior: https://ibb.co/tqfKkSz. 

### Overlapping Refractive Objects
Previously, we assumed that there were no overlapping refractive objects. This made the implementation much simpler, because whenever exiting a refractive object, we know the index to be about 1 (index of refraction for air). However, to handle nested refractive objects, we made it so that whenever exiting a refractive object, we shoot another test ray to check if we are within another refractive object. If we are, there will be an intersection with which we can use to determine the index of refraction of the encompassing object. 
        
### Normal Mapping
Implemented support for normal maps by adding a normals = map("x.png") property to material. Screen caps can be seen under assets/custom/screens.
