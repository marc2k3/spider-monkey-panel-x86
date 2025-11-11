#pragma once

namespace smp::com
{
	void ReportActiveXError(HRESULT hresult, EXCEPINFO& exception, UINT& argerr);
}
