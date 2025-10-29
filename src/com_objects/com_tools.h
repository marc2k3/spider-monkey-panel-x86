#pragma once
#include <com_objects/com_interface.h>

namespace smp::com
{

namespace internal
{

class TypeInfoCacheHolder
{
public:
	TypeInfoCacheHolder();

	[[nodiscard]] bool Empty();

	void InitFromTypelib(ITypeLib* p_typeLib, const GUID& guid);

	// "Expose" some ITypeInfo related methods here
	HRESULT GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
	HRESULT GetIDsOfNames(LPOLESTR* rgszNames, UINT cNames, MEMBERID* pMemId);
	HRESULT Invoke(PVOID pvInstance, MEMBERID memid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);

protected:
	std::unordered_map<ULONG, DISPID> cache_;
	ITypeInfoPtr typeInfo_;
};

} // namespace internal

extern wil::com_ptr<ITypeLib> typelib;

//-- IDispatch --
template <class T>
class IDispatchWithCachedTypes : public T
{
public:
	STDMETHOD(GetTypeInfoCount)(unsigned int* n)
	{
		if (!n)
		{
			return E_INVALIDARG;
		}
		*n = 1;
		return S_OK;
	}

	STDMETHOD(GetTypeInfo)(unsigned int i, LCID lcid, ITypeInfo** pp)
	{
		return g_typeInfoCacheHolder.GetTypeInfo(i, lcid, pp);
	}

	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** names, unsigned int cnames, LCID lcid, DISPID* dispids)
	{
		if (g_typeInfoCacheHolder.Empty())
		{
			return E_UNEXPECTED;
		}
		return g_typeInfoCacheHolder.GetIDsOfNames(names, cnames, dispids);
	}

	STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flag, DISPPARAMS* params, VARIANT* result, EXCEPINFO* excep, unsigned int* err)
	{
		if (g_typeInfoCacheHolder.Empty())
		{
			return E_UNEXPECTED;
		}
		return g_typeInfoCacheHolder.Invoke(this, dispid, flag, params, result, excep, err);
	}

protected:
	IDispatchWithCachedTypes<T>()
	{
		if (g_typeInfoCacheHolder.Empty() && typelib)
		{
			g_typeInfoCacheHolder.InitFromTypelib(typelib.get(), __uuidof(T));
		}
	}

	virtual ~IDispatchWithCachedTypes<T>() = default;

	virtual void FinalRelease()
	{
	}

protected:
	static smp::com::internal::TypeInfoCacheHolder g_typeInfoCacheHolder;
};

template <class T>
__declspec(selectany) smp::com::internal::TypeInfoCacheHolder IDispatchWithCachedTypes<T>::g_typeInfoCacheHolder;

//-- IDispatch impl -- [T] [IDispatch] [IUnknown]
template <class T>
class IDispatchImpl3 : public IDispatchWithCachedTypes<T>
{
protected:
	IDispatchImpl3<T>() = default;
	~IDispatchImpl3<T>() override = default;

private:
	COM_QI_BEGIN()
		COM_QI_ENTRY(IDispatch)
		COM_QI_ENTRY(T)
	COM_QI_END()
};

template <typename T, bool ShouldAddRef = true>
class ComPtrImpl : public T
{
public:
	template <typename... Args>
	ComPtrImpl(Args&&... args)
		: T(std::forward<Args>(args)...)
	{
		if constexpr (ShouldAddRef)
		{
			++refCount_;
		}
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return ++refCount_;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		const ULONG n = --refCount_;
		if (!n)
		{
			this->FinalRelease();
			delete this;
		}
		return n;
	}

private:
	~ComPtrImpl() override = default;

private:
	std::atomic<ULONG> refCount_ = 0;
};

} // namespace smp::com
