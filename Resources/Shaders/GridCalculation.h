float log10Calc(float x)
{
	return log(x) / log(10);
}

float satf(float x)
{
	return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x)
{
	return clamp(x + 0.5, vec2(0.0), vec2(1.0));
}

float max2(vec2 v)
{
	return max(v.x, v.y);
}

vec4 gridColor(vec2 uv, vec2 camPos)
{
	vec2 dudv = vec2(
		length(vec2(dFdx(uv.x), dFdy(uv.x))),
		length(vec2(dFdx(uv.y), dFdy(uv.y)))
	);

	float lodLevel = max(0.0, log10Calc((length(dudv) * gridMinPixelsBetweenCells) / gridMajorSize) + 1.0);
	float lodFade = fract(lodLevel);

	// cell sizes for lod0, lod1 and lod2
	float lod0 = gridMajorSize * pow(10.0, floor(lodLevel));
	float lodA1 = lod0 * gridMinorSize;
	float lodA2 = lodA1 * gridMinorSize;

	float lodB1 = -lod0 * gridMinorSize;
	float lodB2 = lodB1 * gridMinorSize;

	// each anti-aliased line covers up to 4 pixels
	dudv *= 6.0;

	// calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value
	float lod0a = max2(vec2(1.0) - abs(satv(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)));
	float lodA1a = max2(vec2(1.0) - abs(satv(mod(uv, lodA1) / dudv) * 2.0 - vec2(1.0)));
	float lodA2a = max2(vec2(1.0) - abs(satv(mod(uv, lodA2) / dudv) * 2.0 - vec2(1.0)));

	float lodB1a = max2(vec2(1.0) - abs(satv(mod(uv, lodB1) / dudv) * 2.0 - vec2(1.0)));
	float lodB2a = max2(vec2(1.0) - abs(satv(mod(uv, lodB2) / dudv) * 2.0 - vec2(1.0)));


	uv -= camPos;

	// blend between falloff colors to handle LOD transition
	vec4 c1 = lodA2a > 0.0 ? gridColorThick : lodA1a > 0.0 ? mix(gridColorThick, gridColorThin, lodFade) : gridColorThin;
	vec4 c2 = lodB2a > 0.0 ? gridColorThick : lodB1a > 0.0 ? mix(gridColorThick, gridColorThin, lodFade) : gridColorThin;

	vec4 c = min(c1, c2);

	// calculate opacity falloff based on distance to grid extents
	float opacityFalloff = (1.0 - satf(length(uv) / gridSize));

	// blend between LOD level alphas and scale with opacity falloff
	c1.a *= (lodA2a > 0.0 ? lodA2a : lodA1a > 0.0 ? lodA1a : (lod0a * (1.0 - lodFade)));
	c2.a *= (lodB2a < 0.0 ? lodB2a : lodB1a > 0.0 ? lodB1a : (lod0a * (1.0 - lodFade)));

	c.a = max(c1.a, c2.a);

	return c;
}

