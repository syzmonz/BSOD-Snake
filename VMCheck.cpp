#include "VMCheck.h"

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <intrin.h>
#include <string>

#pragma comment(lib, "wbemuuid.lib") // YOU KEEP THIS HERE

bool IsVirtualBox()
{
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return false;

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    if (FAILED(hres))
    {
        CoUninitialize();
        return false;
    }

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        CoUninitialize();
        return false;
    }

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hres))
    {
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT Manufacturer, Model FROM Win32_ComputerSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (0 == uReturn)
    {
        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    VARIANT vtManu, vtModel;
    pclsObj->Get(L"Manufacturer", 0, &vtManu, 0, 0);
    pclsObj->Get(L"Model", 0, &vtModel, 0, 0);

    std::wstring manu = vtManu.bstrVal;
    std::wstring model = vtModel.bstrVal;

    bool isVM =
        manu.find(L"Oracle") != std::wstring::npos ||
        model.find(L"VirtualBox") != std::wstring::npos;

    VariantClear(&vtManu);
    VariantClear(&vtModel);

    pclsObj->Release();
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return isVM;
}

bool IsRunningInVM()
{
    // Ask CPUID for hypervisor vendor string
    int cpuInfo[4] = {};
    __cpuid(cpuInfo, 0x40000000);

    // Extract vendor name
    std::string hvVendor;
    hvVendor.append((char*)&cpuInfo[1], 4);
    hvVendor.append((char*)&cpuInfo[2], 4);
    hvVendor.append((char*)&cpuInfo[3], 4);

    // VirtualBox vendor string is "VBox"
    return hvVendor.find("VBox") != std::string::npos;
}

