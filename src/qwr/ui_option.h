#pragma once

namespace qwr
{
	class IUiOption
	{
	public:
		IUiOption() = default;
		virtual ~IUiOption() = default;

		virtual bool HasChanged() const = 0;
		virtual void Apply() = 0;
		virtual void Revert() = 0;
		virtual void ResetToDefault() = 0;
	};

	template <typename T, typename = void>
	class UiOption : public IUiOption
	{
	public:
		using value_type = typename T;

	public:
		UiOption() = default;

		UiOption(const value_type& value, const value_type& defaultValue)
		{
			InitializeValue(value, defaultValue);
		}

		UiOption(const UiOption&) = delete;

		UiOption& operator=(const value_type& value)
		{
			SetValue(value);
			return *this;
		}

		operator const value_type&() const
		{
			return GetCurrentValue();
		}

		const value_type& GetSavedValue() const
		{
			return savedValue_;
		}

		const value_type& GetCurrentValue() const
		{
			return curValue_;
		}

		void InitializeValue(const value_type& value, const value_type& defaultValue)
		{
			defaultValue_ = defaultValue;
			savedValue_ = value;
			curValue_ = value;
			hasChanged_ = false;
		}

		void SetValue(const value_type& value, bool dontCheck = false)
		{
			hasChanged_ = (dontCheck ? true : (savedValue_ != value));
			curValue_ = value;
		}

		bool HasChanged() const override
		{
			return hasChanged_;
		}

		void Apply() override
		{
			if (hasChanged_)
			{
				savedValue_ = curValue_;
				hasChanged_ = false;
			}
		}

		void Revert() override
		{
			if (hasChanged_)
			{
				curValue_ = savedValue_;
				hasChanged_ = false;
			}
		}

		void ResetToDefault() override
		{
			SetValue(defaultValue_);
		}

	private:
		bool hasChanged_ = false;
		value_type defaultValue_{};
		value_type savedValue_{};
		value_type curValue_{};
	};
}
