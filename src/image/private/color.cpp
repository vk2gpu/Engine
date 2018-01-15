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
		Math::Vec3 hue(float h)
		{
			float r = std::abs(h * 6.0f - 3.0f) - 1.0f;
			float g = 2.0f - std::abs(h * 6.0f - 2.0f);
			float b = 2.0f - std::abs(h * 6.0f - 4.0f);
			return Math::Vec3(Core::Clamp(r, 0.0f, 1.0f), Core::Clamp(g, 0.0f, 1.0f), Core::Clamp(b, 0.0f, 1.0f));
		}
	}


	HSVColor::HSVColor()
	    : h(0.0f)
	    , s(0.0f)
	    , v(0.0f)
	{
	}

	HSVColor::HSVColor(f32* val)
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

	YCoCgColor::YCoCgColor(f32* val)
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

	SRGBAColor::SRGBAColor(u8* val)
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

	RGBAColor::RGBAColor(f32* val)
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
		// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf pg87
		Math::Vec3 sRGBLo(rgba.r * 12.92f, rgba.g * 12.92f, rgba.b * 12.92f);
		Math::Vec3 sRGBHi(std::pow(std::abs(rgba.r), 1.0f / 2.4f), std::pow(std::abs(rgba.g), 1.0f / 2.4f),
		    std::pow(std::abs(rgba.b), 1.0f / 2.4f));
		sRGBHi = (sRGBHi * 1.055f) - Math::Vec3(0.055f, 0.055f, 0.055f);
		RGBAColor sRGBA = RGBAColor((rgba.r <= 0.0031308) ? sRGBLo.x : sRGBHi.x,
		    (rgba.g <= 0.0031308f) ? sRGBLo.y : sRGBHi.y, (rgba.b <= 0.0031308f) ? sRGBLo.z : sRGBHi.z, rgba.a);
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
		Math::Vec3 rgb = ((hue(hsv.h) - ONE) * hsv.s + ONE) * hsv.v;
		return RGBAColor(rgb.x, rgb.y, rgb.z, 1.0f);
	}

	RGBAColor ToRGB(YCoCgColor ycocg)
	{
		const float v = ycocg.y - ycocg.cg;
		return RGBAColor(v + ycocg.co, ycocg.y + ycocg.cg, v - ycocg.co, 1.0f);
	}

	RGBAColor ToRGBA(SRGBAColor rgba)
	{
		// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf pg87
		Math::Vec3 linearRGBLo(rgba.r / 12.92f, rgba.g / 12.92f, rgba.b / 12.92f);
		Math::Vec3 linearRGBHi(std::pow(((rgba.r / 255.0f) + 0.055f) / 1.055f, 2.4f),
		    std::pow(((rgba.g / 255.0f) + 0.055f) / 1.055f, 2.4f),
		    std::pow(((rgba.b / 255.0f) + 0.055f) / 1.055f, 2.4f));
		return RGBAColor((rgba.r <= (255 * 0.04045)) ? linearRGBLo.x : linearRGBHi.x,
		    (rgba.g <= (255 * 0.04045)) ? linearRGBLo.y : linearRGBHi.y,
		    (rgba.b <= (255 * 0.04045)) ? linearRGBLo.z : linearRGBHi.z, rgba.a / 255.0f);
	}

} // namespace Image
