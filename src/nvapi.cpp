#ifdef VEL_USE_NVAPI

#include <algorithm>
#include <string>

#include <nvapi/nvapi.h>
#include <nvapi/NvApiDriverSettings.h>

#include <spdlog/spdlog.h>

#include <vel/Util/Assert.h>
#include <vel/nvapi.h>

namespace vel
{
	namespace
	{
		bool nvapiStatusOk(NvAPI_Status status, const char* operation)
		{
			if (status == NVAPI_OK)
				return true;

			NvAPI_ShortString description{};
			NvAPI_GetErrorMessage(status, description);

			SPDLOG_WARN("{} failed with NVAPI status {}: {}", operation, static_cast<int>(status), description);

			return false;
		}

		void setNVUstring(NvAPI_UnicodeString& destination, const std::wstring& source)
		{
			constexpr size_t maxLength = NVAPI_UNICODE_STRING_MAX - 1;

			VEL_ASSERT(source.size() <= maxLength, "setNVUstring(): Input string exceeds NVAPI_UNICODE_STRING_MAX.");

			for (size_t i = 0; i < NVAPI_UNICODE_STRING_MAX; ++i)
				destination[i] = 0;

			const size_t copyLength = std::min(source.size(), maxLength);

			for (size_t i = 0; i < copyLength; ++i)
				destination[i] = static_cast<NvU16>(source[i]);
		}

		struct NvApiResources
		{
			bool initialized = false;
			NvDRSSessionHandle session = nullptr;

			~NvApiResources()
			{
				if (this->session)
					NvAPI_DRS_DestroySession(this->session);

				if (this->initialized)
					NvAPI_Unload();
			}
		};
	}

	void initNvidiaApplicationProfile(const std::string& applicationName, const std::string& profileName)
	{
		const std::wstring wideApplicationName(applicationName.begin(), applicationName.end());
		const std::wstring wideProfileName(profileName.begin(), profileName.end());

		NvApiResources resources;

		NvAPI_Status status = NvAPI_Initialize();
		if (!nvapiStatusOk(status, "NvAPI_Initialize()"))
			return;

		resources.initialized = true;

		status = NvAPI_DRS_CreateSession(&resources.session);
		if (!nvapiStatusOk(status, "NvAPI_DRS_CreateSession()"))
			return;

		status = NvAPI_DRS_LoadSettings(resources.session);
		if (!nvapiStatusOk(status, "NvAPI_DRS_LoadSettings()"))
			return;

		NvAPI_UnicodeString nvApplicationName{};
		NvAPI_UnicodeString nvProfileName{};

		setNVUstring(nvApplicationName, wideApplicationName);
		setNVUstring(nvProfileName, wideProfileName);

		NvDRSProfileHandle profileHandle = nullptr;

		NVDRS_APPLICATION application{};
		application.version = NVDRS_APPLICATION_VER_V1;

		status = NvAPI_DRS_FindApplicationByName(resources.session, nvApplicationName, &profileHandle, &application);

		if (status == NVAPI_EXECUTABLE_NOT_FOUND)
		{
			status = NvAPI_DRS_FindProfileByName(resources.session, nvProfileName, &profileHandle);

			if (status == NVAPI_PROFILE_NOT_FOUND)
			{
				NVDRS_PROFILE profile{};
				profile.version = NVDRS_PROFILE_VER;
				profile.isPredefined = 0;

				setNVUstring(profile.profileName, wideProfileName);

				status = NvAPI_DRS_CreateProfile(resources.session, &profile, &profileHandle);
				if (!nvapiStatusOk(status, "NvAPI_DRS_CreateProfile()"))
					return;
			}
			else if (!nvapiStatusOk(status, "NvAPI_DRS_FindProfileByName()"))
			{
				return;
			}

			NVDRS_APPLICATION newApplication{};
			newApplication.version = NVDRS_APPLICATION_VER_V1;
			newApplication.isPredefined = 0;

			setNVUstring(newApplication.appName, wideApplicationName);
			setNVUstring(newApplication.userFriendlyName, wideProfileName);
			setNVUstring(newApplication.launcher, L"");
			setNVUstring(newApplication.fileInFolder, L"");

			status = NvAPI_DRS_CreateApplication(resources.session, profileHandle, &newApplication);
			if (!nvapiStatusOk(status, "NvAPI_DRS_CreateApplication()"))
				return;
		}
		else if (!nvapiStatusOk(status, "NvAPI_DRS_FindApplicationByName()"))
		{
			return;
		}

		const bool threadedOptimization = false;

		NVDRS_SETTING setting{};
		setting.version = NVDRS_SETTING_VER;
		setting.settingId = OGL_THREAD_CONTROL_ID;
		setting.settingType = NVDRS_DWORD_TYPE;
		setting.settingLocation = NVDRS_CURRENT_PROFILE_LOCATION;
		setting.isCurrentPredefined = 0;
		setting.isPredefinedValid = 0;
		setting.u32CurrentValue = threadedOptimization ? OGL_THREAD_CONTROL_ENABLE : OGL_THREAD_CONTROL_DISABLE;
		setting.u32PredefinedValue = threadedOptimization ? OGL_THREAD_CONTROL_ENABLE : OGL_THREAD_CONTROL_DISABLE;

		status = NvAPI_DRS_SetSetting(resources.session, profileHandle, &setting);
		if (!nvapiStatusOk(status, "NvAPI_DRS_SetSetting()"))
			return;

		status = NvAPI_DRS_SaveSettings(resources.session);
		if (!nvapiStatusOk(status, "NvAPI_DRS_SaveSettings()"))
			return;

		SPDLOG_INFO("Nvidia application profile updated successfully.");
	}
}

#endif