#include <iostream>
#include <vector>
#include <openvr.h>

//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
    uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
    if (unRequiredBufferLen == 0)
        return "";

    char *pchBuffer = new char[unRequiredBufferLen];
    unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
    std::string sResult = pchBuffer;
    delete[] pchBuffer;
    return sResult;
}

void showDevice(vr::IVRSystem *p)
{
    if(!p)
    {
        return;
    }
    for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
    {
        auto deviceClass = p->GetTrackedDeviceClass(i);
        if (deviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid)
        {
            continue;
        }
        std::cout << "tracker[" << i << "] ";
        auto driver = p->GetUint64TrackedDeviceProperty(i, vr::Prop_ParentDriver_Uint64);
        std::cout << std::endl;
    }
}

void showDriver(vr::IVRDriverManager *p)
{
    if(!p)
    {
        return;
    }
    int drivers = p->GetDriverCount();
    for (int i = 0; i < drivers; ++i)
    {
        auto len = p->GetDriverName(i, nullptr, 0);
        std::vector<char> name(len + 1);
        p->GetDriverName(i, name.data(), len);
        auto handle = p->GetDriverHandle(name.data());
        std::cout << "driver[" << i << "]"
                  << name.data() << " "
                  << (p->IsEnabled(i) ? "enable" : "")
                  << "handle: " << handle
                  << std::endl;
    }
}

void ShowApp(vr::IVRApplications *p)
{
    if(!p)
    {
        return;
    }
    auto count = p->GetApplicationCount();
    for (uint32_t i = 0; i < count; ++i)
    {
        std::cout << "app[" << i << "] ";
        char buffer[vr::k_unMaxApplicationKeyLength];
        auto error = p->GetApplicationKeyByIndex(i, buffer, sizeof(buffer));
        if (error != vr::EVRApplicationError::VRApplicationError_None)
        {
            // error
            std::cout << "error";
        }
        else
        {
            std::cout << buffer;
        }
        std::cout << std::endl;
    }
}

int main()
{
    vr::EVRInitError eError;
    auto system = vr::VR_Init(&eError, vr::EVRApplicationType::VRApplication_Background);
    if (eError != vr::VRInitError_None)
    {
        std::cerr << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << std::endl;
        return 1;
    }
    std::cout << "VR_Init" << std::endl;

    // apps
    ShowApp(vr::VRApplications());

    // drivers
    showDriver(vr::VRDriverManager());

    // trackers
    showDevice(system);

    // inputs

    return 0;
}
