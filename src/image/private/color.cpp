#include "image/color.h"

#include "core/misc.h"
#include "math/vec3.h"
#include "math/vec4.h"

#include <cmath>

namespace Image
{
	namespace
	{
		// http://ploobs.com.br/arquivos/1499
		Math::Vec3 Hue(float h)
		{
			float r = std::abs(h * 6.0f - 3.0f) - 1.0f;
			float g = 2.0f - std::abs(h * 6.0f - 2.0f);
			float b = 2.0f - std::abs(h * 6.0f - 4.0f);
			return Math::Vec3(Core::Clamp(r, 0.0f, 1.0f), Core::Clamp(g, 0.0f, 1.0f), Core::Clamp(b, 0.0f, 1.0f));
		}

		// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf pg87
		f32 SingleToSRGB(f32 linearCol)
		{
			const f32 sRGBLo = linearCol * 12.92f;
			const f32 sRGBHi = (std::powf(std::abs(linearCol), 1.0f / 2.4f) * 1.055f) - 0.055f;
			const f32 sRGB = (linearCol <= 0.0031308f) ? sRGBLo : sRGBHi;
			return sRGB;
		}

		f32 SingleFromSRGB(f32 sRGBCol)
		{
			const f32 linearRGBLo = sRGBCol / 12.92f;
			const f32 linearRGBHi = std::powf((sRGBCol + 0.055f) / 1.055f, 2.4f);
			const f32 linearCol = (sRGBCol <= 0.04045f) ? linearRGBLo : linearRGBHi;
			return linearCol;
		}
	}


	HSVColor::HSVColor()
	    : h(0.0f)
	    , s(0.0f)
	    , v(0.0f)
	{
	}

	HSVColor::HSVColor(const f32* val)
	    : h(val[0])
	    , s(val[1])
	    , v(val[2])
	{
	}

	HSVColor::HSVColor(f32 _h, f32 _s, f32 _v)
	    : h(_h)
	    , s(_s)
	    , v(_v)
	{
	}

	YCoCgColor::YCoCgColor()
	    : y(0.0f)
	    , co(0.0f)
	    , cg(0.0f)
	{
	}

	YCoCgColor::YCoCgColor(const f32* val)
	    : y(val[0])
	    , co(val[1])
	    , cg(val[2])
	{
	}

	YCoCgColor::YCoCgColor(f32 _y, f32 _co, f32 _cg)
	    : y(_y)
	    , co(_co)
	    , cg(_cg)
	{
	}

	SRGBAColor::SRGBAColor()
	    : r(0)
	    , g(0)
	    , b(0)
	    , a(0)
	{
	}

	SRGBAColor::SRGBAColor(const u8* val)
	    : r(val[0])
	    , g(val[1])
	    , b(val[2])
	    , a(val[3])
	{
	}

	SRGBAColor::SRGBAColor(u8 _r, u8 _g, u8 _b, u8 _a)
	    : r(_r)
	    , g(_g)
	    , b(_b)
	    , a(_a)
	{
	}


	RGBAColor::RGBAColor()
	    : r(0.0f)
	    , g(0.0f)
	    , b(0.0f)
	    , a(0.0f)
	{
	}

	RGBAColor::RGBAColor(const f32* val)
	    : r(val[0])
	    , g(val[1])
	    , b(val[2])
	    , a(val[3])
	{
	}

	RGBAColor::RGBAColor(f32 _r, f32 _g, f32 _b, f32 _a)
	    : r(_r)
	    , g(_g)
	    , b(_b)
	    , a(_a)
	{
	}

	HSVColor ToHSV(RGBAColor rgb)
	{
		HSVColor hsv;
		hsv.v = Core::Max(rgb.r, Core::Max(rgb.g, rgb.b));
		float m = Core::Min(rgb.r, Core::Min(rgb.g, rgb.b));
		float c = hsv.v - m;
		if(c != 0)
		{
			hsv.s = c / hsv.v;
			Math::Vec3 delta = (Math::Vec3(hsv.v, hsv.v, hsv.v) - Math::Vec3(rgb.r, rgb.g, rgb.b)) / c;
			delta -= Math::Vec3(delta.z, delta.x, delta.y);
			delta.x += 2.0f;
			delta.y += 4.0f;
			if(rgb.r >= hsv.v)
				hsv.h = delta.z;
			else if(rgb.g >= hsv.v)
				hsv.h = delta.x;
			else
				hsv.h = delta.y;
			hsv.h = std::fmod(hsv.h / 6.0f, 1.0f);
			if(hsv.h < 0.0f)
				hsv.h += 1.0f;
		}
		return HSVColor(hsv);
	}

	YCoCgColor ToYCoCg(RGBAColor rgb)
	{
		Math::Vec3 xyz(rgb.r, rgb.g, rgb.b);
		return YCoCgColor(xyz.Dot(Math::Vec3(0.25f, 0.5f, 0.25f)), xyz.Dot(Math::Vec3(0.5f, 0.0f, -0.5f)),
		    xyz.Dot(Math::Vec3(-0.25f, 0.5f, -0.25f)));
	}

	SRGBAColor ToSRGBA(RGBAColor rgba)
	{
		RGBAColor sRGBA = {
		    SingleToSRGB(rgba.r), SingleToSRGB(rgba.g), SingleToSRGB(rgba.b), rgba.a,
		};
		sRGBA *= 255.0f;
		sRGBA.r = std::roundf(sRGBA.r);
		sRGBA.g = std::roundf(sRGBA.g);
		sRGBA.b = std::roundf(sRGBA.b);
		sRGBA.a = std::roundf(sRGBA.a);
		return SRGBAColor((u8)sRGBA.r, (u8)sRGBA.g, (u8)sRGBA.b, (u8)sRGBA.a);
	}


	RGBAColor ToRGB(HSVColor hsv)
	{
		const auto ONE = Math::Vec3(1.0f, 1.0f, 1.0f);
		Math::Vec3 rgb = ((Hue(hsv.h) - ONE) * hsv.s + ONE) * hsv.v;
		return RGBAColor(rgb.x, rgb.y, rgb.z, 1.0f);
	}

	RGBAColor ToRGB(YCoCgColor ycocg)
	{
		const float v = ycocg.y - ycocg.cg;
		return RGBAColor(v + ycocg.co, ycocg.y + ycocg.cg, v - ycocg.co, 1.0f);
	}

	RGBAColor ToRGBA(SRGBAColor rgba)
	{
		RGBAColor sRGBA = {
		    SingleFromSRGB((f32)rgba.r / 255.0f), SingleFromSRGB((f32)rgba.g / 255.0f),
		    SingleFromSRGB((f32)rgba.b / 255.0f), (f32)rgba.a / 255.0f,
		};
		return sRGBA;
	}

} // namespace Image
