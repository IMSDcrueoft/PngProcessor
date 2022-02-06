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

#include"png.h"

bool ImageProcessingTools::Zoom_DefaultSampling4x4(PngData& input, PngData& result, const float32_t& magnification, const float32_t& CenterWeight,
	void (*WeightEffact)(const float32_t& dx, const float32_t& dy, floatVec4& weightResult))
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	result.width = input.width * magnification;
	result.height = input.height * magnification;

	float32_t scaleIndex = 1.0f / magnification;

	auto& resultRGBA = result.getRGBA_uint8();
	resultRGBA.resize(static_cast<size_t>(result.width) * result.height);

	float32_t center    = CenterWeight * 0.250f;
	float32_t outerNear = (1.0f - CenterWeight) * 0.0910628715f;
	float32_t outerFar  = (1.0f - CenterWeight) * 0.0678742569f;

	const float32_t kernel[4][4]
	{
		{outerFar ,outerNear,outerNear,outerFar },
		{outerNear,center	,center	  ,outerNear},
		{outerNear,center	,center	  ,outerNear},
		{outerFar ,outerNear,outerNear,outerFar }
	};

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height, [&result, &input, &WeightEffact,&kernel, &scaleIndex](uint32_t Y){
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < result.height; ++Y) {
#endif
		auto GetFloorIndex = [&scaleIndex](const uint32_t& index)
		{
			return static_cast<int32_t>(floorf(index * scaleIndex));
		};

#if !WINDOWS_SYSTEM_CPU_PARALLEL
		auto CalculateARowOfPixels = [&result, &input, &WeightEffact, &GetFloorIndex, &kernel, &scaleIndex](uint32_t Y)
		{
			if (Y >= result.height)return;
#endif
			auto Row = GetFloorIndex(Y);

			for (auto X = 0u; X < result.width; ++X)
			{

				auto Column = GetFloorIndex(X);

				float32_t dx = X * scaleIndex - Column;
				float32_t dy = Y * scaleIndex - Row;

				//Calculate weight parameters
				floatVec4 weight;
				WeightEffact(dx, dy, weight);

				RGBAColor_32f rgba_f1(0.0f, 0.0f, 0.0f, 0.0f);
				RGBAColor_32f rgba_f2(0.0f, 0.0f, 0.0f, 0.0f);
				RGBAColor_32f rgba_f3(0.0f, 0.0f, 0.0f, 0.0f);
				RGBAColor_32f rgba_f4(0.0f, 0.0f, 0.0f, 0.0f);

				//Unrolling loops to enhance performance

				rgba_f1 += RGBAColor_32f(input(Column + (-1), Row + (-1)), kernel[0][0]);
				rgba_f1 += RGBAColor_32f(input(Column + 0   , Row + (-1)), kernel[0][1]);
				rgba_f1 += RGBAColor_32f(input(Column + (-1), Row + 0)	 , kernel[1][0]);
				rgba_f1 += RGBAColor_32f(input(Column + 0   , Row + 0)	 , kernel[1][1]);

				rgba_f2 += RGBAColor_32f(input(Column + 1   , Row + (-1)), kernel[0][2]);
				rgba_f2 += RGBAColor_32f(input(Column + 2   , Row + (-1)), kernel[0][3]);
				rgba_f2 += RGBAColor_32f(input(Column + 1   , Row + 0)	 , kernel[1][2]);
				rgba_f2 += RGBAColor_32f(input(Column + 2   , Row + 0)	 , kernel[1][3]);

				rgba_f3 += RGBAColor_32f(input(Column + (-1), Row + 1), kernel[2][0]);
				rgba_f3 += RGBAColor_32f(input(Column + 0   , Row + 1), kernel[2][1]);
				rgba_f3 += RGBAColor_32f(input(Column + (-1), Row + 2), kernel[3][0]);
				rgba_f3 += RGBAColor_32f(input(Column + 0   , Row + 2), kernel[3][1]);

				rgba_f4 += RGBAColor_32f(input(Column + 1   , Row + 1), kernel[2][2]);
				rgba_f4 += RGBAColor_32f(input(Column + 2   , Row + 1), kernel[2][3]);
				rgba_f4 += RGBAColor_32f(input(Column + 1   , Row + 2), kernel[3][2]);
				rgba_f4 += RGBAColor_32f(input(Column + 2   , Row + 2), kernel[3][3]);

				rgba_f1 *= weight.X;
				rgba_f2 *= weight.Y;
				rgba_f3 *= weight.Z;
				rgba_f4 *= weight.W;

				rgba_f1 += rgba_f2;
				rgba_f1 += rgba_f3;
				rgba_f1 += rgba_f4;

				result(X, Y) = rgba_f1.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
		};

		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL
	return true;
}

bool ImageProcessingTools::Zoom_CubicConvolutionSampling4x4(PngData& input, PngData& result, const float32_t& magnification, const float32_t& a)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	result.width = input.width * magnification;
	result.height = input.height * magnification;

	float32_t scaleIndex = 1.0f / magnification;

	auto& resultRGBA = result.getRGBA_uint8();
	resultRGBA.resize(static_cast<size_t>(result.width) * result.height);

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height, [&result, &input,&a, &scaleIndex](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < result.height; ++Y) {
#endif

		auto GetFloorIndex = [&scaleIndex](const uint32_t& index)
		{
			return static_cast<int32_t>(floorf(index * scaleIndex));
		};

#if !WINDOWS_SYSTEM_CPU_PARALLEL
		auto CalculateARowOfPixels = [&result, &input,&a, &GetFloorIndex, &scaleIndex](uint32_t Y)
		{
			if (Y >= result.height)return;
#endif
			auto Formula = ImageProcessingTools::cubicConvolutionZoomFormula;

			auto Row = GetFloorIndex(Y);

			for (auto X = 0u; X < result.width; ++X)
			{

				auto Column = GetFloorIndex(X);

				float32_t dx = X * scaleIndex - Column;
				float32_t dy = Y * scaleIndex - Row;

				//Construct the weight matrix
				floatVec4 kernelX[4];
				floatVec4 kernelY[4];

				//Pay attention to the high and low
				kernelX[0] = floatVec4(Formula(a, 2.0f - dx),Formula(a, 1.0f - dx),Formula(a, 0.0f - dx),Formula(a, -1.0f - dx));
				kernelX[1] = kernelX[0];
				kernelX[2] = kernelX[0];
				kernelX[3] = kernelX[0];

				kernelY[0] = floatVec4(Formula(a, -1.0f - dy));
				kernelY[1] = floatVec4(Formula(a,  0.0f - dy));
				kernelY[2] = floatVec4(Formula(a,  1.0f - dy));
				kernelY[3] = floatVec4(Formula(a,  2.0f - dy));

				kernelX[0] *= kernelY[0];
				kernelX[1] *= kernelY[1];
				kernelX[2] *= kernelY[2];
				kernelX[3] *= kernelY[3];

				const auto& kernel = kernelX;

				// Unrolling loops to enhance performance
				RGBAColor_32f rgba_f(0.0f, 0.0f, 0.0f, 0.0f);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + (-1)), kernel[0][0]);
				rgba_f += RGBAColor_32f(input(Column + ( 0), Row + (-1)), kernel[0][1]);
				rgba_f += RGBAColor_32f(input(Column + ( 1), Row + (-1)), kernel[0][2]);
				rgba_f += RGBAColor_32f(input(Column + ( 2), Row + (-1)), kernel[0][3]);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + (0)), kernel[1][0]);
				rgba_f += RGBAColor_32f(input(Column + ( 0), Row + (0)), kernel[1][1]);
				rgba_f += RGBAColor_32f(input(Column + ( 1), Row + (0)), kernel[1][2]);
				rgba_f += RGBAColor_32f(input(Column + ( 2), Row + (0)), kernel[1][3]);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + (1)), kernel[2][0]);
				rgba_f += RGBAColor_32f(input(Column + ( 0), Row + (1)), kernel[2][1]);
				rgba_f += RGBAColor_32f(input(Column + ( 1), Row + (1)), kernel[2][2]);
				rgba_f += RGBAColor_32f(input(Column + ( 2), Row + (1)), kernel[2][3]);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + (2)), kernel[3][0]);
				rgba_f += RGBAColor_32f(input(Column + ( 0), Row + (2)), kernel[3][1]);
				rgba_f += RGBAColor_32f(input(Column + ( 1), Row + (2)), kernel[3][2]);
				rgba_f += RGBAColor_32f(input(Column + ( 2), Row + (2)), kernel[3][3]);

				result(X, Y) = rgba_f.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
};

		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL
	return true;
}

bool ImageProcessingTools::SharpenLaplace3x3(PngData& input, PngData& result,const float32_t& strength)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	result.width = input.width;
	result.height = input.height;

	auto& resultRGBA = result.getRGBA_uint8();
	resultRGBA.resize(static_cast<size_t>(result.width) * result.height);

	float32_t factor = -0.01f * strength;
	constexpr float32_t oneHalfRoot = 0.70710678f;
	constexpr float32_t oneHalfRootPlusOne = oneHalfRoot + 1.0f;
	
	/*
		kernel = 
		{
		  -sqrt(0.5) -1 -sqrt(0.5)
		  -1       -outers	-1
		  -sqrt(0.5) -1 -sqrt(0.5)
		}
	
	*/

	const float32_t& outerNear = factor;
	const float32_t outerFar = factor * oneHalfRoot;
	const float32_t center = 1.0f - (4.0f * oneHalfRootPlusOne) * factor;

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height,[&result, &input,&outerNear,&outerFar,&center](uint32_t Y){
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < result.height; ++Y) {

		auto CalculateARowOfPixels = [&result, &input, &outerNear, &outerFar, &center](uint32_t Y)
		{

			if (Y >= result.height)return;
#endif
			for (auto X = 0u; X < result.width; ++X)
			{

				RGBAColor_32f rgba_f1(0.0f, 0.0f, 0.0f, 0.0f);
				RGBAColor_32f rgba_f2(0.0f, 0.0f, 0.0f, 0.0f);

				rgba_f1 += RGBAColor_32f(input(X - 1, Y - 1));
				rgba_f1 += RGBAColor_32f(input(X + 1, Y - 1));
				rgba_f1 += RGBAColor_32f(input(X - 1, Y + 1));
				rgba_f1 += RGBAColor_32f(input(X + 1, Y + 1));

				rgba_f2 += RGBAColor_32f(input(X + 0, Y - 1));
				rgba_f2 += RGBAColor_32f(input(X - 1, Y + 0));
				rgba_f2 += RGBAColor_32f(input(X + 1, Y + 0));
				rgba_f2 += RGBAColor_32f(input(X + 0, Y + 1));

				rgba_f1 *= outerFar;
				rgba_f2 *= outerNear;

				rgba_f1 += RGBAColor_32f(input(X + 0, Y + 0), center);
				rgba_f1 += rgba_f2;

				result(X, Y) = rgba_f1.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
			});
#else
		};

		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();

		}
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL	
	return true;
}

bool ImageProcessingTools::SharpenGaussLaplace5x5(PngData& input, PngData& result, const float32_t& strength)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	result.width = input.width;
	result.height = input.height;

	auto& resultRGBA = result.getRGBA_uint8();
	resultRGBA.resize(static_cast<size_t>(result.width) * result.height);

	float32_t factor = -0.002f * strength;
	/*
		kernel = 
		{
		-2 -4 -4 -4 -2
		-4  0  8  0 -4
		-4  8  24 8 -4
		-4  0  8  0 -4
		-2 -4 -4 -4 -2
		}

		kernel =
		{
		-1 -2 -2 -2 -1
		-2  0  4  0 -2
		-2  4  12 4 -2
		-2  0  4  0 -2
		-1 -2 -2 -2 -1
		}
	*/
	
	const float32_t &outerl2Far = factor;
	const float32_t outerl2Near = 2.0f * factor;
	const float32_t outerl1Near = -4.0f * factor;
	//const float32_t outerl1Far = 0.0f;
	const float32_t center = 1.0f - 12.0f * factor;

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height, [&result, &input, &outerl2Far, &outerl2Near,&outerl1Near, &center](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < result.height; ++Y) {

		auto CalculateARowOfPixels = [&result, &input, &outerl2Far, &outerl2Near, &outerl1Near, &center](uint32_t Y)
		{

			if (Y >= result.height)return;
#endif

			for (auto X = 0u; X < result.width; ++X)
			{
				RGBAColor_32f rgba_f1(0.0f, 0.0f, 0.0f, 0.0f);
				RGBAColor_32f rgba_f2(0.0f, 0.0f, 0.0f, 0.0f);
				RGBAColor_32f rgba_f3(0.0f, 0.0f, 0.0f, 0.0f);

				rgba_f1 += RGBAColor_32f(input(X - 2, Y - 2));
				rgba_f1 += RGBAColor_32f(input(X + 2, Y - 2));
				rgba_f1 += RGBAColor_32f(input(X - 2, Y + 2));
				rgba_f1 += RGBAColor_32f(input(X + 2, Y + 2));

				rgba_f2 += RGBAColor_32f(input(X - 1, Y - 2));
				rgba_f2 += RGBAColor_32f(input(X + 0, Y - 2));
				rgba_f2 += RGBAColor_32f(input(X + 1, Y - 2));
				rgba_f2 += RGBAColor_32f(input(X - 2, Y - 1));
				rgba_f2 += RGBAColor_32f(input(X - 2, Y + 0));
				rgba_f2 += RGBAColor_32f(input(X - 2, Y + 1));
				rgba_f2 += RGBAColor_32f(input(X + 2, Y - 1));
				rgba_f2 += RGBAColor_32f(input(X + 2, Y + 0));
				rgba_f2 += RGBAColor_32f(input(X + 2, Y + 1));
				rgba_f2 += RGBAColor_32f(input(X - 1, Y + 2));
				rgba_f2 += RGBAColor_32f(input(X + 0, Y + 2));
				rgba_f2 += RGBAColor_32f(input(X + 1, Y + 2));

				rgba_f3 += RGBAColor_32f(input(X + 0, Y - 1));
				rgba_f3 += RGBAColor_32f(input(X - 1, Y + 0));
				rgba_f3 += RGBAColor_32f(input(X + 1, Y + 0));
				rgba_f3 += RGBAColor_32f(input(X + 0, Y + 1));

				rgba_f1 *= outerl2Far;
				rgba_f2 *= outerl2Near;
				rgba_f3 *= outerl1Near;

				rgba_f1 += RGBAColor_32f(input(X + 0, Y + 0), center);
				rgba_f1 += rgba_f2;
				rgba_f1 += rgba_f3;

				result(X, Y) = rgba_f1.toRGBAColor_8i();
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
};

		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();

		}
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL	
	return true;
}

bool ImageProcessingTools::AecsHdrToneMapping(PngData& inputOutput, const float32_t& lumRatio)
{
	if (inputOutput.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	inputOutput.clearImage();

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, inputOutput.height, [&inputOutput, &lumRatio](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < inputOutput.height; ++Y) {

		auto CalculateARowOfPixels = [&inputOutput, &lumRatio](uint32_t Y) {
#endif
			for (auto X = 0u; X < inputOutput.width; ++X)
			{
				RGBAColor_32f rgba_f = RGBAColor_32f(inputOutput(X, Y));

				ImageProcessingTools::ACESToneMappingColor(rgba_f, lumRatio);

				inputOutput(X, Y) = rgba_f.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
		};

		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::ReverseColorImage(PngData& inputOutput)
{
	if (inputOutput.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	inputOutput.clearImage();

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, inputOutput.height, [&inputOutput](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < inputOutput.height; ++Y) {

		auto CalculateARowOfPixels = [&inputOutput](uint32_t Y) {
#endif
			for (auto X = 0u; X < inputOutput.width; ++X)
			{
				ReverseColor(inputOutput(X, Y));
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
	};
		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::Grayscale(PngData& input, PngData& result)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	input.clearImage();

	result.width = input.width;
	result.height = input.height;
	result.image.resize(input.getRGBA_uint8().size());

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, input.height, [&input,&result](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < input.height; ++Y) {

		auto CalculateARowOfPixels = [&input,&result](uint32_t Y) {
#endif
			for (auto X = 0u; X < input.width; ++X)
			{
				ImageProcessingTools::GrayColor(input(X, Y), result.image[static_cast<size_t>(input.width) * Y + X]);
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
};
		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::ChannelGrayScale(PngData& input, PngData& resultR, PngData& resultG, PngData& resultB)
{
	if (input.image.size() == 0)
		return false;

	resultR.width = input.width;
	resultR.height = input.height;
	resultG.width = input.width;
	resultG.height = input.height;
	resultB.width = input.width;
	resultB.height = input.height;

	resultR.image.resize(input.image.size() >> 2);
	resultG.image.resize(input.image.size() >> 2);
	resultB.image.resize(input.image.size() >> 2);

	for (size_t i = 0; i < input.image.size(); i += 4)
	{
#if LITTLE_ENDIAN
		resultR.image[i >> 2] = input.image[i];
		resultG.image[i >> 2] = input.image[i + 1];
		resultB.image[i >> 2] = input.image[i + 2];

#else
		resultB.image[i >> 2] = input.image[i + 1];
		resultG.image[i >> 2] = input.image[i + 2];
		resultR.image[i >> 2] = input.image[i + 3];
#endif
	}

	return true;
}

bool ImageProcessingTools::VividnessAdjustment(PngData& inputOutput, const float32_t& vividRatio)
{
	if (inputOutput.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	inputOutput.clearImage();

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, inputOutput.height, [&inputOutput,&vividRatio](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < inputOutput.height; ++Y) {

		auto CalculateARowOfPixels = [&inputOutput,&vividRatio](uint32_t Y) {
#endif
			for (auto X = 0u; X < inputOutput.width; ++X)
			{
				RGBAColor_32f color(inputOutput(X, Y));

				ImageProcessingTools::VividnessAdjustmentColor(color,vividRatio);

				inputOutput(X, Y) = color.toRGBAColor_8i();
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
	};
		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::Binarization(PngData& input, PngData& result, const float32_t& threshold)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	input.clearImage();

	result.width = input.width;
	result.height = input.height;
	result.image.resize(input.getRGBA_uint8().size());

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, input.height, [&input,&result,&threshold](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < input.height; ++Y) {

		auto CalculateARowOfPixels = [&input,&result,&threshold](uint32_t Y) {
#endif
			for (auto X = 0u; X < input.width; ++X)
			{
				ImageProcessingTools::BinarizationColor(input(X, Y), threshold, result.image[static_cast<size_t>(input.width) * Y + X]);
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
	};
		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::Quaternization(PngData& input, PngData& result, const float32_t& threshold)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	input.clearImage();

	result.width = input.width;
	result.height = input.height;
	result.image.resize(input.getRGBA_uint8().size());

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, input.height, [&input, &result, &threshold](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < input.height; ++Y) {

		auto CalculateARowOfPixels = [&input, &result, &threshold](uint32_t Y) {
#endif
			for (auto X = 0u; X < input.width; ++X)
			{
				ImageProcessingTools::QuaternizationColor(input(X, Y), threshold, result.image[static_cast<size_t>(input.width) * Y + X]);
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
	};
		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::Hexadecimalization(PngData& input, PngData& result)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	//not need this time
	input.clearImage();

	result.width = input.width;
	result.height = input.height;
	result.image.resize(input.getRGBA_uint8().size());

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, input.height, [&input, &result](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < input.height; ++Y) {

		auto CalculateARowOfPixels = [&input, &result](uint32_t Y) {
#endif
			for (auto X = 0u; X < input.width; ++X)
			{
				ImageProcessingTools::HexadecimalizationColor(input(X, Y), result.image[static_cast<size_t>(input.width) * Y + X]);
			}

#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
	};
		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();
	}
#endif
	return true;
}

bool ImageProcessingTools::SurfaceBlur(PngData& input, PngData& result, const int32_t& radius, const float32_t& threshold)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	result.width = input.width;
	result.height = input.height;

	auto& resultRGBA = result.getRGBA_uint8();
	resultRGBA.resize(input.getRGBA_uint8().size());

	uint32_t sideLength = (radius << 1u) + 1u;
	float32_t denominator = 0.40f / threshold;

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height, [&result, &input, &radius, &denominator,&sideLength](uint32_t Y) {
#else
	CppParallelAccelerator accelerator;
	std::queue<uint32_t> param;

	for (auto Y = 0u; Y < result.height; ++Y) {

		auto CalculateARowOfPixels = [&result, &input, &radius, &denominator, &sideLength](uint32_t Y)
		{

			if (Y >= result.height)return;
#endif
			auto toGray = [](const RGBAColor_32f& rgba_f)
			{
				return fabsf((rgba_f.R + rgba_f.G + rgba_f.B) * 0.33333f);
			};

			for (auto X = 0u; X < result.width; ++X)
			{
				//tectonic kernel
				std::vector <std::vector<float32_t>>kernel(sideLength);
				for (auto& line : kernel)
					line.resize(sideLength);

				//buildKernel
				RGBAColor_32f center = RGBAColor_32f(input(X, Y));

				float32_t sum = 0.0f;
				RGBAColor_32f pixelSum(0.0f, 0.0f, 0.0f, 0.0f);

				for (int64_t h = -radius; h <= radius; ++h)
				{
					for (int64_t w = -radius; w <= radius; ++w)
					{
						RGBAColor_32f pixel = RGBAColor_32f(input(X + w, Y + h));
						pixel -= center;

						 kernel[h + radius][w + radius] = 1.0f - (toGray(pixel) * denominator);

						 sum += kernel[h + radius][w + radius];
					}
				}

				for (int64_t h = -radius; h <= radius; ++h)
				{
					for (int64_t w = -radius; w <= radius; ++w)
					{
						pixelSum += RGBAColor_32f(input(X + w, Y + h), kernel[h + radius][w + radius]);
					}
				}

				pixelSum /= sum;
				pixelSum.A = center.A;

				result(X, Y) = pixelSum.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
};

		// numberOfExecutionThreads threads
		for (auto i = 0; i < accelerator.GetNumThreads(); i++)
		{
			param.push(Y + i);
		}
		Y += (accelerator.GetNumThreads() - 1);

		accelerator.Run(CalculateARowOfPixels, param);
		accelerator.Join();

		}
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL	
	return true;
}

void ImageProcessingTools::importFile(PngData& data, std::filesystem::path& pngfile)
{
	auto path = AdaptString::toString(pngfile.wstring());

	uint32_t error = lodepng::decode(data.image, data.width, data.height, path);

	//if there's an error, display it
	if (error)
	{
		std::cout << "Decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::exportFile(PngData& result, std::wstring& resultname,const LodePNGColorType& colorType,const uint32_t& bitdepth)
{
	auto path = AdaptString::toString(resultname);

	if ((static_cast<size_t>(result.width) * result.height * 4) >= 0xFF'FF'FF'FF)
	{
		assert(false && "Byte size is bigger than UINT32,need cut.");
		std::cout << "Byte size is bigger than UINT32,need cut." << std::endl;
	}
	else
	{
		uint32_t error = lodepng::encode(path, result.image, result.width, result.height,colorType,bitdepth);

		if (error)
		{
			std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			exit(0);
		}

		std::cout << "Result filename:" << path << '\n' << std::endl;
	}
}

void ImageProcessingTools::exportFile(const byte* result, const uint32_t& width, const uint32_t& height, std::wstring& resultname, const LodePNGColorType& colorType, const uint32_t& bitdepth)
{
	auto path = AdaptString::toString(resultname);

	if ((static_cast<size_t>(width) * height * 4) >= 0xFF'FF'FF'FF)
	{
		assert(false && "Byte size is bigger than UINT32,need cut.");
		std::cout << "Byte size is bigger than UINT32,need cut." << std::endl;
	}
	else
	{
		uint32_t error = lodepng::encode(path, result, width, height, colorType, bitdepth);

		if (error)
		{
			std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			exit(0);
		}

		std::cout << "Result filename:" << path << '\n' << std::endl;
	}
}

void ImageProcessingTools::help()
{
	std::cout << "Check Help Info.\n\n"
		<< "Help:[---] is a prompt, not an input.[DF] means it has a default value.\n"
		<< "Startup parameters-->\n"
		<< "[    Default Zoom    ]: z     \n"
		<< "[    Bicubic Zoom    ]: Z     \n"
		<< "[   Laplace Sharpen  ]: s     \n"
		<< "[GaussLaplace Sharpen]: S     \n"
		<< "[    Tone Mapping    ]: t or T\n"
		<< "[     Gray Scale     ]: g     \n"
		<< "[ Channle Gray Scale ]: G     \n"
		<< "[ Surface Blur Filter]: F     \n"
		<< "[    Reverse Color   ]: r or R\n"
		<< "[    Binarization    ]: b or B\n"
		<< "[   Quaternization   ]: q or Q\n"
		<< "[ Hexadecimalization ]: h or H\n"
		<< "[Vividness Adjustment]: v or V\n"
		<< "[      Block Cut     ]: c     \n"
		<< "[     Cut horizon    ]: C     \n"
		<< "[       Mosaic       ]: m       [Feature not currently supported]\n"
		<< '\n'
		<< "Input Sample-->\n"
		<< "./pngProcessor.exe filename.png z[default zoom] 1.0[zoom ratio:DF] 0.5[center weight:DF] 2[Exponent:DF]\n"
		<< "[default zoom]\n"
		<< "[zoom ratio(from 0.001 to 32.0)]\n"
		<< "[center weight(from 0.25 to 13.0, 0.25:similar to MSAAx16, 1.0 : similar to bilinear, >1:sharp)]\n"
		<< "[Exponent(from 1 to 4:0.5, 1.0, 2.0, 4.0)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png z[bicubic zoom] -1.0[formula factor:DF]\n"
		<< "[bicubic zoom]\n"
		<< "[zoom ratio(from 0.001 to 32.0)]\n"
		<< "[formula factor(from -3.0 to -0.1)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png t[tone mapping] 2.0[lumming ratio:DF]\n"
		<< "[tone mapping]\n"
		<< "[lumming ratio(from 0.1 to 16.0)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png g[gray scale]\n"
		<< "[gray scale]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png G[channle gray scale]\n"
		<< "[channle gray scale]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png b[binarization] 0.5[binarization threshold:DF]\n"
		<< "[binarization]\n"
		<< "[binarization threshold(from 0 to 1-1/255)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png q[quaternization] 0.5[quaternization threshold:DF]\n"
		<< "[quaternization]\n"
		<< "[quaternization threshold(from 0 to 1]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png h[hexadecimalization]\n"
		<< "[hexadecimalization]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png v[vividness Adjustment] 0.2[vivid ratio:DF]\n"
		<< "[vividness Adjustment]\n"
		<< "[vivid ratio(from -1.0 to 254.0)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png r[reverse color]\n"
		<< "[reverse color]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png s[laplace sharpen] 4.0[sharpen ratio:DF]\n"
		<< "[laplace sharpen]\n"
		<< "[sharpen ratio(from 1 to 1000)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png S[gauss-laplace sharpen] 4.0[sharpen ratio:DF]\n"
		<< "[gauss-laplace sharpen]\n"
		<< "[sharpen ratio(from 1 to 1000)]\n"
		<< '\n'
		<< "./pngProcessor.exe filename.png C[cut horizon] 1024[Vertical Interval:DF]\n"
		<< "[cut horizon]\n"
		<< "[Vertical Interval(>0)]"
		<< '\n'
		<< "./pngProcessor.exe filename.png c[cut] 1024[Horizontal Interval:DF] 1024[Vertical Interval:DF]\n"
		<< "[cut]\n"
		<< "[Horizontal Interval(>0)]\n"
		<< "[Vertical Interval(>0)]\n"
		<< std::endl;
}

void ImageProcessingTools::commandStartUps(int32_t argCount, STR argValues[])
{
	std::filesystem::path pngfile;
	char mode = (char)Mode::unknown;
	clockTimer timer;
	std::istringstream iss;

	timer.TimerStart();

	if (argCount == 1)
	{
		std::cout << "No image or parameters entered!\n";
		help();
		exit(0);
	}
	else
		if (argCount > 1)
		{
			pngfile = argValues[1];
			std::cout << "Input filename:" << pngfile << '\n' << std::endl;
		}

	if (argCount == 2)
	{
		std::cout << "Too few parameters.\n";
		help();
		exit(0);
	}

	iss.str(argValues[2]);
	iss >> mode;

	float32_t Ratio = 1.0f;
	float32_t Weight = 0.64f;
	uint32_t exponent = (uint32_t)ImageProcessingTools::Exponent::square;

	uint32_t interval_horizontal = 1024u;
	uint32_t interval_vertical = 1024u;

	int32_t radius = 1;

	switch (mode)
	{
	case (int)Mode::zoom:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;

			if (argCount > 4)
			{
				iss.clear();
				iss.str(argValues[4]);
				iss >> Weight;

				if (argCount > 5)
				{
					iss.clear();
					iss.str(argValues[5]);
					iss >> exponent;
				}
			}
		}

		std::cout << "Input exponent factor:" << exponent << '\n';
		Clamp(exponent, 1, 4);
		std::cout << "Adoption exponent factor:";
		
		if (exponent == 1)
		{
			std::cout << "Half\n";
		}
		else
			if (exponent == 2)
			{
				std::cout << "One\n";
			}
			else
				if (exponent == 3)
				{
					std::cout << "Square\n";
				}
				else
					if (exponent == 4)
					{
						std::cout << "Quartet\n";
					}

		ImageProcessingTools::zoomProgramDefault(Ratio, pngfile, Weight, (ImageProcessingTools::Exponent)exponent);
		break;
	case (int)Mode::Zoom:
		Weight = -0.5f;
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;

			if (argCount > 4)
			{
				iss.clear();
				iss.str(argValues[4]);
				iss >> Weight;
			}
		}
		ImageProcessingTools::zoomProgramCubicConvolution(Ratio, pngfile, Weight);
		break;

	case (int)Mode::sharpen:
		Ratio = 15.0f;
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::laplaceSharpenProgram(Ratio, pngfile);
		break;

	case (int)Mode::Sharpen:
		Ratio = 15.0f;
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::gaussLaplaceSharpenProgram(Ratio, pngfile);
		break;

	case (int)Mode::toneMapping:
	case (int)Mode::ToneMapping:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::hdrToneMappingColorProgram(Ratio, pngfile);
		break;

	case (int)Mode::grayScale:
		ImageProcessingTools::grayColorProgram(pngfile);
		break;

	case (int)Mode::GrayScale:
		ImageProcessingTools::channelGrayColorProgram(pngfile);
		break;

	case (int)Mode::reverseColor:
	case (int)Mode::ReverseColor:
		ImageProcessingTools::reverseColorProgram(pngfile);
		break;

	case (int)Mode::vividness:
	case (int)Mode::Vividness:
		Ratio = 0.2f;

		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}

		ImageProcessingTools::vividnessAdjustmentColorProgram(Ratio, pngfile);
		break;

	case (int)Mode::binarization:
	case (int)Mode::Binarization:
		Ratio = 0.5f;

		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::binarizationColorProgram(Ratio, pngfile);
		break;

	case (int)Mode::quaternization:
	case (int)Mode::Quaternization:
		Ratio = 0.5f;

		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::quaternizationColorProgram(Ratio,pngfile);
		break;

	case (int)Mode::hexadecimalization:
	case (int)Mode::Hexadecimalization:
		ImageProcessingTools::hexadecimalizationColorProgram(pngfile);
		break;

	case (int)Mode::cut:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> interval_horizontal;

			if (argCount > 4)
			{
				iss.clear();
				iss.str(argValues[4]);
				iss >> interval_vertical;
			}
		}
		ImageProcessingTools::blockSplitProgram(interval_horizontal, interval_vertical, pngfile);
		break;

	case (int)Mode::Cut:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> interval_vertical;
		}
		ImageProcessingTools::fastSplitHorizonProgram(interval_vertical, pngfile);
		break;

	case (int)Mode::Filter:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;

			if (argCount > 4)
			{
				iss.clear();
				iss.str(argValues[4]);
				iss >> radius;
			}
		}
		ImageProcessingTools::surfaceBlurfilterProgram(Ratio, pngfile, radius);
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
}

void ImageProcessingTools::zoomProgramDefault(float32_t& zoomRatio, std::filesystem::path& pngfile,float32_t& CenterWeight,const Exponent& exponent)
{
	std::cout << "Zoom Default:\n"
		<<"Input zoom factor:" << zoomRatio << '\n';

	Clamp(zoomRatio, 0.001f, 32.0f);
	std::cout << "Adoption zoom factor:" << zoomRatio << '\n';

	std::cout<< "Input center weight:" << CenterWeight << '\n';

	Clamp(CenterWeight, 0.25f, 13.0f);

	std::cout << "Adoption center weight:" << CenterWeight << '\n'
		<< "Start processing . . ." << std::endl;


	PngData image, result;
	importFile(image, pngfile);


	void (*WeightEffact)(const float32_t&, const float32_t&, floatVec4&) = nullptr;

	if (exponent == Exponent::half)
	{
		WeightEffact = ImageProcessingTools::weightEffectHalf;
	}
	else
		if (exponent == Exponent::one)
		{
			WeightEffact = ImageProcessingTools::weightEffectOne;
		}
		else
			if (exponent == Exponent::quartet)
			{
				WeightEffact = ImageProcessingTools::weightEffectQuartet;
			}
			else
			{
				WeightEffact = ImageProcessingTools::weightEffectSquare;
			}

	if (ImageProcessingTools::Zoom_DefaultSampling4x4(image, result, zoomRatio, CenterWeight, WeightEffact))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_zoom_x").append(std::to_wstring(zoomRatio))
			.append(L"_centerWeight_").append(std::to_wstring(CenterWeight))
			.append(L"_Exponent_Mode_").append(std::to_wstring((uint32_t)exponent))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);

#else
		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::zoomProgramCubicConvolution(float32_t& zoomRatio, std::filesystem::path& pngfile, float32_t& a)
{
	std::cout << "Zoom Cubic:\n"
		<< "Input zoom factor:" << zoomRatio << '\n';

	Clamp(zoomRatio, 0.001f, 32.0f);
	std::cout << "Adoption zoom factor:" << zoomRatio << '\n';

	std::cout << "Input formula factor:" << a << '\n';

	Clamp(a, -3.0f, -0.1f);

	std::cout << "Adoption formula factor:" << a << '\n'
		<< "Start processing . . ." << std::endl;


	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::Zoom_CubicConvolutionSampling4x4(image, result, zoomRatio, a))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_Zoom_x").append(std::to_wstring(zoomRatio))
			.append(L"_cubicFactor_").append(std::to_wstring(a))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);

#else
		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::laplaceSharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile)
{
	std::cout << "Laplace Sharpen:\n" 
		<< "Input sharpen factor:" << sharpenRatio << '\n' << std::endl;

	Clamp(sharpenRatio, 1.0f, 1000.0f);

	std::cout << "Adoption sharpen factor:" << sharpenRatio << "%\n"
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::SharpenLaplace3x3(image, result, sharpenRatio))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_L_sharpen_x").append(std::to_wstring(sharpenRatio))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);

#else
		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::gaussLaplaceSharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile)
{
	std::cout << "Gauss-Laplace Sharpen:\n"
		<< "Input sharpen factor:" << sharpenRatio << '\n' << std::endl;

	Clamp(sharpenRatio, 1.0f, 1000.0f);

	std::cout << "Adoption sharpen factor:" << sharpenRatio << "%\n"
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::SharpenGaussLaplace5x5(image, result, sharpenRatio))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_GL_sharpen_x").append(std::to_wstring(sharpenRatio))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);

#else
		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::hdrToneMappingColorProgram(float32_t& lumRatio, std::filesystem::path& pngfile)
{
	std::cout << "ToneMapping:\n"
		<< "Input lumming factor:" << lumRatio << '\n' << std::endl;

	Clamp(lumRatio, 0.1f, 16.0f);

	std::cout << "Adoption lumming factor:" << lumRatio << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	if (ImageProcessingTools::AecsHdrToneMapping(image, lumRatio))
	{
		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_toneMapping_x").append(std::to_wstring(lumRatio))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(image.getRGBA_uint8().data()), image.width, image.height, resultname);
#else
		//load result into stream to save to file
		image.loadRGBAtoByteStream();
		image.clearRGBA_uint8();

		exportFile(image, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::reverseColorProgram(std::filesystem::path& pngfile)
{
	std::cout << "ReverseColor:\n"
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	if (ImageProcessingTools::ReverseColorImage(image))
	{
		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_reverse")
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(image.getRGBA_uint8().data()), image.width, image.height, resultname);
#else
		//load result into stream to save to file
		image.loadRGBAtoByteStream();
		image.clearRGBA_uint8();

		exportFile(image, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::grayColorProgram(std::filesystem::path& pngfile)
{
	std::cout << "Grayscale:\n"
		<< "Start processing . . ." << std::endl;

	PngData image,result;
	importFile(image, pngfile);

	if (ImageProcessingTools::Grayscale(image,result))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_gray")
			.append(pngfile.extension());

		exportFile(result.image.data(), result.width, result.height, resultname, LodePNGColorType::LCT_GREY);
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::channelGrayColorProgram(std::filesystem::path& pngfile)
{
	std::cout << "ChannelGrayscale:\n"
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	PngData imageR;
	PngData imageG;
	PngData imageB;

	if (ImageProcessingTools::ChannelGrayScale(image, imageR, imageG, imageB))
	{
		image.clearImage();

		std::wstring resultnameR;
		std::wstring resultnameG;
		std::wstring resultnameB;

		resultnameR.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_Red_gray")
			.append(pngfile.extension());

		resultnameG.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_Green_gray")
			.append(pngfile.extension());

		resultnameB.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_Blue_gray")
			.append(pngfile.extension());

		auto exportChannel = [](PngData *result, std::wstring *resultname) {
			ImageProcessingTools::exportFile(result->image.data(), result->width, result->height, *resultname, LodePNGColorType::LCT_GREY);
		};

		std::thread RedChannel(exportChannel, &imageR, &resultnameR);
		std::thread GreenChannel(exportChannel, &imageG, &resultnameG);
		std::thread BlueChannel(exportChannel, &imageB, &resultnameB);

		if (RedChannel.joinable())
			RedChannel.join();

		if (GreenChannel.joinable())
			GreenChannel.join();

		if (BlueChannel.joinable())
			BlueChannel.join();
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::vividnessAdjustmentColorProgram(float32_t& VividRatio, std::filesystem::path& pngfile)
{
	std::cout << "VividnessAdjustment:\n"
		<<"Input Vivid factor:" << VividRatio << '\n' << std::endl;

	Clamp(VividRatio, -1.0f, 254.0f);


	std::cout << "Adoption Vivid factor:" << VividRatio << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	if (ImageProcessingTools::VividnessAdjustment(image,VividRatio))
	{
		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_vivid_x").append(std::to_wstring(VividRatio))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(image.getRGBA_uint8().data()), image.width, image.height, resultname);
#else
		//load result into stream to save to file
		image.loadRGBAtoByteStream();
		image.clearRGBA_uint8();

		exportFile(image, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::binarizationColorProgram(float32_t& threshold, std::filesystem::path& pngfile)
{
	std::cout << "Binarization:\n"
		<<"Input threshold factor:" << threshold << '\n' << std::endl;

	Clamp(threshold, 0.0f, 1.0f - ColorPixTofloat);

	std::cout << "Adoption threshold factor:" << threshold << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::Binarization(image, result, threshold))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_binarization_").append(std::to_wstring(threshold))
			.append(pngfile.extension());

		exportFile(result.image.data(), result.width, result.height, resultname, LodePNGColorType::LCT_GREY);
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::quaternizationColorProgram(float32_t& threshold,std::filesystem::path& pngfile)
{
	std::cout << "Quaternization:\n"
		<<"Input threshold factor:" << threshold << '\n' << std::endl;

	Clamp(threshold, 0.0f, 1.0f);

	std::cout << "Adoption threshold factor:" << threshold << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::Quaternization(image, result,threshold))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_quaternization_").append(std::to_wstring(threshold))
			.append(pngfile.extension());

		exportFile(result.image.data(), result.width, result.height, resultname, LodePNGColorType::LCT_GREY);
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::hexadecimalizationColorProgram(std::filesystem::path& pngfile)
{
	std::cout << "Hexadecimalization:\n"
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::Hexadecimalization(image, result))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_hexadecimalization")
			.append(pngfile.extension());

		exportFile(result.image.data(), result.width, result.height, resultname, LodePNGColorType::LCT_GREY);
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::fastSplitHorizonProgram(uint32_t& splitInterval, std::filesystem::path& pngfile)
{
	std::cout << "FastSplitHorizon:\n"
		<< "Input split interval factor:" << splitInterval << '\n' << std::endl;

	if (splitInterval == 0u)
		splitInterval = 128u;

	std::cout << "Adoption split interval factor:" << splitInterval << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	if (image.image.size() != 0u)
	{
		uint32_t splitNum = std::ceil(float64_t(image.height) / float64_t(splitInterval));

		if (splitNum == 1u)
		{
			std::cout << "Wrong size, cannot be split." << std::endl;
			exit(0);
		}

		size_t byteSplitInterval = (static_cast<size_t>(image.width) * splitInterval) << 2;

		std::vector<std::unique_ptr<std::thread>> allthreads;
		
		allthreads.reserve(splitNum);

		auto exportSplitSlice = [&image](const byte* result, uint32_t height, std::wstring resultName)
		{
			ImageProcessingTools::exportFile(result, image.width, height, resultName);
		};

		std::wstring resultnamepart;

		resultnamepart.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_splitHorizon_slice_");

		for (size_t Current = 0u, currentSlice = 1u; Current < image.image.size(); Current += byteSplitInterval, ++currentSlice)
		{
			std::wstring thisName;
			thisName.append(resultnamepart).append(std::to_wstring(currentSlice)).append(pngfile.extension());

			if ((Current + byteSplitInterval) < image.image.size())
			{
				allthreads.push_back(std::make_unique<std::thread>(exportSplitSlice, image.image.data() + Current, splitInterval, thisName));
			}
			else
				if ((image.image.size() - Current) > 0u)
				{
					allthreads.push_back(std::make_unique<std::thread>(exportSplitSlice, image.image.data() + Current, image.height % splitInterval, thisName));
				}
		}

		for (auto& thread : allthreads)
		{
			if (thread->joinable())
				thread->join();
		}
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::blockSplitProgram(uint32_t& horizontalInterval, uint32_t& verticalInterval, std::filesystem::path& pngfile)
{
	std::cout << "BlockSplit:\n"
		<< "Input horizontal interval factor:" << horizontalInterval << '\n'
		<< "Input  vertical  interval factor:" << verticalInterval << '\n' << std::endl;

	if (horizontalInterval == 0u)
		horizontalInterval = 128u;
	if (verticalInterval == 0u)
		verticalInterval = 128u;

	std::cout << "Adoption horizontal interval factor:" << horizontalInterval << '\n'
		<< "Adoption vertical interval factor:" << horizontalInterval << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	if (image.image.size() != 0u)
	{
		uint32_t horizontalSplitNum = std::ceil(float64_t(image.width) / float64_t(horizontalInterval));
		uint32_t verticalSplitNum = std::ceil(float64_t(image.height) / float64_t(verticalInterval));

		if (horizontalSplitNum == 1u && verticalSplitNum == 1u)
		{
			std::cout << "Wrong size, cannot be split." << std::endl;
			exit(0);
		}

		std::vector<uint32_t>widths(horizontalSplitNum);
		std::vector<uint32_t>heights(verticalSplitNum);

		for (auto& w : widths)
			w = horizontalInterval;

		for (auto& h : heights)
			h = verticalInterval;

		if ((image.width % horizontalInterval) > 0u)
			widths.back() = image.width % horizontalInterval;
		if ((image.height % verticalInterval) > 0u)
			heights.back() = image.height % verticalInterval;

		std::vector<std::vector<byte>>LineBlocks(horizontalSplitNum);
		//to ensure space is enough
		for (auto& blocks : LineBlocks)
			blocks.reserve(static_cast<size_t>(horizontalInterval) * verticalInterval << 2u);

		std::vector<std::unique_ptr<std::thread>> allthreads;

		allthreads.resize(horizontalSplitNum);

		size_t byteOffset = 0u;

		auto exportSplitSlice = [](const byte* result, uint32_t width, uint32_t height, std::wstring resultName)
		{
			ImageProcessingTools::exportFile(result, width, height, resultName);
		};

		std::wstring resultnamepart;

		resultnamepart.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_slice_");

		const auto& data = image.image.data();

		for (uint32_t y = 0u; y < verticalSplitNum; ++y)
		{
			for (uint32_t i = 0u; i < heights[y]; ++i)
			{
				for (uint32_t x = 0u; x < horizontalSplitNum; ++x)
				{
					size_t size = LineBlocks[x].size();
					LineBlocks[x].resize(size + (static_cast<size_t>(widths[x]) << 2u));

					std::copy((data + byteOffset), (data + byteOffset) + (static_cast<size_t>(widths[x]) << 2u), LineBlocks[x].begin() + size);
					byteOffset += (static_cast<size_t>(widths[x]) << 2u);
				}
			}

			for (uint32_t x = 0u; x < horizontalSplitNum; ++x)
			{
				std::wstring thisName;
				thisName.append(resultnamepart)
					.append(L"V").append(std::to_wstring(y + 1)).append(L"_")
					.append(L"H").append(std::to_wstring(x + 1)).append(pngfile.extension());

				allthreads[x] = std::make_unique<std::thread>(exportSplitSlice, LineBlocks[x].data(), widths[x], heights[y], thisName);
			}

			for (auto& thread : allthreads)
			{
				if (thread->joinable())
					thread->join();
			}

			for (auto& blocks : LineBlocks)
				blocks.clear();
		}
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::surfaceBlurfilterProgram(float32_t& threshold, std::filesystem::path& pngfile, int32_t& radius)
{
	std::cout << "Surface Blur Filter:\n"
		<< "Input blur factor:" << threshold << '\n' << std::endl;

	Clamp(threshold, ColorPixTofloat, 1.0f);

	std::cout << "Adoption blur factor:" << threshold << '\n';

	std::cout<< "Input radius factor:" << radius << '\n' << std::endl;

	Clamp(radius, 1u, 12u);

	std::cout << "Adoption radius factor:" << radius << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::SurfaceBlur(image, result,radius,threshold))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"/").append(pngfile.stem())
			.append(L"_surfaceBlur_").append(std::to_wstring(threshold))
			.append(L"_radius_").append(std::to_wstring(radius))
			.append(pngfile.extension());

#if LITTLE_ENDIAN
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);

#else
		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in convert." << std::endl;
		exit(0);
	}
}
