namespace ImageProcessing
{
	float Brightness(float col, float e)
	{
		col = 1.0 - pow((1.0 - col), e);
		col = saturate(col);

		return col;
	}

	float Exposure(float col, float e, float offset, float gammaInv)
	{
		col *= pow(2.0, e);
		col = saturate(col + offset);
		col = pow(col, gammaInv);

		return col;
	}

	float Invert(float col)
	{
		return 1.0 - col;
	}

	float Clamp(float col, float min, float max)
	{
		return clamp(col, min, max);
	}
};
