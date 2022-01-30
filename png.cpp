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

	float32_t center = CenterWeight / 4.0f;
	float32_t outer = (1.0f - CenterWeight) / 12.0f;

	const float32_t kernel[4][4] =
	{
		{outer,outer,outer,outer},
		{outer,center,center,outer},
		{outer,center,center,outer},
		{outer,outer,outer,outer}
	};

#if !WINDOWS_SYSTEM_CPU_PARALLEL
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 6) ? numberOfExecutionThreads : 6;
#endif

	for (auto Y = 0u; Y < result.height; ++Y)
	{
		auto GetFloorIndex = [&scaleIndex](const uint32_t& index)
		{
			return static_cast<int32_t>(floorf(index * scaleIndex));
		};

#if WINDOWS_SYSTEM_CPU_PARALLEL
		auto Row = GetFloorIndex(Y);

		concurrency::parallel_for(0u, result.width, [&result, &input,&WeightEffact, &GetFloorIndex, &Row, &kernel, &scaleIndex, Y](uint32_t X)
			{
#else
		auto CalculateARowOfPixels = [&result, &input, &WeightEffact, &GetFloorIndex, &kernel, &scaleIndex](uint32_t Row, uint32_t Y)
		{

			if (Y >= result.height)return;

			for (auto X = 0u; X < result.width; ++X)
			{
#endif
				auto Column = GetFloorIndex(X);

				float32_t dx = X * scaleIndex - Column;
				float32_t dy = Y * scaleIndex - Row;

				//Calculate weight parameters
				floatVec4 weight;
				WeightEffact(dx, dy, weight);

				RGBAColor_32f rgba_f(0.0f, 0.0f, 0.0f, 0.0f);

				//Unrolling loops to enhance performance

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + (-1))) * (kernel[0][0] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 0   , Row + (-1))) * (kernel[0][1] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 1   , Row + (-1))) * (kernel[0][2] * weight.Y);
				rgba_f += RGBAColor_32f(input(Column + 2   , Row + (-1))) * (kernel[0][3] * weight.Y);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + 0)) * (kernel[1][0] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 0   , Row + 0)) * (kernel[1][1] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 1   , Row + 0)) * (kernel[1][2] * weight.Y);
				rgba_f += RGBAColor_32f(input(Column + 2   , Row + 0)) * (kernel[1][3] * weight.Y);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + 1)) * (kernel[2][0] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 0   , Row + 1)) * (kernel[2][1] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 1   , Row + 1)) * (kernel[2][2] * weight.W);
				rgba_f += RGBAColor_32f(input(Column + 2   , Row + 1)) * (kernel[2][3] * weight.W);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + 2)) * (kernel[3][0] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 0   , Row + 2)) * (kernel[3][1] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 1   , Row + 2)) * (kernel[3][2] * weight.W);
				rgba_f += RGBAColor_32f(input(Column + 2   , Row + 2)) * (kernel[3][3] * weight.W);

				result(X, Y) = rgba_f.toRGBAColor_8i();

#if WINDOWS_SYSTEM_CPU_PARALLEL
			});
#else
			}
		};
		// numberOfExecutionThreads threads
		static std::vector<uint32_t> Row(numberOfExecutionThreads);

		for (auto i = 0; i < numberOfExecutionThreads; ++i)
		{
			Row[i] = GetFloorIndex(Y + i);
		}

		static std::vector<std::unique_ptr<std::thread>> allThreads(numberOfExecutionThreads);

		for (auto i = 0; i < numberOfExecutionThreads; ++i)
		{
			allThreads[i] = std::make_unique<std::thread>(CalculateARowOfPixels, Row[i], Y + i);
		}

		Y += (numberOfExecutionThreads - 1);

		for (auto& ptr : allThreads)
			ptr->join();
		
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL	
	}
	return true;
}

bool ImageProcessingTools::Sharpen3x3(PngData& input, PngData& result,const float32_t& strength)
{
	if (input.getRGBA_uint8().size() == 0)//Handle it well, otherwise there will be problems in parallel
		return false;

	result.width = input.width;
	result.height = input.height;

	auto& resultRGBA = result.getRGBA_uint8();
	resultRGBA.resize(static_cast<size_t>(result.width) * result.height);

	float32_t factor = -1.0f / strength;

	const float32_t kernel[3][3] =
	{
		{ factor ,factor              ,factor},
		{ factor ,1.0f - 8.0f * factor,factor},
		{ factor ,factor              ,factor}
	};

#if !WINDOWS_SYSTEM_CPU_PARALLEL
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 6) ? numberOfExecutionThreads : 6;
#endif

	for (auto Y = 0u; Y < result.height; ++Y)
	{
#if WINDOWS_SYSTEM_CPU_PARALLEL
		concurrency::parallel_for(0u, result.width, [&result, &input, &kernel, Y](uint32_t X)
			{
#else
		auto CalculateARowOfPixels = [&result, &input, &kernel](uint32_t Y)
		{

			if (Y >= result.height)return;

			for (auto X = 0u; X < result.width; ++X)
			{
#endif
				RGBAColor_32f rgba_f(0.0f, 0.0f, 0.0f, 0.0f);

				rgba_f += RGBAColor_32f(input(X - 1, Y - 1)) * kernel[0][0];
				rgba_f += RGBAColor_32f(input(X + 0, Y - 1)) * kernel[0][1];
				rgba_f += RGBAColor_32f(input(X + 1, Y - 1)) * kernel[0][2];

				rgba_f += RGBAColor_32f(input(X - 1, Y + 0)) * kernel[1][0];
				rgba_f += RGBAColor_32f(input(X + 0, Y + 0)) * kernel[1][1];
				rgba_f += RGBAColor_32f(input(X + 1, Y + 0)) * kernel[1][2];

				rgba_f += RGBAColor_32f(input(X - 1, Y + 1)) * kernel[2][0];
				rgba_f += RGBAColor_32f(input(X + 0, Y + 1)) * kernel[2][1];
				rgba_f += RGBAColor_32f(input(X + 1, Y + 1)) * kernel[2][2];

				result(X, Y) = rgba_f.toRGBAColor_8i();

#if WINDOWS_SYSTEM_CPU_PARALLEL
			});
#else
			}
		};

		// numberOfExecutionThreads threads
		static std::vector<std::unique_ptr<std::thread>> allThreads(numberOfExecutionThreads);

		for (auto i = 0; i < numberOfExecutionThreads; ++i)
		{
			allThreads[i] = std::make_unique<std::thread>(CalculateARowOfPixels,Y + i);
		}

		Y += (numberOfExecutionThreads - 1);

		for (auto& ptr : allThreads)
			ptr->join();

#endif // !WINDOWS_SYSTEM_CPU_PARALLEL	
	}
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

void ImageProcessingTools::exportFile(PngData& result, std::wstring& resultname)
{
	auto path = AdaptString::toString(resultname);

	uint32_t error = lodepng::encode(path, result.image, result.width, result.height);

	if (error)
	{
		std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		exit(0);
	}

	std::cout << "Result filename:" << path << '\n' << std::endl;
}

void ImageProcessingTools::exportFile(const byte* result, const uint32_t& width, const uint32_t& height, std::wstring& resultname)
{
	auto path = AdaptString::toString(resultname);

	uint32_t error = lodepng::encode(path, result, width, height);

	if (error)
	{
		std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::weightEffectHalf(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	float32_t sum = 0.0f;

	float32_t dx2 = dx * dx;
	float32_t dy2 = dy * dy;
	float32_t _dx2 = (1.0f - dx) * (1.0f - dx);
	float32_t _dy2 = (1.0f - dy) * (1.0f - dy);

	sum += weightResult.X = std::sqrtf(std::sqrtf(_dx2 + _dy2));
	sum += weightResult.W = std::sqrtf(std::sqrtf(dx2 + dy2));
	sum += weightResult.Y = std::sqrtf(std::sqrtf(dx2 + _dy2));
	sum += weightResult.Z = std::sqrtf(std::sqrtf(_dx2 + dy2));

	weightResult *= (4.0f / sum);
}

void ImageProcessingTools::weightEffectOne(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	float32_t sum = 0.0f;

	float32_t dx2 = dx * dx;
	float32_t dy2 = dy * dy;
	float32_t _dx2 = (1.0f - dx) * (1.0f - dx);
	float32_t _dy2 = (1.0f - dy) * (1.0f - dy);

	sum += weightResult.X = std::sqrtf(_dx2 + _dy2);
	sum += weightResult.W = std::sqrtf(dx2 + dy2);
	sum += weightResult.Y = std::sqrtf(dx2 + _dy2);
	sum += weightResult.Z = std::sqrtf(_dx2 + dy2);

	weightResult *= (4.0f / sum);
}

void ImageProcessingTools::weightEffectSquare(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	float32_t sum = 0.0f;

	float32_t dx2 = dx * dx;
	float32_t dy2 = dy * dy;
	float32_t _dx2 = (1.0f - dx) * (1.0f - dx);
	float32_t _dy2 = (1.0f - dy) * (1.0f - dy);

	sum += weightResult.X = _dx2 + _dy2;
	sum += weightResult.W = dx2 + dy2;
	sum += weightResult.Y = dx2 + _dy2;
	sum += weightResult.Z = _dx2 + dy2;

	weightResult *= (4.0f / sum);
}

void ImageProcessingTools::weightEffectQuartet(const float32_t& dx, const float32_t& dy, floatVec4& weightResult)
{
	float32_t sum = 0.0f;

	float32_t dx2 = dx * dx;
	float32_t dy2 = dy * dy;
	float32_t _dx2 = (1.0f - dx) * (1.0f - dx);
	float32_t _dy2 = (1.0f - dy) * (1.0f - dy);

	sum += weightResult.X = (_dx2 + _dy2) * (_dx2 + _dy2);
	sum += weightResult.W = (dx2 + dy2) * (dx2 + dy2);
	sum += weightResult.Y = (dx2 + _dy2) * (dx2 + _dy2);
	sum += weightResult.Z = (_dx2 + dy2) * (_dx2 + dy2);

	weightResult *= (4.0f / sum);
}

void ImageProcessingTools::zoomProgramDefault(float32_t& zoomRatio, std::filesystem::path& pngfile,float32_t& CenterWeight,const Exponent& exponent)
{
	std::cout << "Input zoom factor:" << zoomRatio << '\n';

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
		resultname.append(pngfile.parent_path()).append(L"\\").append(pngfile.stem())
			.append(L"_zoom_x").append(std::to_wstring(zoomRatio))
			.append(L"_centerWeight_").append(std::to_wstring(CenterWeight))
			.append(L"_Exponent_Mode_").append(std::to_wstring((uint32_t)exponent))
			.append(pngfile.extension());

#if !LITTLE_ENDIAN

		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#else
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);
#endif
	}
	else
	{
		std::cout << "Something wrong in sampling." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::sharpenProgram(float32_t& sharpenRatio, std::filesystem::path& pngfile)
{
	std::cout << "Input sharpen factor:" << sharpenRatio << '\n' << std::endl;

	Clamp(sharpenRatio, 0.2f, 16.0f);

	std::cout << "Adoption sharpen factor:" << sharpenRatio << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image, result;
	importFile(image, pngfile);

	if (ImageProcessingTools::Sharpen3x3(image, result, sharpenRatio))
	{
		image.clear();

		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"\\").append(pngfile.stem())
			.append(L"_sharpen_x").append(std::to_wstring(sharpenRatio))
			.append(pngfile.extension());

#if !LITTLE_ENDIAN

		//load result into stream to save to file
		result.loadRGBAtoByteStream();
		result.clearRGBA_uint8();

		exportFile(result, resultname);
#else
		exportFile(reinterpret_cast<byte*>(result.getRGBA_uint8().data()), result.width, result.height, resultname);
#endif

	}
	else
	{
		std::cout << "Something wrong in sampling." << std::endl;
		exit(0);
	}
}