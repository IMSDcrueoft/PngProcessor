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

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height, [&result, &input, &WeightEffact,&kernel, &scaleIndex](uint32_t Y){
#else
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 6) ? numberOfExecutionThreads : 6;

	for (auto Y = 0u; Y < result.height; ++Y) {
#endif
		auto GetFloorIndex = [&scaleIndex](const uint32_t& index)
		{
			return static_cast<int32_t>(floorf(index * scaleIndex));
		};

#if WINDOWS_SYSTEM_CPU_PARALLEL
		auto Row = GetFloorIndex(Y);
#else
		auto CalculateARowOfPixels = [&result, &input, &WeightEffact, &GetFloorIndex, &kernel, &scaleIndex](uint32_t Row, uint32_t Y)
		{
			if (Y >= result.height)return;
#endif
			for (auto X = 0u; X < result.width; ++X)
			{

				auto Column = GetFloorIndex(X);

				float32_t dx = X * scaleIndex - Column;
				float32_t dy = Y * scaleIndex - Row;

				//Calculate weight parameters
				floatVec4 weight;
				WeightEffact(dx, dy, weight);

				RGBAColor_32f rgba_f(0.0f, 0.0f, 0.0f, 0.0f);

				//Unrolling loops to enhance performance

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + (-1))) * (kernel[0][0] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 0, Row + (-1))) * (kernel[0][1] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 1, Row + (-1))) * (kernel[0][2] * weight.Y);
				rgba_f += RGBAColor_32f(input(Column + 2, Row + (-1))) * (kernel[0][3] * weight.Y);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + 0)) * (kernel[1][0] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 0, Row + 0)) * (kernel[1][1] * weight.X);
				rgba_f += RGBAColor_32f(input(Column + 1, Row + 0)) * (kernel[1][2] * weight.Y);
				rgba_f += RGBAColor_32f(input(Column + 2, Row + 0)) * (kernel[1][3] * weight.Y);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + 1)) * (kernel[2][0] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 0, Row + 1)) * (kernel[2][1] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 1, Row + 1)) * (kernel[2][2] * weight.W);
				rgba_f += RGBAColor_32f(input(Column + 2, Row + 1)) * (kernel[2][3] * weight.W);

				rgba_f += RGBAColor_32f(input(Column + (-1), Row + 2)) * (kernel[3][0] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 0, Row + 2)) * (kernel[3][1] * weight.Z);
				rgba_f += RGBAColor_32f(input(Column + 1, Row + 2)) * (kernel[3][2] * weight.W);
				rgba_f += RGBAColor_32f(input(Column + 2, Row + 2)) * (kernel[3][3] * weight.W);

				result(X, Y) = rgba_f.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
		};

		// numberOfExecutionThreads threads
		static std::vector<std::unique_ptr<std::thread>> allThreads(numberOfExecutionThreads);

		for (auto i = 0; i < numberOfExecutionThreads; ++i)
		{
			allThreads[i] = std::make_unique<std::thread>(
				CalculateARowOfPixels,
				GetFloorIndex(Y + i), Y + i);
		}

		Y += (numberOfExecutionThreads - 1);

		for (auto& ptr : allThreads)
			ptr->join();

	}
#endif // !WINDOWS_SYSTEM_CPU_PARALLEL
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

#if WINDOWS_SYSTEM_CPU_PARALLEL
	concurrency::parallel_for(0u, result.height,[&result, &input, &kernel](uint32_t Y){
#else
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 6) ? numberOfExecutionThreads : 6;

	for (auto Y = 0u; Y < result.height; ++Y) {

		auto CalculateARowOfPixels = [&result, &input, &kernel](uint32_t Y)
		{

			if (Y >= result.height)return;
#endif
			for (auto X = 0u; X < result.width; ++X)
			{
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
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
			});
#else
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
	uint32_t numberOfExecutionThreads = (std::thread::hardware_concurrency() + 1) >> 1;
	numberOfExecutionThreads = (numberOfExecutionThreads > 6) ? numberOfExecutionThreads : 6;

	for (auto Y = 0u; Y < inputOutput.height; ++Y) {

		auto CalculateARowOfPixels = [&inputOutput, &lumRatio](uint32_t Y) {
#endif
			for (auto X = 0u; X < inputOutput.width; ++X)
			{
				RGBAColor_32f rgba_f = RGBAColor_32f(inputOutput(X, Y));

				ImageProcessingTools::ACESToneMapping(rgba_f, lumRatio);

				inputOutput(X, Y) = rgba_f.toRGBAColor_8i();
			}
#if WINDOWS_SYSTEM_CPU_PARALLEL
		});
#else
		};

		// numberOfExecutionThreads threads
		static std::vector<std::unique_ptr<std::thread>> allThreads(numberOfExecutionThreads);

		for (auto i = 0; i < numberOfExecutionThreads; ++i)
		{
			allThreads[i] = std::make_unique<std::thread>(CalculateARowOfPixels, Y + i);
		}

		Y += (numberOfExecutionThreads - 1);

		for (auto& ptr : allThreads)
			ptr->join();
	}
#endif
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

	if ((static_cast<size_t>(result.width) * result.height * 4) >= 0xFF'FF'FF'FF)
	{
		assert(false && "Byte size is bigger than UINT32,need cut.");
		std::cout << "Byte size is bigger than UINT32,need cut." << std::endl;
	}
	else
	{
		uint32_t error = lodepng::encode(path, result.image, result.width, result.height);

		if (error)
		{
			std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			exit(0);
		}

		std::cout << "Result filename:" << path << '\n' << std::endl;
	}
}

void ImageProcessingTools::exportFile(const byte* result, const uint32_t& width, const uint32_t& height, std::wstring& resultname)
{
	auto path = AdaptString::toString(resultname);

	if ((static_cast<size_t>(width) * height * 4) >= 0xFF'FF'FF'FF)
	{
		assert(false && "Byte size is bigger than UINT32,need cut.");
		std::cout << "Byte size is bigger than UINT32,need cut." << std::endl;
	}
	else
	{
		uint32_t error = lodepng::encode(path, result, width, height);

		if (error)
		{
			std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			exit(0);
		}

		std::cout << "Result filename:" << path << '\n' << std::endl;
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

void ImageProcessingTools::help()
{
	std::cout << "Check Help Info.\n\n"
		<< "Help:[--] is a prompt, not an input.\n"
		<< "Startup parameters-->\n"
		<< "[default zoom]: z\n"
		<< "[bicubic zoom]: Z [Feature not currently supported]\n"
		<< "[sharpen]: s\n"
		<< "[tone mapping]:t\n"
		<< "[cut]: c [Feature not currently supported]\n\n"
		<< "Input Sample-->\n"
		<< "./exe filename.png z[default zoom] 1.0[zoom ratio:has default value] 0.5[center weight:has default value] 2[Exponent:has default value]\n"
		<< "[default zoom]\n[zoom ratio(from 0.001 to 32.0)]\n[center weight(from 0.25 to 13.0,0.25:MSAA,1.0:bilinear,>1:sharp)]\n[Exponent(from 1 to 4:0.5,1.0,2.0,4.0)]\n\n"
		<< "./exe filename.png t[tone mapping] 2.0[lumming ratio:has default value]\n"
		<< "[tone mapping]\n[lumming ratio(from 0.1 to 16.0)]\n"
		<< "./exe filename.png s[sharpen] 4.0[sharpen ratio:has default value]\n"
		<< "[sharpen]\n[sharpen ratio(from 0.2 to 16.0)]\n\n"
		<< "./exe filename.png c[cut] 1024[Horizontal size] 1024[Vertical size]\n" << std::endl;
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
	float32_t centerWeight = 0.64f;
	uint32_t exponent = (uint32_t)ImageProcessingTools::Exponent::square;

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
				iss >> centerWeight;

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
		std::cout << "Adoption exponent factor:" << exponent << '\n';

		ImageProcessingTools::zoomProgramDefault(Ratio, pngfile, centerWeight, (ImageProcessingTools::Exponent)exponent);
		break;
	case (int)Mode::Zoom:
		std::cout << "function call not designed!" << std::endl;
		exit(0);
		break;

	case (int)Mode::sharpen:
	case (int)Mode::Sharpen:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::sharpenProgram(Ratio, pngfile);
		break;

	case (int)Mode::cut:
	case (int)Mode::Cut:
		std::cout << "function call not designed!" << std::endl;
		exit(0);
		break;

	case (int)Mode::toneMapping:
	case (int)Mode::ToneMapping:
		if (argCount > 3)
		{
			iss.clear();
			iss.str(argValues[3]);
			iss >> Ratio;
		}
		ImageProcessingTools::hdrToneMappingProgram(Ratio, pngfile);
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
		std::cout << "Something wrong in sampling." << std::endl;
		exit(0);
	}
}

void ImageProcessingTools::hdrToneMappingProgram(float32_t& lumRatio, std::filesystem::path& pngfile)
{
	std::cout << "Input lumming factor:" << lumRatio << '\n' << std::endl;

	Clamp(lumRatio, 0.1f, 16.0f);

	std::cout << "Adoption lumming factor:" << lumRatio << '\n'
		<< "Start processing . . ." << std::endl;

	PngData image;
	importFile(image, pngfile);

	if (ImageProcessingTools::AecsHdrToneMapping(image, lumRatio))
	{
		std::wstring resultname;
		resultname.append(pngfile.parent_path()).append(L"\\").append(pngfile.stem())
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
		std::cout << "Something wrong in sampling." << std::endl;
		exit(0);
	}
}
