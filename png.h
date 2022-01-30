#pragma once
#ifndef PNG
#define PNG

#include "lodepng.h"
#include "AdaptString.h"
#include <cassert>
#include <emmintrin.h>
#include <filesystem>
#include <iostream>
#include <memory>

/*
* Processors in different working modes need to be treated differently
* only windows can use <ppl.h>
*/
#ifndef CPU_TYPE
#define LITTLE_ENDIAN true
#endif

#ifndef OPERATING_SYSTEM
#define WINDOWS_SYSTEM_CPU_PARALLEL false

#if WINDOWS_SYSTEM_CPU_PARALLEL
#define OPENMP_CPU false
#else
#define CPP_MULTITHREAD_PARALLELISM true
#endif //CPP_MULTITHREAD_PARALLELISM

#endif//OPERATING_SYSTEM

#if WINDOWS_SYSTEM_CPU_PARALLEL
#include<ppl.h>
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL

#if CPP_MULTITHREAD_PARALLELISM
#include<thread>
#endif//OPENMP_CPU

using float32_t = float;
using float64_t = double;

// [min,max)
#define Clamp(val,min,max)									            \
{                                                                       \
	assert(min < max && "wrong! min is larger than max in Clamp.");     \
                                                                        \
	if(val < min)                                                       \
		val = min;                                                      \
	else                                                                \
	if(val > max)                                                       \
		val = max;                                                      \
}

#define Lerp(val1,val2,weight)	                                                          \
{								                                                          \
	assert((0.0f <= weight&& weight<=1.0f) && "wrong! weight in lerp out of range[0,1].");\
	val1 += weight * (val2 - val1);                                                       \
}

constexpr float maxColorPix = 255.0f;
constexpr float ColorPixTofloat = (1.0f / maxColorPix);
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

	RGBAColor_8i(byte* ptr)
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

	RGBAColor_8i(unknown_pointer ptr)
	{
		new (this) RGBAColor_8i(static_cast<byte*>(ptr));
	}

	RGBAColor_8i(const uint8_t& R, const uint8_t& G, const uint8_t& B, const uint8_t& A = 0xFF)
	{
		this->R = R;
		this->G = G;
		this->B = B;
		this->A = A;
	}

	RGBAColor_8i& operator=(const uint32_t& rgba)
	{
		this->data = rgba;
		return *this;
	}

	RGBAColor_8i operator*(const int32_t& num)
	{
		RGBAColor_8i result;
		result.R = num * this->R;
		result.G = num * this->G;
		result.B = num * this->B;
		return result;
	}
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
	};

protected:
	RGBAColor_32f(const __m128& vec4)
	{
		float32X4 = vec4;
	}

public:
	RGBAColor_32f()
	{
		float32X4 = _mm_set1_ps(1.0f);
	}

	RGBAColor_32f(const float32_t& r, const float32_t& g, const float32_t& b, const float32_t& a = 1.0f)
	{
		float32X4 = _mm_set_ps(r, g, b, a);
	}

	RGBAColor_32f(const RGBAColor_8i& color)
	{
		new (this) RGBAColor_32f(color.R, color.G, color.B, color.A);

		*this *= ColorPixTofloat;
	}

	RGBAColor_32f operator*(const float32_t& num)
	{
		return RGBAColor_32f(_mm_mul_ps(float32X4, _mm_load1_ps(&num)));
	}

	RGBAColor_32f& operator*=(const float32_t& num)
	{
		this->float32X4 = _mm_mul_ps(float32X4, _mm_load1_ps(&num));
		return *this;
	}

	RGBAColor_32f operator+(const RGBAColor_32f& nextColor)
	{
		return RGBAColor_32f(_mm_add_ps(this->float32X4, nextColor.float32X4));
	}

	RGBAColor_32f& operator+=(const RGBAColor_32f& nextColor)
	{
		this->float32X4 = _mm_add_ps(this->float32X4, nextColor.float32X4);

		return *this;
	}

	RGBAColor_8i toRGBAColor_8i()
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

protected:
	static void weightEffectHalf(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);
	static void weightEffectOne(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);
	static void weightEffectSquare(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);
	static void weightEffectQuartet(const float32_t& dx, const float32_t& dy, floatVec4& weightResult);

public:
	static void zoomProgramDefault(float32_t& zoomRatio, std::filesystem::path& pngfile,float32_t& CenterWeight,const Exponent& exponent = Exponent::one);
	static void sharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile);
	static void importFile(PngData& data, std::filesystem::path& pngfile);
	static void exportFile(PngData& result, std::wstring& resultname);
	static void exportFile(const byte* result,const uint32_t& width,const uint32_t& height, std::wstring& resultname);

public:
	static bool Zoom_DefaultSampling4x4(PngData& input, PngData& result, const float32_t& magnification = 1.0f, const float32_t& CenterWeight = 0.64f,
		void (*WeightEffact)(const float32_t& dx, const float32_t& dy,floatVec4& weightResult) = ImageProcessingTools::weightEffectSquare);

	static bool Sharpen3x3(PngData& input, PngData& result, const float32_t& strength = 1.0f);
};


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
#endif // !PNG
/*
SSE2每次可以处理4个RGBA像素RGBA'RGBA'RGBA'RGBA 或者 RGB0'RGB0'RGB0'RGB0
算出块数可以用(num + 512u) / 1024u
相当于 (num + 512u) >> 10u
为了防止大小太小结果为0,可让结果与0x00'00'00'01 进行|运算
也就是((num + 512u) >> 10u) | 0x00'00'00'01
*/
