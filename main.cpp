/*
LodePNG Examples

Copyright (c) 2005-2012 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/

#include <iostream>
#include <filesystem>
#include "png.h"
/*
3 ways to decode a PNG from a file to RGBA pixel data (and 2 in-memory ways).
*/

//g++ lodepng.cpp example_decode.cpp -ansi -pedantic -Wall -Wextra -O3


////Example 1
////Decode from disk to raw pixels with a single function call
//void decodeOneStep(const char* filename) {
//	std::vector<unsigned char> image; //the raw pixels
//	unsigned width, height;
//
//	//decode
//	unsigned error = lodepng::decode(image, width, height, filename);
//
//	//if there's an error, display it
//	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
//
//	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
//}
//
////Example 2
////Load PNG file from disk to memory first, then decode to raw pixels in memory.
//void decodeTwoSteps(const char* filename) {
//	std::vector<unsigned char> png;
//	std::vector<unsigned char> image; //the raw pixels
//	unsigned width, height;
//
//	//load and decode
//	unsigned error = lodepng::load_file(png, filename);
//	if (!error) error = lodepng::decode(image, width, height, png);
//
//	//if there's an error, display it
//	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
//
//	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
//}
//
////Example 3
////Load PNG file from disk using a State, normally needed for more advanced usage.
//void decodeWithState(const char* filename) {
//	std::vector<unsigned char> png;
//	std::vector<unsigned char> image; //the raw pixels
//	unsigned width, height;
//	lodepng::State state; //optionally customize this one
//
//	unsigned error = lodepng::load_file(png, filename); //load the image file with given filename
//	if (!error) error = lodepng::decode(image, width, height, state, png);
//
//	//if there's an error, display it
//	if (error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
//
//	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
//	//State state contains extra information about the PNG such as text chunks, ...
//}
#define DEBUG false

#include <sstream>
#include "clockTimer.h"

enum class Mode :char
{
	zoom = 'z',
	Zoom = 'Z',
	sharpen = 's',
	Sharpen = 'S',
	cut = 'c',
	Cut = 'C',
	unknown = '?'
};
void help()
{
	std::cout << "Check Help Info.\n\n"
		<< "Help:[--] is a prompt, not an input.\n"
		<< "Startup parameters-->\n"
		<< "[default zoom]: z\n"
		<< "[bicubic zoom]: Z [Feature not currently supported]\n"
		<< "[sharpen]: s\n"
		<< "[cut]: c [Feature not currently supported]\n\n"
		<< "Input Sample-->\n"
		<< "./exe filename.png z[default zoom] 1.0[zoom ratio:has default value] 0.5[center weight:has default value] 2[Exponent:has default value]\n"
		<< "[default zoom]\n[zoom ratio(from 0.001 to 32.0)]\n[center weight(from 0.25 to 13.0,0.25:MSAA,1.0:bilinear,>1:sharp)]\n[Exponent(from 1 to 4:0.5,1.0,2.0,4.0)]\n\n"
		<< "./exe filename.png s[sharpen] 4.0[sharpen ratio:has default value]\n"
		<< "[sharpen]\n[sharpen ratio(from 0.2 to 16.0)]\n\n"
		<< "./exe filename.png c[cut] 1024[Horizontal size] 1024[Vertical size]\n" << std::endl;
}

int main(int argc, char* argv[]) {
	//const char* filename = argc > 1 ? argv[1] : "test.png";
	std::filesystem::path pngfile;
	char mode = (char)Mode::unknown;
	clockTimer timer;
	std::istringstream iss;

	timer.TimerStart();

#if !DEBUG
	if (argc == 1)
	{
		std::cout << "No image or parameters entered!\n";
		help();
		exit(0);
	}
	else
		if (argc > 1)
		{
			pngfile = argv[1];
			std::cout << "Input filename:" << pngfile << '\n' << std::endl;
		}

	if (argc == 2)
	{
		std::cout << "Too few parameters.\n";
		help();
		exit(0);
	}

	iss.str(argv[2]);
	iss >> mode;

	float32_t Ratio = 1.0f;
	float32_t centerWeight = 0.64f;
	uint32_t exponent = (uint32_t)ImageProcessingTools::Exponent::square;

	switch (mode)
	{
	case (int)Mode::zoom:


		if (argc > 3)
		{
			iss.clear();
			iss.str(argv[3]);
			iss >> Ratio;

			if (argc > 4)
			{
				iss.clear();
				iss.str(argv[4]);
				iss >> centerWeight;

				if (argc > 5)
				{
					iss.clear();
					iss.str(argv[5]);
					iss >> exponent;
				}
			}
		}

		std::cout << "Input exponent factor:" << exponent << '\n';
		Clamp(exponent, 1, 4);
		std::cout << "Adoption exponent factor:" << exponent << '\n';

		ImageProcessingTools::zoomProgramDefault(Ratio, pngfile, centerWeight, (ImageProcessingTools::Exponent)exponent);
		break;
	case (int)Mode::Zoom:
		std::cout << "function call not designed!" << std::endl;
		exit(0);
		break;

	case (int)Mode::sharpen:
	case (int)Mode::Sharpen:
		if (argc > 3)
		{
			iss.clear();
			iss.str(argv[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::sharpenProgram(Ratio, pngfile);
		break;

	case (int)Mode::cut:
	case (int)Mode::Cut:
		std::cout << "function call not designed!" << std::endl;
		exit(0);
		break;

	default:
		std::cout << "Error:unknown working mode.\n";
		help();
		exit(0);
		break;
	}

	timer.TimerStop();

	std::cout << "End processing . . .\n"
		<< "Time used:" << timer.getTime() << "(second).\n" << std::endl;
#else
	float32_t Ratio = 2.5f;
	float32_t centerWeight = 0.64f;
	pngfile = "D:/pics/4K.png";
	uint32_t exponent = (uint32_t)ImageProcessingTools::Exponent::one;

	ImageProcessingTools::zoomProgramDefault(Ratio, pngfile, centerWeight, (ImageProcessingTools::Exponent)exponent);
	timer.TimerStop();
	std::cout << "Time use:" << timer.getTime();
#endif

	return 0;
}