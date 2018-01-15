typedef int8 i8;
typedef unsigned int8 u8;

typedef int16 i16;
typedef unsigned int16 u16;

typedef int32 i32;
typedef unsigned int32 u32;

export struct Error
{
	double r;
	double g;
	double b;
	double a;
};

export struct Color
{
	float r;
	float g;
	float b;
	float a;
};

static inline Color operator+(const Color a, const Color b)
{
	Color c;
	c.r = a.r + b.r;
	c.g = a.g + b.g;
	c.b = a.b + b.b;
	c.a = a.a + b.a;
	return c;
}

static inline Color operator*(const Color a, const float b)
{
	Color c;
	c.r = a.r * b;
	c.g = a.g * b;
	c.b = a.b * b;
	c.a = a.a * b;
	return c;
}

export struct Color_R8G8B8A8
{
	u8 r, g, b, a;
};


static inline Color GammaToLinear(Color color)
{
	// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_movingfrostbite_to_pbr.pdf pg87
	float<3> colorVec = { color.r, color.g, color.b };
	float<3> linearRGBLo = { color.r / 12.92, color.g / 12.92, color.b / 12.92 };
	float<3> linearRGBHi = {
		pow((colorVec.x + 0.055) / 1.055, 2.4),
		pow((colorVec.y + 0.055) / 1.055, 2.4),
		pow((colorVec.z + 0.055) / 1.055, 2.4)
	};
	color.r = (color.r <= 0.04045) ? linearRGBLo.x : linearRGBHi.x,
	color.g = (color.g <= 0.04045) ? linearRGBLo.y : linearRGBHi.y,
	color.b = (color.b <= 0.04045) ? linearRGBLo.z : linearRGBHi.z;
	return color;
}

static inline Color LinearToGamma(Color color)
{
	// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_movingfrostbite_to_pbr.pdf pg87
	float<3> colorVec = { color.r, color.g, color.b };
	float<3> sRGBLo = { color.r * 12.92, color.g * 12.92, color.b * 12.92 };
	float<3> sRGBHi = {
		(pow(abs(colorVec.x), 1.0 / 2.4) * 1.055) - 0.055,
		(pow(abs(colorVec.y), 1.0 / 2.4) * 1.055) - 0.055,
		(pow(abs(colorVec.z), 1.0 / 2.4) * 1.055) - 0.055
	};
	color.r = (color.r <= 0.0031308) ? sRGBLo.x : sRGBHi.x,
	color.g = (color.g <= 0.0031308) ? sRGBLo.y : sRGBHi.y,
	color.b = (color.b <= 0.0031308) ? sRGBLo.z : sRGBHi.z;
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


export void ImageProc_MSE(uniform Error output[], uniform int w, uniform int h, uniform const Color base[], uniform const Color compare[])
{
	uniform double numTexels = (double)w * (double)h;
	double sumSqR = 0.0;
	double sumSqG = 0.0;
	double sumSqB = 0.0;
	double sumSqA = 0.0;

	foreach (y = 0 ... h, x = 0 ... w)
	{
		int inIdx = x + y * w;

		Color baseTexel = base[inIdx];
		Color compareTexel = compare[inIdx];

		Color diff = {
			baseTexel.r - compareTexel.r,
			baseTexel.g - compareTexel.g,
			baseTexel.b - compareTexel.b,
			baseTexel.a - compareTexel.a,
		};

		sumSqR += diff.r * diff.r;
		sumSqG += diff.g * diff.g;
		sumSqB += diff.b * diff.b;
		sumSqA += diff.a * diff.a;
	}

	output->r = (reduce_add(sumSqR) / numTexels);
	output->g = (reduce_add(sumSqG) / numTexels);
	output->b = (reduce_add(sumSqB) / numTexels);
	output->a = (reduce_add(sumSqA) / numTexels);
}


export void ImageProc_MSE_R8G8B8A8(uniform Error output[], uniform int w, uniform int h, uniform const Color_R8G8B8A8 base[], uniform const Color_R8G8B8A8 compare[])
{
	uniform double numTexels = (double)w * (double)h;
	double sumSqR = 0.0;
	double sumSqG = 0.0;
	double sumSqB = 0.0;
	double sumSqA = 0.0;

	foreach (y = 0 ... h, x = 0 ... w)
	{
		int inIdx = x + y * w;

		Color_R8G8B8A8 baseTexel = base[inIdx];
		Color_R8G8B8A8 compareTexel = compare[inIdx];

		Color diff = {
			(float)baseTexel.r - (float)compareTexel.r,
			(float)baseTexel.g - (float)compareTexel.g,
			(float)baseTexel.b - (float)compareTexel.b,
			(float)baseTexel.a - (float)compareTexel.a,
		};

		sumSqR += diff.r * diff.r;
		sumSqG += diff.g * diff.g;
		sumSqB += diff.b * diff.b;
		sumSqA += diff.a * diff.a;
	}


	output->r = (reduce_add(sumSqR) / numTexels);
	output->g = (reduce_add(sumSqG) / numTexels);
	output->b = (reduce_add(sumSqB) / numTexels);
	output->a = (reduce_add(sumSqA) / numTexels);
}

export void ImageProc_Unpack_R8G8B8A8(uniform int numTexels, uniform const Color_R8G8B8A8 input[], uniform Color output[])
{
	const float scale = 255.0;
	foreach (i = 0 ... numTexels)
	{
		Color_R8G8B8A8 inTexel = input[i];
		Color outTexel;
		outTexel.r = (float)inTexel.r / scale;
		outTexel.g = (float)inTexel.g / scale;
		outTexel.b = (float)inTexel.b / scale;
		outTexel.a = (float)inTexel.a / scale;
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
		outTexel.r = (u8)(clamp(inTexel.r, 0.0, 1.0) * scale);
		outTexel.g = (u8)(clamp(inTexel.g, 0.0, 1.0) * scale);
		outTexel.b = (u8)(clamp(inTexel.b, 0.0, 1.0) * scale);
		outTexel.a = (u8)(clamp(inTexel.a, 0.0, 1.0) * scale);
		output[i] = outTexel;
	}
}


