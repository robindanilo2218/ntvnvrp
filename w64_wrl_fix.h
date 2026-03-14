#pragma once
#include <windows.h>
#include <unknwn.h>
#include <wrl.h>  // Include w64devkit's wrl.h to get ComPtr

namespace Microsoft {
namespace WRL {

// Note: ComPtr is already defined by w64devkit's <wrl/client.h>
// We only need to provide Callback!

template <typename TInterface, typename TCallback, typename F>
struct CallbackImplMatch;

// Trait to get IID for an interface, avoiding __uuidof
template<typename T> struct InterfaceTraits;

// Macro to declare IID trait for an interface in WRL Callback
#define WRL_DECLARE_INTERFACE_IID(IFace) \
    template<> struct InterfaceTraits<IFace> { \
        static const GUID& IID() { return IID_##IFace; } \
    }

// Declare traits for all used interfaces
WRL_DECLARE_INTERFACE_IID(ICoreWebView2NavigationCompletedEventHandler);
WRL_DECLARE_INTERFACE_IID(ICoreWebView2SourceChangedEventHandler);
WRL_DECLARE_INTERFACE_IID(ICoreWebView2DocumentTitleChangedEventHandler);
WRL_DECLARE_INTERFACE_IID(ICoreWebView2NavigationStartingEventHandler);
WRL_DECLARE_INTERFACE_IID(ICoreWebView2WebResourceRequestedEventHandler);
WRL_DECLARE_INTERFACE_IID(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler);
WRL_DECLARE_INTERFACE_IID(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler);

// Specialization for lambda with 2 arguments
template <typename TInterface, typename TCallback, typename Ret, typename Class, typename Arg1, typename Arg2>
class CallbackImplMatch<TInterface, TCallback, Ret(Class::*)(Arg1, Arg2) const> : public TInterface {
public:
    CallbackImplMatch(TCallback cb) : refCount_(1), callback_(cb) {}
    virtual ~CallbackImplMatch() {}
    
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&refCount_);
        if (count == 0) delete this;
        return count;
    }
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        // Use InterfaceTraits instead of __uuidof
        if (IsEqualIID(riid, InterfaceTraits<TInterface>::IID()) || IsEqualIID(riid, IID_IUnknown)) {
            *ppv = static_cast<TInterface*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE Invoke(Arg1 a1, Arg2 a2) override {
        return callback_(a1, a2);
    }

private:
    volatile ULONG refCount_;
    TCallback callback_;
};

// Specialization for lambda with 1 argument
template <typename TInterface, typename TCallback, typename Ret, typename Class, typename Arg1>
class CallbackImplMatch<TInterface, TCallback, Ret(Class::*)(Arg1) const> : public TInterface {
public:
    CallbackImplMatch(TCallback cb) : refCount_(1), callback_(cb) {}
    virtual ~CallbackImplMatch() {}
    
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&refCount_);
        if (count == 0) delete this;
        return count;
    }
    
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        // Use InterfaceTraits instead of __uuidof
        if (IsEqualIID(riid, InterfaceTraits<TInterface>::IID()) || IsEqualIID(riid, IID_IUnknown)) {
            *ppv = static_cast<TInterface*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE Invoke(Arg1 a1) override {
        return callback_(a1);
    }

private:
    volatile ULONG refCount_;
    TCallback callback_;
};

template <typename TInterface, typename TCallback>
HRESULT Callback(TCallback cb, TInterface** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = new CallbackImplMatch<TInterface, TCallback, decltype(&TCallback::operator())>(cb);
    return S_OK;
}

template <typename TInterface, typename TCallback>
ComPtr<TInterface> Callback(TCallback cb) {
    ComPtr<TInterface> cp;
    Callback<TInterface, TCallback>(cb, cp.GetAddressOf());
    return cp;
}

} // namespace WRL
} // namespace Microsoft
