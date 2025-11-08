#pragma once

namespace smp::image
{
	using Size = std::tuple<uint32_t, uint32_t>;

	[[nodiscard]] Size GetResizedImageSize(const Size& currentDimension, const Size& maxDimensions) noexcept;
	[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> Load(IStream* stream);
}
