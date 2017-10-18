typedef int8 i8;
typedef unsigned int8 u8;

typedef int16 i16;
typedef unsigned int16 u16;

typedef int32 i32;
typedef unsigned int32 u32;

export struct Color
{
	float r_;
	float g_;
	float b_;
	float a_;
};

static inline Color operator+(const Color a, const Color b)
{
	Color c;
	c.r_ = a.r_ + b.r_;
	c.g_ = a.g_ + b.g_;
	c.b_ = a.b_ + b.b_;
	c.a_ = a.a_ + b.a_;
	return c;
}

static inline Color operator*(const Color a, const float b)
{
	Color c;
	c.r_ = a.r_ * b;
	c.g_ = a.g_ * b;
	c.b_ = a.b_ * b;
	c.a_ = a.a_ * b;
	return c;
}

export struct Color_R8G8B8A8
{
	u8 r_, g_, b_, a_;
};


static inline Color GammaToLinear(Color color)
{
	// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf pg87
	float<3> colorVec = { color.r_, color.g_, color.b_ };
	float<3> linearRGBLo = { color.r_ / 12.92, color.g_ / 12.92, color.b_ / 12.92 };
	float<3> linearRGBHi = {
		pow((colorVec.x + 0.055) / 1.055, 2.4),
		pow((colorVec.y + 0.055) / 1.055, 2.4),
		pow((colorVec.z + 0.055) / 1.055, 2.4)
	};
	color.r_ = (color.r_ <= 0.04045) ? linearRGBLo.x : linearRGBHi.x,
	color.g_ = (color.g_ <= 0.04045) ? linearRGBLo.y : linearRGBHi.y,
	color.b_ = (color.b_ <= 0.04045) ? linearRGBLo.z : linearRGBHi.z;
	return color;
}

static inline Color LinearToGamma(Color color)
{
	// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf pg87
	float<3> colorVec = { color.r_, color.g_, color.b_ };
	float<3> sRGBLo = { color.r_ * 12.92, color.g_ * 12.92, color.b_ * 12.92 };
	float<3> sRGBHi = {
		(pow(abs(colorVec.x), 1.0 / 2.4) * 1.055) - 0.055,
		(pow(abs(colorVec.y), 1.0 / 2.4) * 1.055) - 0.055,
		(pow(abs(colorVec.z), 1.0 / 2.4) * 1.055) - 0.055
	};
	color.r_ = (color.r_ <= 0.0031308) ? sRGBLo.x : sRGBHi.x,
	color.g_ = (color.g_ <= 0.0031308) ? sRGBLo.y : sRGBHi.y,
	color.b_ = (color.b_ <= 0.0031308) ? sRGBLo.z : sRGBHi.z;
	return color;
}

export void ImageProc_Downsample2x(uniform int w, uniform int h, uniform const Color input[], uniform Color output[])
{
	uniform int hw = w / 2;
	uniform int hh = h / 2;
	foreach (y = 0 ... hh, x = 0 ... hw)
	{
		int inIdx = (x*2) + (y*2) * w;
		int outIdx = x + y * hw;

		Color texels[4] = {
			input[inIdx],
			input[inIdx+1],
			input[inIdx+w],
			input[inIdx+w+1],
		};

		output[outIdx] = (texels[0] + texels[1] + texels[2] + texels[3]) * 0.25;
	}
}

export void ImageProc_GammaToLinear(uniform int w, uniform int h, uniform const Color input[], uniform Color output[])
{
	foreach (y = 0 ... h, x = 0 ... w)
	{
		int inIdx = x + y * w;

		output[inIdx] = GammaToLinear(input[inIdx]);
	}
}

export void ImageProc_LinearToGamma(uniform int w, uniform int h, uniform const Color input[], uniform Color output[])
{
	foreach (y = 0 ... h, x = 0 ... w)
	{
		int inIdx = x + y * w;

		output[inIdx] = LinearToGamma(input[inIdx]);
	}
}

export void ImageProc_Unpack_R8G8B8A8(uniform int numTexels, uniform const Color_R8G8B8A8 input[], uniform Color output[])
{
	const float scale = 255.0;
	foreach (i = 0 ... numTexels)
	{
		Color_R8G8B8A8 inTexel = input[i];
		Color outTexel;
		outTexel.r_ = (float)inTexel.r_ / scale;
		outTexel.g_ = (float)inTexel.g_ / scale;
		outTexel.b_ = (float)inTexel.b_ / scale;
		outTexel.a_ = (float)inTexel.a_ / scale;
		output[i] = outTexel;
	}
}

export void ImageProc_Pack_R8G8B8A8(uniform int numTexels, uniform const Color input[], uniform Color_R8G8B8A8 output[])
{
	const float scale = 255.0;
	foreach (i = 0 ... numTexels)
	{
		Color inTexel = input[i];
		Color_R8G8B8A8 outTexel;
		outTexel.r_ = (u8)(clamp(inTexel.r_, 0.0, 1.0) * scale);
		outTexel.g_ = (u8)(clamp(inTexel.g_, 0.0, 1.0) * scale);
		outTexel.b_ = (u8)(clamp(inTexel.b_, 0.0, 1.0) * scale);
		outTexel.a_ = (u8)(clamp(inTexel.a_, 0.0, 1.0) * scale);
		output[i] = outTexel;
	}
}

