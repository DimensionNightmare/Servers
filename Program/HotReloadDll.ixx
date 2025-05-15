module;

export module HotReloadDll;

import std.compat;
import Platform;
import Logger;
import ECSW;

export class HotReloadDll : public System
{
protected:
	friend class World;
	/// @brief
	HotReloadDll(World::WPtr world):System(world)
	{
		World::Ptr pWorld = GetWorld();
		
		emSystemType = EMSystemType::HotReloadDll;

		sDllDir = std::filesystem::path(*pWorld->LaunchParam("program")).parent_path() / sDllDir;
		sServerName = *pWorld->LaunchParam("svrName");

		pLogger = pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
	}
public:
	using Ptr = std::shared_ptr<HotReloadDll>;

	/// @brief
	~HotReloadDll()
	{
		
	}

	void Dispose() override
	{
		System::Dispose();
		
		FreeHandle();
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
			// LoggerPrint()(EL10nCode_DllLoad, Platform::GetLastError());
			return nullptr;
		}

#elif __unix__
		std::string fullPath = filesystem::current_path().append(sDllDir).string();
		fullPath = std::format("{}/lib{}.so", fullPath, SDllName);
		void* hModule = dlopen(fullPath.c_str(), RTLD_LAZY);
		if (!hModule)
		{
			GetLogger()->Record(ELogLevel_Debug, dlerror());
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
				GetLogger->Record(ELogLevel_Debug, "filesystem:{}", e.what());
			}
		}

		sDllDirRand.clear();
	}

	/// @brief reload dll/so runtime library
	bool ReloadHandle()
	{
		if (!std::filesystem::exists(sDllDir))
		{
			GetLogger->Record(EL10nCode_DllMenuPath);
			return false;
		}

		if (!SDllName)
		{
			GetLogger->Record(EL10nCode_DllFileName);
			return false;
		}
#ifdef _WIN32

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int>  u(10000, 99999);

		int randNum = u(gen);
		std::filesystem::path newDllDir = sDllDir.parent_path() / std::format("{}/Runtime_{}", sServerName, randNum);
		try
		{
			std::filesystem::create_directories(newDllDir);
			std::filesystem::copy(sDllDir /* / SDllName*/, newDllDir, std::filesystem::copy_options::recursive);
		}
		catch (const std::exception& e)
		{
			GetLogger->Record(ELogLevel_Debug, "{}", e.what());
			return false;
		}
#endif
		Platform::HotHandle hModule = LoadHandle(newDllDir);
		if (hModule)
		{
			FreeHandle();
			oLibHandle = hModule;
			sDllDirRand = newDllDir;
			Platform::SetConsoleTitleA(std::format("{}_{}", sServerName, randNum).c_str());
			return true;
		}

		return false;
	}

	/// @brief get runtime lib funcpointer
	Platform::FuncHandle GetFuncPtr(const char* funcName)
	{
#ifdef _WIN32
		return Platform::GetProcAddress(oLibHandle, funcName);
#elif __unix__
		return dlsym(oLibHandle, funcName);
#endif
		return nullptr;
	}

	std::filesystem::path GetDllPath()
	{
		return sDllDirRand;
	}

	void SetExcptionState()
	{
		isNormalFree = false;
	}

protected:

	LoggerPrint::Ptr GetLogger(){ return pLogger.lock(); }

protected:
	/// @brief runtime library floder name
	std::filesystem::path sDllDir = "Runtime";

	/// @brief runtime library file name
	inline static const char* SDllName = "HotReload.dll";

	std::filesystem::path sDllDirRand;

	/// @brief runtime library loaded pointer
	Platform::HotHandle oLibHandle = nullptr;

	/// @brief nomal exit or exception exit
	bool isNormalFree = true;

	std::string sServerName;

	LoggerPrint::WPtr pLogger;
};
