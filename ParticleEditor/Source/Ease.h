#pragma once

#include <DirectXMath.h>
#include <math.h>

using namespace DirectX;

#define EASE_FUNC(type, name, factor) inline type name(type start, type end, float t) \
{ \
	return Lerp(start, end, factor(t)); \
}

namespace ease {

inline float Lerp(float start, float end, float t)
{
	return start + t * (end - start);
}

inline XMVECTOR Lerp(XMVECTOR start, XMVECTOR end, float t)
{
	return XMVectorLerp(start, end, t);
}

inline float EaseInFactor(float t)
{
	return powf(t, 5);
}

inline float EaseOutFactor(float t)
{
	return 1 - powf(1 - t, 5);
}

EASE_FUNC(float, EaseIn, EaseInFactor)
EASE_FUNC(XMVECTOR, EaseIn, EaseInFactor)

EASE_FUNC(float, EaseOut, EaseOutFactor)
EASE_FUNC(XMVECTOR, EaseOut, EaseOutFactor)

}