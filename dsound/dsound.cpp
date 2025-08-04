extern "C" void* __stdcall LoadLibraryA(const char* lpLibFileName);
extern "C" void* __stdcall GetProcAddress(void* hModule, const char* lpProcName);

namespace
{
	struct x {
		x()
		{
			LoadLibraryA("steam_api64.dll");
		}
	} y;

	void* GetOriginalDSound()
	{
		(void)y;
		static const auto dsound = LoadLibraryA(R"(C:\Windows\System32\dsound.dll)");
		return dsound;
	}

	template <typename FuncType>
	FuncType GetFunc(const char* name)
	{
		return (FuncType)GetProcAddress(GetOriginalDSound(), name);
	}

#define Get(f) GetFunc<decltype(&f)>(#f)

#define MakeFunc(func) \
	extern "C" __declspec(dllexport) void* __stdcall func(void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, \
		void* a7, void* a8) \
	{ \
		static const auto* f = Get(func); \
		return f(a1, a2, a3, a4, a5, a6, a7, a8); \
	}
}

MakeFunc(DirectSoundCreate)
MakeFunc(DirectSoundEnumerateA)
MakeFunc(DirectSoundEnumerateW)
MakeFunc(DllCanUnloadNow)
MakeFunc(DllGetClassObject)
MakeFunc(DirectSoundCaptureCreate)
MakeFunc(DirectSoundCaptureEnumerateA)
MakeFunc(DirectSoundCaptureEnumerateW)
MakeFunc(GetDeviceID)
MakeFunc(DirectSoundFullDuplexCreate)
MakeFunc(DirectSoundCreate8)
MakeFunc(DirectSoundCaptureCreate8)
