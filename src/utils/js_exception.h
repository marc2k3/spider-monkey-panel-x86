#pragma once

namespace smp
{
	/// @brief This exception should be used when JS exception is set
	class JsException : public std::exception
	{
	public:
		JsException() = default;
		virtual ~JsException() = default;

		_Post_satisfies_(checkValue) static void ExpectTrue(bool checkValue)
		{
			if (!checkValue)
			{
				throw JsException();
			}
		}

		/// @details This overload is needed for SAL: it can't understand that `(bool)ptr == true` is the same as  `ptr != null`
		static void ExpectTrue(_Post_notnull_ void* checkValue)
		{
			return ExpectTrue(static_cast<bool>(checkValue));
		}
	};
}
