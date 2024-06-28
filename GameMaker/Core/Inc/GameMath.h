#pragma once

#include <cstdint>
#include <limits>


namespace GameMaker
{
	/**
	 * @brief 파이(π) 값입니다.
	 */
	const float PI = 3.141592654f;


	/**
	 * @brief 2파이(2π) 값입니다.
	 */
	const float TWO_PI = 6.283185307f;


	/**
	 * @brief 파이(π) 의 역수 값입니다.
	 */
	const float ONE_DIV_PI = 0.318309886f;


	/**
	 * @brief 2파이(2π) 의 역수 값입니다.
	 */
	const float ONE_DIV_2PI = 0.159154943f;


	/**
	 * @brief 파이(π) 의 반값(π/2)입니다.
	 */
	const float PI_DIV_2 = 1.570796327f;


	/**
	 * @brief 파이(π) 의 반의 반 값(π/4)입니다.
	 */
	const float PI_DIV_4 = 0.785398163f;


	/**
	 * @brief 엡실론(ε) 값입니다.
	 */
	const float EPSILON = 1.192092896e-07F;


	/**
	 * @brief 라디안 각을 육십분법 각으로 변환합니다.
	 *
	 * @param radian 변환할 라디안 각입니다.
	 *
	 * @return 변환된 육십분법 각입니다.
	 */
	__forceinline float ToDegree(float radian)
	{
		return (radian * 180.0f) / PI;
	}


	/**
	 * @brief 육십분법 각을 라디안 각으로 변환합니다.
	 *
	 * @param degree 변환할 육십분법 각입니다.
	 *
	 * @return 변환된 라디안 각입니다.
	 */
	__forceinline float ToRadian(float degree)
	{
		return (degree * PI) / 180.0f;
	}
}