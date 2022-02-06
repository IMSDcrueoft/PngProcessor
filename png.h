#pragma once
/*
PNG Processor Version 20220206

Copyright (c) 2005-2022 IM&SD

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

#ifndef PNG
#define PNG

#include "lodepng.h"
#include "AdaptString.h"
#include "CppParallelAccelerator.h"
#include "clockTimer.h"
#include <cassert>
#include <emmintrin.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <tuple>
#include <thread>

/*
* Processors in different working modes need to be treated differently
* note that only windows can use <ppl.h>
*/
#ifndef CPU_TYPE
#define CPU_TYPE
#define LITTLE_ENDIAN true
#endif

#ifndef OPERATING_SYSTEM
#define OPERATING_SYSTEM
#define WINDOWS_SYSTEM_CPU_PARALLEL false
#endif//OPERATING_SYSTEM

#if WINDOWS_SYSTEM_CPU_PARALLEL
#include<ppl.h>
#endif //PARALLELISM


/*
// [min,max)
#define Clamp(val,min,max)									                                      \
{                                                                                                 \
	assert((min) < (max) && "wrong! min is larger than max in Clamp.");                           \
                                                                                                  \
	if((val) < (min))                                                                             \
		(val) = (min);                                                                            \
	else                                                                                          \
	if((val) > (max))                                                                             \
		(val) = (max);                                                                            \
}

#define Lerp(val1,val2,weight)	                                                                  \
{								                                                                  \
	assert((0.0f <= (weight) && (weight)<=1.0f) && "wrong! weight in lerp out of range[0,1].");   \
	(val1) += (weight) * ((val2) - (val1));                                                       \
}
*/

using float32_t = float;
using float64_t = double;

static constexpr float32_t pi = 3.1415926f;
static constexpr float32_t maxColorPix = 255.0f;
static constexpr float32_t ColorPixTofloat = (1.0f / maxColorPix);

auto Clamp = [](auto& val, const auto& min, const auto& max)
{
	assert((min) < (max) && "wrong! min is larger than max in Clamp.");

	if (val < min)
		val = min;
	else
		if (val > max)
			val = max;
};

auto Lerp = [](auto& val1, const auto& val2, const auto& weight)
{
	assert((0.0f <= (weight) && (weight) <= 1.0f) && "wrong! weight in lerp out of range[0,1].");

	val1 += weight * (val2-val1);
};

#define _mm_mul_add_ps(a,b,c) _mm_add_ps(_mm_mul_ps((a),(b)),(c))
#define _mm_fma_ps(a,b,c) _mm_mul_add_ps((a),(b),(c))
/*
* R8G8B8A8 color with uint8_t
*/
struct alignas(4) RGBAColor_8i
{
	union
	{
		uint32_t data = 0xFF'FF'FF'FF;

		struct alignas(4)
		{
			uint8_t R;
			uint8_t G;
			uint8_t B;
			uint8_t A;
		};
	};

	RGBAColor_8i() = default;
	RGBAColor_8i(byte* ptr);
	RGBAColor_8i(unknown_pointer ptr);
	RGBAColor_8i(const uint8_t& color);
	RGBAColor_8i(const uint8_t& R, const uint8_t& G, const uint8_t& B, const uint8_t& A = 0xFF);

	RGBAColor_8i& operator=(const uint32_t& rgba);
	RGBAColor_8i operator*(const int32_t& num);
	RGBAColor_8i& operator~();
};

struct PngData
{
public:
	PngData() = default;
	PngData(std::vector<RGBAColor_8i>& image_in,const uint32_t& width,const uint32_t& height);

	std::vector<RGBAColor_8i>& getRGBA_uint8();

	RGBAColor_8i& operator()(int32_t column,int32_t row);

	byte& operator[](const size_t& index);

	void loadRGBAtoByteStream();
	void clearImage();
	void clearRGBA_uint8();
	void clear();

public:
	uint32_t width = 0u;
	uint32_t height = 0u;

	std::vector<byte> image;

protected:
	std::vector<RGBAColor_8i> imageRGBA_uint8;
};


struct alignas(16) RGBAColor_32f
{
	union
	{
		__m128 float32X4;

		struct alignas(16)
		{
			float32_t A;
			float32_t B;
			float32_t G;
			float32_t R;
		};

		struct alignas(16)
		{
			float32_t X;
			float32_t Y;
			float32_t Z;
			float32_t W;
		};

		alignas(16) float32_t arr[4];
	};
protected:
	RGBAColor_32f(const __m128& vec4);

public:
	RGBAColor_32f();
	RGBAColor_32f(const float32_t& val);
	RGBAColor_32f(const float32_t& r, const float32_t& g, const float32_t& b, const float32_t& a = 1.0f);
	RGBAColor_32f(const RGBAColor_8i& color);
	RGBAColor_32f(const RGBAColor_8i& color,const float32_t& multNum);

	RGBAColor_32f operator+(const float32_t& num) const;
	RGBAColor_32f operator-(const float32_t& num) const;
	RGBAColor_32f operator*(const float32_t& num) const;
	RGBAColor_32f operator/(const float32_t& num) const;

	RGBAColor_32f& operator+=(const float32_t& num);
	RGBAColor_32f& operator-=(const float32_t& num);
	RGBAColor_32f& operator*=(const float32_t& num);
	RGBAColor_32f& operator/=(const float32_t& num);

	RGBAColor_32f operator+(const RGBAColor_32f& nextColor) const;
	RGBAColor_32f operator-(const RGBAColor_32f& nextColor) const;
	RGBAColor_32f operator*(const RGBAColor_32f& nextColor) const;
	RGBAColor_32f operator/(const RGBAColor_32f& nextColor) const;

	RGBAColor_32f& operator+=(const RGBAColor_32f& nextColor);
	RGBAColor_32f& operator-=(const RGBAColor_32f& nextColor);
	RGBAColor_32f& operator*=(const RGBAColor_32f& nextColor);
	RGBAColor_32f& operator/=(const RGBAColor_32f& nextColor);

	const float32_t& operator[](const uint32_t& index) const;

	RGBAColor_8i toRGBAColor_8i();

	void FMA(RGBAColor_32f& addResult, const RGBAColor_32f& mul1, const RGBAColor_32f& mul2);
};
using floatVec4 = RGBAColor_32f;


class ImageProcessingTools
{
public:
	enum class Exponent:uint8_t
	{
		half = 1,
		one = 2,
		square = 3,
		quartet = 4,
	};

	enum class Mode :char
	{
		zoom = 'z',
		Zoom = 'Z',
		sharpen = 's',
		Sharpen = 'S',
		cut = 'c',
		Cut = 'C',
		toneMapping = 't',
		ToneMapping = 'T',
		grayScale = 'g',
		GrayScale = 'G',
		reverseColor = 'r',
		ReverseColor = 'R',
		vividness = 'v',
		Vividness = 'V',
		binarization = 'b',
		Binarization = 'B',
		quaternization = 'q',
		Quaternization = 'Q',
		hexadecimalization = 'h',
		Hexadecimalization = 'H',
		unknown = '?'
	};

protected:
	static void GrayColor(const RGBAColor_8i& color,byte& result);
	static void BinarizationColor(const RGBAColor_8i& color,const float32_t& threshold,byte& result);//Too few colors, need to adjust the threshold
	static void QuaternizationColor(const RGBAColor_8i& color, const float32_t& threshold, byte& result);//Too few colors, need to adjust the threshold
	static void HexadecimalizationColor(const RGBAColor_8i& color, byte& result);//16 colors are rich enough, no need for thresholding anymore
	static void ReverseColor(RGBAColor_8i& color);
	static void VividnessAdjustmentColor(RGBAColor_32f& color,const float32_t& changeMagnification);
	static void ACESToneMappingColor(RGBAColor_32f& color, const float32_t& adapted_lum);

protected:
	static float32_t cubicConvolutionZoomFormula(const float32_t& a, const float32_t& x);

	static void weightEffectAdapt(const float32_t& dx, const float32_t& dy, floatVec4& weightResult, float32_t(*Index)(const float32_t&));
	static void weightEffectHalf(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);
	static void weightEffectOne(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);
	static void weightEffectSquare(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);
	static void weightEffectQuartet(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);

public:
	static void help();
	static void commandStartUps(int32_t argCount,STR argValues[]);

	static void zoomProgramDefault(float32_t& zoomRatio, std::filesystem::path& pngfile,float32_t& CenterWeight,const Exponent& exponent = Exponent::one);
	static void zoomProgramCubicConvolution(float32_t& zoomRatio, std::filesystem::path& pngfile, float32_t& a);
	static void laplaceSharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile);
	static void gaussLaplaceSharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile);
	static void hdrToneMappingColorProgram(float32_t& lumRatio, std::filesystem::path& pngfile);
	static void reverseColorProgram(std::filesystem::path& pngfile);
	static void grayColorProgram(std::filesystem::path& pngfile);
	static void channelGrayColorProgram(std::filesystem::path& pngfile);
	static void vividnessAdjustmentColorProgram(float32_t& VividRatio,std::filesystem::path& pngfile);
	static void binarizationColorProgram(float32_t& threshold, std::filesystem::path& pngfile);
	static void quaternizationColorProgram(float32_t& threshold, std::filesystem::path& pngfile);
	static void hexadecimalizationColorProgram(std::filesystem::path& pngfile);
	static void fastSplitHorizonProgram(uint32_t& splitInterval, std::filesystem::path& pngfile);
	static void blockSplit(uint32_t& horizontalInterval,uint32_t& verticalInterval, std::filesystem::path& pngfile);

	//The following three methods rely on lodepng
	static void importFile(PngData& data, std::filesystem::path& pngfile);
	static void exportFile(PngData& result, std::wstring& resultname,
		const LodePNGColorType& colorType = LodePNGColorType::LCT_RGBA,const uint32_t& bitdepth = 8u);

	static void exportFile(const byte* result,const uint32_t& width,const uint32_t& height, std::wstring& resultname,
		const LodePNGColorType& colorType = LodePNGColorType::LCT_RGBA, const uint32_t& bitdepth = 8u);

public:
	static bool Zoom_DefaultSampling4x4(PngData& input, PngData& result, const float32_t& magnification = 1.0f, const float32_t& CenterWeight = 0.64f,
		void (*WeightEffact)(const float32_t& dx, const float32_t& dy,floatVec4& weightResult) = ImageProcessingTools::weightEffectSquare);

	static bool Zoom_CubicConvolutionSampling4x4(PngData& input, PngData& result, const float32_t& magnification = 1.0f, const float32_t& a = -0.5f);

	static bool SharpenLaplace3x3(PngData& input, PngData& result, const float32_t& strength = 1.0f);
	static bool SharpenGaussLaplace5x5(PngData& input, PngData& result, const float32_t& strength = 1.0f);
	static bool AecsHdrToneMapping(PngData& inputOutput,const float32_t& lumRatio = 1.0f);
	static bool ReverseColorImage(PngData& inputOutput);
	static bool Grayscale(PngData& input, PngData& result);
	static bool ChannelGrayScale(PngData& input, PngData& resultR, PngData& resultG, PngData& resultB);
	static bool VividnessAdjustment(PngData& inputOutput, const float32_t& vividRatio = 0.2f);
	static bool Binarization(PngData& input, PngData& result, const float32_t& threshold = 0.5f);
	static bool Quaternization(PngData& input, PngData& result, const float32_t& threshold = 0.5f);
	static bool Hexadecimalization(PngData& input, PngData& result);
};


inline RGBAColor_8i::RGBAColor_8i(byte* ptr)
{
#if LITTLE_ENDIAN
	this->data = *(reinterpret_cast<uint32_t*>(ptr));
#else
	data = 0x00'00'00'FF & *(ptr + 0u);
	data <<= 8u;
	data += *(ptr + 1u);
	data <<= 8u;
	data += *(ptr + 2u);
	data <<= 8u;
	data += *(ptr + 3u);
#endif
}

inline RGBAColor_8i::RGBAColor_8i(unknown_pointer ptr)
{
	new (this) RGBAColor_8i(static_cast<byte*>(ptr));
}

inline RGBAColor_8i::RGBAColor_8i(const uint8_t& color)
{
	this->R = color;
	this->G = color;
	this->B = color;
	this->A = 0xFF;
}

inline RGBAColor_8i::RGBAColor_8i(const uint8_t& R, const uint8_t& G, const uint8_t& B, const uint8_t& A)
{
	this->R = R;
	this->G = G;
	this->B = B;
	this->A = A;
}

inline RGBAColor_8i& RGBAColor_8i::operator=(const uint32_t& rgba)
{
	this->data = rgba;
	return *this;
}

inline RGBAColor_8i RGBAColor_8i::operator*(const int32_t& num)
{
	RGBAColor_8i result;
	result.R = num * this->R;
	result.G = num * this->G;
	result.B = num * this->B;
	return result;
}

inline RGBAColor_8i& RGBAColor_8i::operator~()
{
	this->data = ~(this->data);
	this->A = ~(this->A);

	return *this;
}

inline RGBAColor_32f::RGBAColor_32f(const __m128& vec4)
{
	float32X4 = vec4;
}

inline RGBAColor_32f::RGBAColor_32f()
{
	float32X4 = _mm_set1_ps(1.0f);
}

inline RGBAColor_32f::RGBAColor_32f(const float32_t& val)
{
	float32X4 = _mm_set1_ps(val);
}

inline RGBAColor_32f::RGBAColor_32f(const float32_t& r, const float32_t& g, const float32_t& b, const float32_t& a)
{
	float32X4 = _mm_set_ps(r, g, b, a);
}

inline RGBAColor_32f::RGBAColor_32f(const RGBAColor_8i& color)
{
	float32X4 = _mm_mul_ps(_mm_set_ps(color.R, color.G, color.B, color.A), _mm_set1_ps(ColorPixTofloat));
}

inline RGBAColor_32f::RGBAColor_32f(const RGBAColor_8i& color, const float32_t& multNum)
{
	this->float32X4 = _mm_mul_ps(_mm_mul_ps(_mm_set_ps(color.R, color.G, color.B, color.A), _mm_set1_ps(ColorPixTofloat)),_mm_set1_ps(multNum));
}

inline RGBAColor_32f RGBAColor_32f::operator+(const float32_t& num) const
{
	return RGBAColor_32f(_mm_add_ps(this->float32X4, _mm_load1_ps(&num)));
}

inline RGBAColor_32f RGBAColor_32f::operator-(const float32_t& num) const
{
	return RGBAColor_32f(_mm_sub_ps(this->float32X4, _mm_load1_ps(&num)));
}

inline RGBAColor_32f RGBAColor_32f::operator*(const float32_t& num) const
{
	return RGBAColor_32f(_mm_mul_ps(this->float32X4, _mm_load1_ps(&num)));
}

inline RGBAColor_32f RGBAColor_32f::operator/(const float32_t& num) const
{
	return RGBAColor_32f(_mm_div_ps(this->float32X4, _mm_load1_ps(&num)));
}

inline RGBAColor_32f& RGBAColor_32f::operator+=(const float32_t& num)
{
	this->float32X4 = _mm_add_ps(this->float32X4, _mm_load1_ps(&num));
	return *this;
}

inline RGBAColor_32f& RGBAColor_32f::operator-=(const float32_t& num)
{
	this->float32X4 = _mm_sub_ps(this->float32X4, _mm_load1_ps(&num));
	return *this;
}

inline RGBAColor_32f& RGBAColor_32f::operator*=(const float32_t& num)
{
	this->float32X4 = _mm_mul_ps(this->float32X4, _mm_load1_ps(&num));
	return *this;
}

inline RGBAColor_32f& RGBAColor_32f::operator/=(const float32_t& num)
{
	this->float32X4 = _mm_div_ps(this->float32X4, _mm_load1_ps(&num));
	return *this;
}

inline RGBAColor_32f RGBAColor_32f::operator+(const RGBAColor_32f& nextColor) const
{
	return RGBAColor_32f(_mm_add_ps(this->float32X4, nextColor.float32X4));
}

inline RGBAColor_32f RGBAColor_32f::operator-(const RGBAColor_32f& nextColor) const
{
	return RGBAColor_32f(_mm_sub_ps(this->float32X4, nextColor.float32X4));
}

inline RGBAColor_32f RGBAColor_32f::operator*(const RGBAColor_32f& nextColor) const
{
	return RGBAColor_32f(_mm_mul_ps(this->float32X4, nextColor.float32X4));
}

inline RGBAColor_32f RGBAColor_32f::operator/(const RGBAColor_32f& nextColor) const
{
	return RGBAColor_32f(_mm_div_ps(this->float32X4, nextColor.float32X4));
}

inline RGBAColor_32f& RGBAColor_32f::operator+=(const RGBAColor_32f& nextColor)
{
	this->float32X4 = _mm_add_ps(this->float32X4, nextColor.float32X4);
	return *this;
}

inline RGBAColor_32f& RGBAColor_32f::operator-=(const RGBAColor_32f& nextColor)
{
	this->float32X4 = _mm_sub_ps(this->float32X4, nextColor.float32X4);
	return *this;
}

inline RGBAColor_32f& RGBAColor_32f::operator*=(const RGBAColor_32f& nextColor)
{
	this->float32X4 = _mm_mul_ps(this->float32X4, nextColor.float32X4);
	return *this;
}

inline RGBAColor_32f& RGBAColor_32f::operator/=(const RGBAColor_32f& nextColor)
{
	this->float32X4 = _mm_div_ps(this->float32X4, nextColor.float32X4);
	return *this;
}

inline const float32_t& RGBAColor_32f::operator[](const uint32_t& index)const
{
	assert(0u <= index && index < 4u && "Index out of range.");

	return this->arr[index];
}

inline RGBAColor_8i RGBAColor_32f::toRGBAColor_8i()
{
	RGBAColor_32f result = *this * maxColorPix;

	Clamp(result.R, 0.0f, maxColorPix);
	Clamp(result.G, 0.0f, maxColorPix);
	Clamp(result.B, 0.0f, maxColorPix);
	Clamp(result.A, 0.0f, maxColorPix);

	return RGBAColor_8i(
		static_cast<uint8_t>(result.R),
		static_cast<uint8_t>(result.G),
		static_cast<uint8_t>(result.B),
		static_cast<uint8_t>(result.A));
}

inline void RGBAColor_32f::FMA(RGBAColor_32f& addResult, const RGBAColor_32f& mul1, const RGBAColor_32f& mul2)
{
	addResult.float32X4 = _mm_fma_ps(mul1.float32X4, mul2.float32X4, addResult.float32X4);
}

inline PngData::PngData(std::vector<RGBAColor_8i>& image_in,const uint32_t& width,const uint32_t& height)
{
	this->width = width;
	this->height = height;

	this->image.reserve((static_cast<size_t>(width) * height) << 2);

	for (const auto& rgba : image_in)
	{
		this->image.push_back(rgba.R);
		this->image.push_back(rgba.G);
		this->image.push_back(rgba.B);
		this->image.push_back(rgba.A);
	}
}

inline std::vector<RGBAColor_8i>& PngData::getRGBA_uint8()
{
	if (this->imageRGBA_uint8.size() == 0u)
	{
		if (this->image.size() != 0u)
		{
			this->imageRGBA_uint8.reserve(this->image.size() >> 2);

			for (size_t index = 0u; index < this->image.size(); index += 4)
			{
				this->imageRGBA_uint8.push_back(
					RGBAColor_8i{ this->image.data() + index }
				);
			}
		}
	}
	return this->imageRGBA_uint8;
}

inline RGBAColor_8i& PngData::operator()(int32_t column,int32_t row)
{
	//assert(row >= 0 && "row out of image range.");
	//assert(row < width && "row out of image range.");
	//assert(column >= 0 && "column out of image range.");
	//assert(column < height && "column out of image range.");

	Clamp(column, 0, width - 1);
	Clamp(row, 0, height - 1);

	return this->getRGBA_uint8()[size_t(row) * width + column];
}

inline byte& PngData::operator[](const size_t& index)
{
	assert(this->image.size() >= index && "index out of range.");

	return this->image[index];
}

inline void PngData::loadRGBAtoByteStream()
{
	this->image.clear();
	/*this->image.reserve(this->imageRGBA_uint8.size() << 2);*/

	// Acceleration is worthless here
	//for (const auto& rgba : this->imageRGBA_uint8)
	//{
	//	this->image.push_back(rgba.R);
	//	this->image.push_back(rgba.G);
	//	this->image.push_back(rgba.B);
	//	this->image.push_back(rgba.A);
	//}

	this->image.resize(this->imageRGBA_uint8.size() << 2);
	// Acceleration is worthless here
	for (size_t index = 0u; index < this->imageRGBA_uint8.size(); ++index)
	{
		image[(index << 2) + 0] = this->imageRGBA_uint8[index].R;
		image[(index << 2) + 1] = this->imageRGBA_uint8[index].G;
		image[(index << 2) + 2] = this->imageRGBA_uint8[index].B;
		image[(index << 2) + 3] = this->imageRGBA_uint8[index].A;
	}
}

inline void PngData::clearImage()
{
	this->image.clear();
	std::vector<byte>().swap(this->image);
}

inline void PngData::clearRGBA_uint8()
{
	this->imageRGBA_uint8.clear();
	std::vector<RGBAColor_8i>().swap(this->imageRGBA_uint8);
}

inline void PngData::clear()
{
	clearImage();
	clearRGBA_uint8();
}

inline void ImageProcessingTools::GrayColor(const RGBAColor_8i& color, byte& result)
{
	result = (color.R + color.G + color.B) * 0.33333f;
}

inline void ImageProcessingTools::BinarizationColor(const RGBAColor_8i& color, const float32_t& threshold, byte& result)
{
	float32_t avg = (color.R + color.G + color.B) * 0.33333f;

	float32_t l = threshold * maxColorPix;

	result = (avg >= l) ? 0b1111'1111u : 0b0000'0000u;
}

inline void ImageProcessingTools::QuaternizationColor(const RGBAColor_8i& color, const float32_t& threshold, byte& result)
{
	float32_t avg = (color.R + color.G + color.B) * 0.33333f;

	float32_t l1 = 171.0f;
	float32_t l2 = 86.0f;
	float32_t l3 = 1.0f;

	Lerp(l1, 255.0f, threshold);
	Lerp(l2, 170.0f, threshold);
	Lerp(l3, 85.0f, threshold);

	if (avg >= l1)
	{
		result = 0b1111'1111u;
	}
	else
		if (avg >= l2)
		{
			result = 0b1010'1010u;
		}
		else
			if (avg >= l3)
			{
				result = 0b0101'0101u;
			}
			else
			{
				result = 0b0000'0000u;
			}
}

inline void ImageProcessingTools::HexadecimalizationColor(const RGBAColor_8i& color, byte& result)
{
	uint16_t avg = (color.R + color.G + color.B) / 3u;

	if (avg >= 247u)
	{
		result = 0b1111'1111u;
	}
	else
		if(avg >= 230u)
		{
			result = 0b1110'1110u;
		}
		else
			if (avg >= 213u)
			{
				result = 0b1101'1101u;
			}
			else
				if (avg >= 196u)
				{
					result = 0b1100'1100u;
				}
				else
					if (avg >= 179u)
					{
						result = 0b1011'1011u;
					}
					else
						if (avg >= 162u)
						{
							result = 0b1010'1010u;
						}
						else
							if (avg >= 145u)
							{
								result = 0b1001'1001u;
							}
							else
								if (avg >= 128u)
								{
									result = 0b1000'1000u;
								}
								else
									if (avg >= 111u)
									{
										result = 0b0111'0111u;
									}
									else
										if (avg >= 94u)
										{
											result = 0b0110'0110u;
										}
										else
											if (avg >= 77u)
											{
												result = 0b0101'0101u;
											}
											else
												if (avg >= 60u)
												{
													result = 0b0100'0100u;
												}
												else
													if (avg >= 43u)
													{
														result = 0b0011'0011u;
													}
													else
														if (avg >= 26u)
														{
															result = 0b0010'0010u;
														}
														else
															if (avg >= 9u)
															{
																result = 0b0001'0001u;
															}
															else
																{
																	result = 0b0000'0000u;
																}

}

inline void ImageProcessingTools::ReverseColor(RGBAColor_8i& color)
{
	color = ~color;
}

inline void ImageProcessingTools::VividnessAdjustmentColor(RGBAColor_32f& color,const float32_t& changeMagnification)
{
	//worthless calculation
	if (fabsf(changeMagnification) < 0.001f) return;

	float32_t Alpha = color.A;

	float32_t avg = (color.R + color.G + color.B) * 0.33333f;

	color -= avg;

	color *= (1.0f + changeMagnification);

	color += avg;

	color.A = Alpha;
}

inline void ImageProcessingTools::ACESToneMappingColor(RGBAColor_32f& color, const float32_t& adapted_lum)
{
	static constexpr float32_t A = 2.51f;
	static constexpr float32_t B = 0.03f;
	static constexpr float32_t C = 2.43f;
	static constexpr float32_t D = 0.59f;
	static constexpr float32_t E = 0.14f;

	const float32_t Alpha = color.A;

	color *= adapted_lum;

	color = (color * (color * A + B)) / (color * (color * C + D) + E);

	color.A = Alpha;
}

inline float32_t ImageProcessingTools::cubicConvolutionZoomFormula(const float32_t& a, const float32_t& x)
{
	// -3 <= a <= -0.1
	// -2 <= x <= 2
	float32_t absX = fabsf(x);

	if (absX <= 1.0f)
	{
		return (
			(a + 2.0f) * (absX * absX * absX)
			- (a + 3.0f) * (absX * absX)
			+ 1.0f
			);
	}
	else
		if (absX < 2.0f)
		{
			return (
				a * (absX * absX * absX)
				- 5.0f * a * (absX * absX)
				+ 8.0f * a * absX
				- 4.0f * a);
		}
		else
		{
			return 0.0f;
		}
}

inline void ImageProcessingTools::weightEffectAdapt(const float32_t& dx, const float32_t& dy, floatVec4& weightResult, float32_t(*Index)(const float32_t&))
{
	float32_t sum = 0.0f;

	float32_t dx2 = dx * dx;
	float32_t dy2 = dy * dy;
	float32_t _dx2 = (1.0f - dx) * (1.0f - dx);
	float32_t _dy2 = (1.0f - dy) * (1.0f - dy);

	sum += weightResult.X = Index(_dx2 + _dy2);
	sum += weightResult.W = Index(dx2 + dy2);
	sum += weightResult.Y = Index(dx2 + _dy2);
	sum += weightResult.Z = Index(_dx2 + dy2);

	weightResult *= (4.0f / sum);
}

inline void ImageProcessingTools::weightEffectHalf(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	weightEffectAdapt(dx, dy, weightResult,
		[](const float32_t& value)
		{
			return std::sqrtf(std::sqrtf(value));
		});
}

inline void ImageProcessingTools::weightEffectOne(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	weightEffectAdapt(dx, dy, weightResult,
		[](const float32_t& value)
		{
			return std::sqrtf(value);
		});
}

inline void ImageProcessingTools::weightEffectSquare(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	weightEffectAdapt(dx, dy, weightResult,
		[](const float32_t& value)
		{
			return value;
		});
}

inline void ImageProcessingTools::weightEffectQuartet(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	weightEffectAdapt(dx, dy, weightResult,
		[](const float32_t& value)
		{
			return value * value;
		});
}

#endif // !PNG