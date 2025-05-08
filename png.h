#pragma once
/*
PNG Processor Version 20220322

Copyright (c) 2021-2025 IMSDCrueoft

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. Do not distort the source of this software; you cannot Claiming that you wrote the original software.
if you use this software In a product, the confirmation in the product documentation will be Appreciated but not required.

2. Changed source versions must be clearly identified and must not be Mistaken for original software.

3. This notice may not be deleted or changed from any source distribute.

4. The LodePng header files and source files are modified from the project of GItHub Lode Vandevenne,
so you should also comply with the requirements of its header declaration
*/

#ifndef PNG_H
#define PNG_H
#include "Image.h"
#include "clockTimer.h"
#include "lodepng.h"
#include "AdaptString.h"

class PngProcessingTools :public ImageProcessingTools
{
public:
	enum class Mode :char
	{
		binarization = 'b',
		Binarization = 'B',
		cut = 'c',
		Cut = 'C',
		encryption = 'e',
		Encryption = 'E',
		filter = 'f',
		Filter = 'F',
		grayScale = 'g',
		GrayScale = 'G',
		hexadecimalization = 'h',
		HSLAdjustment = 'H',
		InterlacedScanning = 'i',
		mosaicPixelation = 'm',
		MixedGraph = 'M',
		pixelToRGB8_3x3 = 'p',
		quaternization = 'q',
		Quaternization = 'Q',
		reverseColor = 'r',
		ReverseColor = 'R',
		sharpen = 's',
		Sharpen = 'S',
		toneMapping = 't',
		ToneMapping = 'T',
		vividness = 'v',
		Vividness = 'V',
		zoom = 'z',
		Zoom = 'Z',
		unknown = '?'
	};

public:
	static void help();
	static void commandStartUps(int32_t argCount, STR argValues[]);

	static void zoomProgramDefault(float32_t& zoomRatio, std::filesystem::path& pngfile, float32_t& threshold, const Exponent& exponent = Exponent::one);
	static void zoomProgramBicubicConvolution(float32_t& zoomRatio, std::filesystem::path& pngfile, float32_t& a);
	static void laplaceSharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile);
	static void gaussLaplaceSharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile);
	static void hdrToneMappingColorProgram(float32_t& lumRatio, std::filesystem::path& pngfile);
	static void reverseColorProgram(std::filesystem::path& pngfile);
	static void grayColorProgram(std::filesystem::path& pngfile);
	static void channelGrayColorProgram(std::filesystem::path& pngfile);
	static void vividnessAdjustmentColorProgram(float32_t& VividRatio, std::filesystem::path& pngfile);
	static void natualvividnessAdjustmentColorProgram(float32_t& VividRatio, std::filesystem::path& pngfile);
	static void binarizationColorProgram(float32_t& threshold, std::filesystem::path& pngfile);
	static void quaternizationColorProgram(float32_t& threshold, std::filesystem::path& pngfile);
	static void hexadecimalizationColorProgram(std::filesystem::path& pngfile);
	static void fastSplitHorizonProgram(uint32_t& splitInterval, std::filesystem::path& pngfile);
	static void blockSplitProgram(uint32_t& horizontalInterval, uint32_t& verticalInterval, std::filesystem::path& pngfile);
	static void surfaceBlurfilterProgram(float32_t& threshold, std::filesystem::path& pngfile, int32_t& radius);
	static void sobelEdgeEnhancementProgram(float32_t& strength, std::filesystem::path& pngfile, float32_t& thresholdMin, float32_t& thresholdMax);
	static void mosaicPixelationProgram(uint32_t& sideLength, std::filesystem::path& pngfile);
	static void mixedPicturesProgram(uint32_t& workMode, std::filesystem::path& pngfileOut, std::filesystem::path& pngfileIn);
	static void pixelToRGB8_3x3Program(float32_t& brightness, std::filesystem::path& pngfile);
	static void interlacedScanningProgram(std::filesystem::path& pngfile);
	static void encryption_xorProgram(uint32_t& xorKey, std::filesystem::path& pngfile);
	static void hslAdjustMentProgram(float32_t& hueChange, float32_t& saturationRatio, float32_t& lightnessRatio, std::filesystem::path& pngfile);

	//The following three methods rely on lodepng
	static void importFile(TextureData& data, std::filesystem::path& pngfile);
	static void exportFile(TextureData& result, std::wstring& resultname,
		const LodePNGColorType& colorType = LodePNGColorType::LCT_RGBA, const uint32_t& bitdepth = 8u);

	static void exportFile(const byte* result, const uint32_t& width, const uint32_t& height, std::wstring& resultname,
		const LodePNGColorType& colorType = LodePNGColorType::LCT_RGBA, const uint32_t& bitdepth = 8u);
};
#endif // !PNG