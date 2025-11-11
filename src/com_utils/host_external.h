#pragma once
#include "com_tools.h"

_COM_SMARTPTR_TYPEDEF(IHostExternal, __uuidof(IHostExternal));

namespace smp::com
{
	class HostExternal : public IDispatchImpl3<IHostExternal>
	{
	protected:
		HostExternal(_variant_t data) : data_(data) {}

		~HostExternal() override = default;

	public:
		STDMETHODIMP get_dialogArguments(VARIANT* pData) override
		{
			if (pData)
			{
				return VariantCopy(pData, &data_);
			}
			else
			{
				return S_OK;
			}
		}

	private:
		_variant_t data_;
	};
}
