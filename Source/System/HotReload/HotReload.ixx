export module HotReload;

import ThirdParty.Platform;
import std.compat;
import ECSW;
import Logger;

export class HotReload : public System
{
protected:
	friend class UniversalMemoryPool;
	/// @brief
	HotReload(World::CVPtr world):System(world)
	{
		emSystemType = EMSystemType::HotReload;
		
	}
public:
	using Ptr = std::shared_ptr<HotReload>;
	using CVPtr = const Ptr&;

	/// @brief
	virtual ~HotReload()
	{
		if(oLibHandle != nullptr)
		{
			std::cerr << "HotReload Handle not disposed! Please check code!\n" << Platform::GetStackTrace() << "\n";
		}
	}

	virtual bool Awake() override
	{
		
		sDllDir = std::filesystem::path() / *GetWorld()->GetParam("WorkDir") / sDllDir;

		return true;
	}

	virtual void Dispose() override
	{
		FreeHandle();
		
		System::Dispose();
	}

	/// @brief load dll/so runtime library
	Platform::HotHandle LoadHandle(std::filesystem::path dllPath)
	{
#ifdef _WIN32
		dllPath = dllPath.append(SDllName);
	#ifdef NDEBUG
		// SetEnvironmentVariableA("PATH", "./Bin;%PATH%");
	#endif

		Platform::HotHandle hModule = Platform::LoadLibraryA(dllPath.string().c_str());
		if (!hModule)
		{
			LoggerPrint::Log(nullptr, EL10nCode_DllLoad, Platform::GetLastError());
			return nullptr;
		}

#elif __unix__
		std::string fullPath = filesystem::current_path().append(sDllDir).string();
		fullPath = std::format("{}/lib{}.so", fullPath, SDllName);
		void* hModule = dlopen(fullPath.c_str(), RTLD_LAZY);
		if (!hModule)
		{
			LoggerPrint::Log(nullptr, ELogLevel_Debug, dlerror());
			return nullptr;
		}
#endif


		return hModule;
	}

	/// @brief unload dll/so runtime library
	void FreeHandle()
	{
		if (oLibHandle)
		{
#ifdef _WIN32
			Platform::FreeLibrary(oLibHandle);
#elif __unix__
			dlclose(oLibHandle);
#endif
			oLibHandle = nullptr;
		}

		if (isNormalFree && !sDllDirRand.empty())
		{
			try
			{
				std::filesystem::remove_all(sDllDirRand.c_str());
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(nullptr, ELogLevel_Debug, "filesystem:{}", e.what());
			}
		}

		sDllDirRand.clear();
	}

	/// @brief reload dll/so runtime library
	bool ReloadHandle(std::function<void()> ReloadPre = nullptr)
	{
		if (!std::filesystem::exists(sDllDir))
		{
			LoggerPrint::Log(nullptr, EL10nCode_DllMenuPath);
			return false;
		}

		if (!SDllName)
		{
			LoggerPrint::Log(nullptr, EL10nCode_DllFileName);
			return false;
		}
#ifdef _WIN32

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int>  u(10000, 99999);

		int randNum = u(gen);
		std::filesystem::path newDllDir = sDllDir.parent_path() / "Runtime" / "HotReload" / std::format("Runtime_{}", randNum);
		try
		{
			std::filesystem::create_directories(newDllDir);
			std::filesystem::copy(sDllDir /* / SDllName*/, newDllDir, std::filesystem::copy_options::recursive);
		}
		catch (const std::exception& e)
		{
			LoggerPrint::Log(nullptr, ELogLevel_Debug, "{}", e.what());
			return false;
		}
#endif
		Platform::HotHandle hModule = LoadHandle(newDllDir);
		if (hModule)
		{
			if(ReloadPre)
			{
				ReloadPre();
			}

			FreeHandle();
			oLibHandle = hModule;
			sDllDirRand = newDllDir;
			Platform::SetConsoleTitleA(std::format("{}_{}", sServerName, randNum).c_str());
			return true;
		}

		return false;
	}

	std::filesystem::path GetDllPath()
	{
		return sDllDirRand;
	}

	void SetExcptionState()
	{
		isNormalFree = false;
	}

	void Clear()
	{
		try
		{
			std::filesystem::remove_all(sDllDir.parent_path() / "Runtime" / "HotReload");
		}
		catch (const std::exception& e)
		{
			LoggerPrint::Log(nullptr, ELogLevel_Debug, "filesystem:{}", e.what());
		}
	}

protected:
	/// @brief runtime library floder name
	std::filesystem::path sDllDir = "HotReload";

	/// @brief runtime library file name
	inline static const char* SDllName = "HotReload.dll";

	std::filesystem::path sDllDirRand;

	/// @brief runtime library loaded pointer
	Platform::HotHandle oLibHandle = nullptr;

	/// @brief nomal exit or exception exit
	bool isNormalFree = true;

	std::string sServerName;
};
