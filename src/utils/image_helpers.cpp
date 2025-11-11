#include <stdafx.h>
#include "image_helpers.h"
#include <utils/gdi_helpers.h>

namespace
{
	std::unique_ptr<Gdiplus::Bitmap> LoadWithWIC(IStream* stream)
	{
		auto factory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory);

		if (!factory)
			return nullptr;

		wil::com_ptr<IWICBitmapDecoder> decoder;
		wil::com_ptr<IWICBitmapFrameDecode> frame;
		wil::com_ptr<IWICBitmapSource> source;

		if FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder))
			return nullptr;

		if FAILED(decoder->GetFrame(0, &frame))
			return nullptr;

		if FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, frame.get(), &source))
			return nullptr;

		uint32_t w{}, h{};

		if FAILED(source->GetSize(&w, &h))
			return nullptr;

		auto bitmap = std::make_unique<Gdiplus::Bitmap>(w, h, PixelFormat32bppPARGB);

		Gdiplus::Rect rect{ 0, 0, static_cast<INT>(w), static_cast<INT>(h) };
		Gdiplus::BitmapData bmpdata{};

		if (Gdiplus::Ok != bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bmpdata))
			return nullptr;

		const auto hr = source->CopyPixels(nullptr, bmpdata.Stride, bmpdata.Stride * bmpdata.Height, static_cast<uint8_t*>(bmpdata.Scan0));
		bitmap->UnlockBits(&bmpdata);

		if FAILED(hr)
			return nullptr;

		return bitmap;
	}
}

namespace smp::image
{
	Size GetResizedImageSize(const Size& currentDimension, const Size& maxDimensions) noexcept
	{
		const auto& [maxWidth, maxHeight] = maxDimensions;
		const auto& [imgWidth, imgHeight] = currentDimension;

		if (imgWidth <= maxWidth && imgHeight <= maxHeight)
			return std::make_tuple(imgWidth, imgHeight);

		uint32_t newWidth{}, newHeight{};
		const double imgRatio = static_cast<double>(imgHeight) / imgWidth;
		const double constraintsRatio = static_cast<double>(maxHeight) / maxWidth;

		if (imgRatio > constraintsRatio)
		{
			newHeight = maxHeight;
			newWidth = lround(newHeight / imgRatio);
		}
		else
		{
			newWidth = maxWidth;
			newHeight = lround(newWidth * imgRatio);
		}

		return std::make_tuple(newWidth, newHeight);
	}

	std::unique_ptr<Gdiplus::Bitmap> Load(IStream* stream)
	{
		auto bitmap = std::make_unique<Gdiplus::Bitmap>(stream, TRUE);

		if (smp::gdi::IsGdiPlusObjectValid(bitmap.get()))
			return bitmap;

		return LoadWithWIC(stream);
	}
}
