#pragma once
#include <qwr/ui_ddx.h>
#include <qwr/ui_option.h>

namespace qwr
{
	class IUiDdxOption
	{
	public:
		IUiDdxOption() = default;
		virtual ~IUiDdxOption() = default;

		virtual IUiDdx& Ddx() = 0;
		virtual const IUiDdx& Ddx() const = 0;
		virtual IUiOption& Option() = 0;
		virtual const IUiOption& Option() const = 0;
	};

	template <template <typename> typename DdxT, typename OptionT>
	class UiDdxOption final : public IUiDdxOption
	{
	public:
		template <typename... Args>
		UiDdxOption(OptionT& option, Args&&... args) : option_(option), ddx_(option, std::forward<Args>(args)...)
		{
			static_assert(std::is_base_of_v<IUiOption, OptionT>);
		}

		~UiDdxOption() override = default;

		IUiDdx& Ddx() override
		{
			return ddx_;
		}

		const IUiDdx& Ddx() const override
		{
			return ddx_;
		}

		IUiOption& Option() override
		{
			return option_;
		}

		const IUiOption& Option() const override
		{
			return option_;
		}

	private:
		OptionT& option_;
		DdxT<OptionT> ddx_;
	};

	template <template <typename> typename DdxT, typename OptionT, typename... Args>
	std::unique_ptr<IUiDdxOption> CreateUiDdxOption(OptionT& value, Args&&... args)
	{
		return std::make_unique<UiDdxOption<DdxT, OptionT>>(value, std::forward<Args>(args)...);
	}
}
