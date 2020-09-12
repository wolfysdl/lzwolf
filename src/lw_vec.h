/*
** lw_vec.h
**
**---------------------------------------------------------------------------
** Copyright 2020 Linux Wolf
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#ifndef LWLIB_VEC_H
#define LWLIB_VEC_H

#include <algorithm>
#include <iostream>
#include <cmath>

/* common */

#define lwlib_EPSILON 0.00001
#define lwlib_MIN(a,b) ((a) < (b) ? (a) : (b))
#define lwlib_MAX(a,b) ((a) > (b) ? (a) : (b))
#define lwlib_MINMAX(a,b,c) ((c) ? ((a) > (b) ? (a) : (b)) : ((a) < (b) ? (a) : (b)))
#define lwlib_CLIP(x,a,b) lwlib_MIN(lwlib_MAX(x,a), b)

/* vector math */

/* access named component of vector */
#define lwlib_X(a) (a).v[0]
#define lwlib_Y(a) (a).v[1]
#define lwlib_Z(a) (a).v[2]
#define lwlib_W(a) (a).v[3]

/* access numbered component of vector */
#define lwlib_C(a, i) (a).v[i]

/* access mapped, numbered component of vector */ 
#define lwlib_Cm(a, i, m) lwlib_C(a, (m)[i])

#define lwlib_AddVector2(c,a,b) lwlib_X(c)=lwlib_X(a)+lwlib_X(b), lwlib_Y(c)=lwlib_Y(a)+lwlib_Y(b)
#define lwlib_SubVector2(c,a,b) lwlib_X(c)=lwlib_X(a)-lwlib_X(b), lwlib_Y(c)=lwlib_Y(a)-lwlib_Y(b)
#define lwlib_DotVector2(a,b) (lwlib_X(a)*lwlib_X(b) + lwlib_Y(a)*lwlib_Y(b))
#define lwlib_ScaleVector2(c,a,b) lwlib_X(c)=lwlib_X(a)*(b), lwlib_Y(c)=lwlib_Y(a)*(b)
#define lwlib_DivideVector2(c,a,b) lwlib_X(c)=lwlib_X(a)/(b), lwlib_Y(c)=lwlib_Y(a)/(b)
#define lwlib_NormalizeVector2(n,a) \
	( \
		(n)=sqrt(lwlib_DotVector2(a,a)), \
		(n) ? \
			(lwlib_X(a)/=n, lwlib_Y(a)/=n) : \
			0 \
	)
#define lwlib_InitVector2(v,a,b) lwlib_X(v) = (a), lwlib_Y(v) = (b)
#define lwlib_InvertVector2(a,b) lwlib_X(a) = -lwlib_X(b), lwlib_Y(a) = -lwlib_Y(b)
#define lwlib_MultVector2(c,a,b) lwlib_X(c) = lwlib_X(a) * lwlib_X(b), lwlib_Y(c) = lwlib_Y(a) * lwlib_Y(b)
#define lwlib_VectorLength2(v) sqrt(lwlib_DotVector2(v,v))
#define lwlib_VectorLengthSq2(v) lwlib_DotVector2(v,v)
#define lwlib_TransformVector2(a,b,f) lwlib_X(a) = f(lwlib_X(b)), lwlib_Y(a) = f(lwlib_Y(b))
#define lwlib_PackVector2(a) (a)[0], (a)[1]
#define lwlib_UnpackVector2(a, b) (b)[0] = lwlib_X(a), (b)[1] = lwlib_Y(a)
#define lwlib_ExpandVector2(a) lwlib_X(a), lwlib_Y(a)

#define lwlib_vec2_init(x,y) { { (x), (y) } }

#define lwlib_AddVector(c,a,b) lwlib_X(c)=lwlib_X(a)+lwlib_X(b), lwlib_Y(c)=lwlib_Y(a)+lwlib_Y(b), lwlib_Z(c)=lwlib_Z(a)+lwlib_Z(b)
#define lwlib_SubVector(c,a,b) lwlib_X(c)=lwlib_X(a)-lwlib_X(b), lwlib_Y(c)=lwlib_Y(a)-lwlib_Y(b), lwlib_Z(c)=lwlib_Z(a)-lwlib_Z(b)
#define lwlib_CrossVector(c,a,b)	lwlib_X(c) = lwlib_Y(a)*lwlib_Z(b) - lwlib_Z(a)*lwlib_Y(b), \
                				lwlib_Y(c) = lwlib_Z(a)*lwlib_X(b) - lwlib_X(a)*lwlib_Z(b), \
                				lwlib_Z(c) = lwlib_X(a)*lwlib_Y(b) - lwlib_Y(a)*lwlib_X(b)
#define lwlib_DotVector(a,b) (lwlib_X(a)*lwlib_X(b) + lwlib_Y(a)*lwlib_Y(b) + lwlib_Z(a)*lwlib_Z(b))
#define lwlib_ScaleVector(c,a,b) lwlib_X(c)=lwlib_X(a)*(b), lwlib_Y(c)=lwlib_Y(a)*(b), lwlib_Z(c)=lwlib_Z(a)*(b)
#define lwlib_DivideVector(c,a,b) lwlib_X(c)=lwlib_X(a)/(b), lwlib_Y(c)=lwlib_Y(a)/(b), lwlib_Z(c)=lwlib_Z(a)/(b)
#define lwlib_NormalizeVector(n,a) \
	( \
		(n)=sqrt(lwlib_DotVector(a,a)), \
		(n) ? \
			(lwlib_X(a)/=n, lwlib_Y(a)/=n, lwlib_Z(a)/=n) : \
			0 \
	)
#define lwlib_InitVector(v,a,b,c) lwlib_X(v) = (a), lwlib_Y(v) = (b), lwlib_Z(v) = (c)
#define lwlib_InvertVector(a,b) lwlib_X(a) = -lwlib_X(b), lwlib_Y(a) = -lwlib_Y(b), lwlib_Z(a) = -lwlib_Z(b)
#define lwlib_MultVector(c,a,b) lwlib_X(c) = lwlib_X(a) * lwlib_X(b), lwlib_Y(c) = lwlib_Y(a) * lwlib_Y(b), lwlib_Z(c) = lwlib_Z(a) * lwlib_Z(b)
#define lwlib_VectorLength(v) sqrt(lwlib_DotVector(v,v))
#define lwlib_VectorLengthSq(v) lwlib_DotVector(v,v)
#define lwlib_TransformVector(a,b,f) lwlib_X(a) = f(lwlib_X(b)), lwlib_Y(a) = f(lwlib_Y(b)), lwlib_Z(a) = f(lwlib_Z(b))
#define lwlib_PackVector(a) (a)[0], (a)[1], (a)[2]
#define lwlib_UnpackVector(a, b) (b)[0] = lwlib_X(a), (b)[1] = lwlib_Y(a), (b)[2] = lwlib_Z(a)
#define lwlib_ExpandVector(a) lwlib_X(a), lwlib_Y(a), lwlib_Z(a)

#define lwlib_vec3_init(x,y,z) { { (x), (y), (z) } }

#define lwlib_AddVector4(c,a,b) lwlib_X(c)=lwlib_X(a)+lwlib_X(b), lwlib_Y(c)=lwlib_Y(a)+lwlib_Y(b), lwlib_Z(c)=lwlib_Z(a)+lwlib_Z(b), lwlib_W(c)=lwlib_W(a)+lwlib_W(b)
#define lwlib_SubVector4(c,a,b) lwlib_X(c)=lwlib_X(a)-lwlib_X(b), lwlib_Y(c)=lwlib_Y(a)-lwlib_Y(b), lwlib_Z(c)=lwlib_Z(a)-lwlib_Z(b), lwlib_W(c)=lwlib_W(a)-lwlib_W(b)
#define lwlib_DotVector4(a,b) (lwlib_X(a)*lwlib_X(b) + lwlib_Y(a)*lwlib_Y(b) + lwlib_Z(a)*lwlib_Z(b) + lwlib_W(a)*lwlib_W(b))
#define lwlib_ScaleVector4(c,a,b) lwlib_X(c)=lwlib_X(a)*(b), lwlib_Y(c)=lwlib_Y(a)*(b), lwlib_Z(c)=lwlib_Z(a)*(b), lwlib_W(c)=lwlib_W(a)*(b)
#define lwlib_DivideVector4(c,a,b) lwlib_X(c)=lwlib_X(a)/(b), lwlib_Y(c)=lwlib_Y(a)/(b), lwlib_Z(c)=lwlib_Z(a)/(b), lwlib_W(c)=lwlib_W(a)/(b)
#define lwlib_NormalizeVector4(n,a) \
	( \
		(n)=sqrt(lwlib_DotVector4(a,a)), \
		(n) ? \
			(lwlib_X(a)/=n, lwlib_Y(a)/=n, lwlib_Z(a)/=n, lwlib_W(a)/=n) : \
			0 \
	)
#define lwlib_InitVector4(v,a,b,c,d) lwlib_X(v) = (a), lwlib_Y(v) = (b), lwlib_Z(v) = (c), lwlib_W(v) = (d)
#define lwlib_InvertVector4(a,b) lwlib_X(a) = -lwlib_X(b), lwlib_Y(a) = -lwlib_Y(b), lwlib_Z(a) = -lwlib_Z(b), lwlib_W(a) = -lwlib_W(b)
#define lwlib_MultVector4(c,a,b) lwlib_X(c) = lwlib_X(a) * lwlib_X(b), lwlib_Y(c) = lwlib_Y(a) * lwlib_Y(b), lwlib_Z(c) = lwlib_Z(a) * lwlib_Z(b), lwlib_W(c) = lwlib_W(a) * lwlib_W(b)
#define lwlib_VectorLength4(v) sqrt(lwlib_DotVector4(v,v))
#define lwlib_VectorLengthSq4(v) lwlib_DotVector4(v,v)
#define lwlib_TransformVector4(a,b,f) lwlib_X(a) = f(lwlib_X(b)), lwlib_Y(a) = f(lwlib_Y(b)), lwlib_Z(a) = f(lwlib_Z(b)), lwlib_W(a) = f(lwlib_W(b))
#define lwlib_PackVector4(a) (a)[0], (a)[1], (a)[2], (a)[3]
#define lwlib_UnpackVector4(a, b) (b)[0] = lwlib_X(a), (b)[1] = lwlib_Y(a), (b)[2] = lwlib_Z(a), (b)[3] = lwlib_W(a)
#define lwlib_ExpandVector4(a) lwlib_X(a), lwlib_Y(a), lwlib_Z(a), lwlib_W(a)

#define lwlib_vec4_init(x,y,z,w) { { (x), (y), (z), (w) } }

#define lwlib_vec4b_black lwlib_vec4b(0x00, 0x00, 0x00, 0xff)
#define lwlib_vec4b_red lwlib_vec4b(0xff, 0x00, 0x00, 0xff)
#define lwlib_vec4b_green lwlib_vec4b(0x00, 0xff, 0x00, 0xff)
#define lwlib_vec4b_yellow lwlib_vec4b(0xff, 0xff, 0x00, 0xff)
#define lwlib_vec4b_blue lwlib_vec4b(0x00, 0x00, 0xff, 0xff)
#define lwlib_vec4b_cyan lwlib_vec4b(0x00, 0xff, 0xff, 0xff)
#define lwlib_vec4b_magenta lwlib_vec4b(0xff, 0x00, 0xff, 0xff)
#define lwlib_vec4b_white lwlib_vec4b(0xff, 0xff, 0xff, 0xff)

#define lwlib_plane4f lwlib_vec4f_pair

namespace lwlib
{
	template <typename T, int N>
	class Point
	{
	public:
		T v[N];

		Point()
		{
			reset();
		}

		template <int M>
		Point(const Point<T, M> &a)
		{
			reset();
			const int n = std::min(M, N);
			std::copy(a.v, a.v + n, v);
		}

		void reset()
		{
			memset(v, 0, sizeof(v));
		}

		bool operator<(const Point<T, N> &point) const
		{
			int i;
			for (i = 0; i < N; i++)
			{
				if (v[i] < point.v[i])
				{
					return true;
				}
			}

			return false;
		}
	};

	template <typename T, int N>
	std::ostream &operator<<(std::ostream &os, const Point<T, N> &x)
	{
		int i;
		os << std::string("(");
		for (i = 0; i < N; i++)
		{
			os << std::string(" ") << x.v[i];
		}
		os << std::string(" )");
		return os;
	}

	typedef Point<double, 2> Point2f;
	typedef Point<double, 3> Point3f;
	typedef Point<double, 4> Point4f;
	typedef Point<int, 2> Point2i;
	typedef Point<int, 3> Point3i;
	typedef Point<int, 4> Point4i;
	typedef Point<unsigned char, 3> Point3b;
	typedef Point<unsigned char, 4> Point4b;

	typedef Point<double, 2> Vector2f;
	typedef Point<double, 3> Vector3f;
	typedef Point<double, 4> Vector4f;
	typedef Point<int, 2> Vector2i;
	typedef Point<int, 3> Vector3i;

	template <typename T>
	T clip(const T& n, const T& lower, const T& upper)
	{
		return std::max(lower, std::min(n, upper));
	}

	template <typename T>
	T clipl(T& n, const T& lower, const T& upper)
	{
		n = clip(n, lower, upper);
		return n;
	}

	template <typename T>
	T clip(const T& n, const Point<T, 2> &range)
	{
		return clip(n, range.v[0], range.v[1]);
	}

	template <typename T>
	bool inrange(const T& n, const T& lower, const T& upper)
	{
		return clip(n, lower, upper) == n;
	}

	template <typename T>
	bool indelta(const T& n, const T& delta)
	{
		return inrange(n, -delta, delta);
	}

	template <typename T>
	bool inrange(const T& n, const Point<T, 2> &range)
	{
		return inrange(n, range.v[0], range.v[1]);
	}

	static inline double lerp(double a, double b, double t)
	{
		return a * (1 - t) + b * t;
	}

	static inline int lerpi(int a, int b, int t, int d)
	{
		return (a * (d - t) + b * t) / d;
	}

	static inline double lerp2(double q[4], double u, double v)
	{
		return q[0] * (1-u)*(1-v) + q[1] * (1-v)*u + q[2] * u*v + 
			q[3] * v*(1-u);
	}

	template <typename T, typename R, int N, int M>
	const Point<R, M> vec_convert(const Point<T, N> &a)
	{
		int i;
		Point<R, M> b;

		for (i = 0; i < (N < M ? N : M); i++)
		{
			b.v[i] = (R)a.v[i];
		}

		return b;
	}

	template <typename T, int N>
	Point<T, N> vec_convert(const Point<T, N> &a)
	{
		return a;
	}

	template <typename T, typename R, int N, int M>
	void vec_convert(const Point<T, N> &a, Point<R, M> &b)
	{
		b = vec_convert<T, R, N, M>(a);
	}

	template <typename T>
	Point<T, 2> vec2(T x, T y)
	{
		Point<T, 2> a;
		a.v[0] = x;
		a.v[1] = y;
		return a;
	}

	template <typename T, int N>
	const Point<T, 2> vec2(const Point<T, N> &a)
	{
		return vec2(a.v[0], a.v[1]);
	}

	template <typename T>
	Point<T, 3> vec3(T x, T y, T z)
	{
		Point<T, 3> a;
		a.v[0] = x;
		a.v[1] = y;
		a.v[2] = z;
		return a;
	}

	template <typename T, int N>
	const Point<T, 3> vec3(const Point<T, N> &p)
	{
		return vec_convert<T, T, N, 3>(p);
	}

	template <typename T>
	Point<T, 4> vec4(T x, T y, T z, T w)
	{
		Point<T, 4> a;
		a.v[0] = x;
		a.v[1] = y;
		a.v[2] = z;
		a.v[3] = w;
		return a;
	}

	template <typename T, int N>
	const Point<T, 4> vec4(const Point<T, N> &p)
	{
		return vec_convert<T, T, N, 4>(p);
	}

	template <typename T, int N>
	const Point<int, N> vec_int(const Point<T, N> &a)
	{
		return vec_convert<T, int, N, N>(a);
	}

	template <typename T, int N>
	const Point<double, N> vec_double(const Point<T, N> &a)
	{
		return vec_convert<T, double, N, N>(a);
	}

	template <typename R, typename T, int N>
	const Point<R, N> vec_typeconv(const Point<T, N> &a)
	{
		return vec_convert<T, R, N, N>(a);
	}

	static inline Point2f vec2f(double x, double y)
	{
		return vec2(x, y);
	}

	static inline Point2i vec2i(int x, int y)
	{
		return vec2(x, y);
	}

	static inline Point2i vec2i(Point3f p)
	{
		Point<int, 2> a;
		a.v[0] = (int)p.v[0];
		a.v[1] = (int)p.v[1];
		return a;
	}

	static inline Point3i vec3i(int x, int y, int z)
	{
		return vec3(x, y, z);
	}

	static inline Point3f vec3f(double x, double y, double z)
	{
		return vec3(x, y, z);
	}

	static inline Point4i vec4i(int x, int y, int z, int w)
	{
		return vec4(x, y, z, w);
	}

	static inline Point4f vec4f(double x, double y, double z, double w)
	{
		return vec4(x, y, z, w);
	}

	static inline Point4f vec4f_origin(void)
	{
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	static inline Point4f vec4f_point(const Point3f &p)
	{
		return vec4(p.v[0], p.v[1], p.v[2], 1.0);
	}

	static inline Point4b vec4b(unsigned char x, unsigned char y, 
		unsigned char z, unsigned char w)
	{
		return vec4(x, y, z, w);
	}

	template <typename T, int N>
	Point<T, N> vec_same(T x)
	{
		int i;
		Point<T, N> a;
		for (i = 0; i < N; i++)
		{
			a.v[i] = x;
		}
		return a;
	}

	static inline Point2i vec2i_same(int a)
	{
		return vec_same<int, 2>(a);
	}

	static inline Point3i vec3i_same(int a)
	{
		return vec_same<int, 3>(a);
	}

	static inline Point2f vec2f_same(double a)
	{
		return vec_same<double, 2>(a);
	}

	static inline Point3f vec3f_same(double a)
	{
		return vec_same<double, 3>(a);
	}

	template <typename T, int N>
	Point<T, N> vec_zero()
	{
		return vec_same<T, N>(T());
	}

	template <typename T, int N>
	Point<T, N> vec_ones()
	{
		return vec_same<T, N>((T)1);
	}

	template <typename T, int N>
	const Point<T, N> vec_axial(int i, T x)
	{
		Point<T, N> a;
		a.v[i] = x;
		return a;
	}

	template <typename T>
	const Point<T, 2> vec2_axial(int i, T x)
	{
		return vec_axial<T, 2>(i, x);
	}

	template <typename T>
	const Point<T, 3> vec3_axial(int i, T x)
	{
		return vec_axial<T, 3>(i, x);
	}

	template <typename T>
	const Point<T, 4> vec4_axial(int i, T x)
	{
		return vec_axial<T, 4>(i, x);
	}

	static inline Point2f vec2f_zero()
	{
		return vec_zero<double, 2>();
	}

	static inline Point3f vec3f_zero()
	{
		return vec_zero<double, 3>();
	}

	static inline Point3f vec3f_ones()
	{
		return vec_ones<double, 3>();
	}

	static inline Point2i vec2i_zero()
	{
		return vec_zero<int, 2>();
	}

	static inline Point3i vec3i_zero()
	{
		return vec_zero<int, 3>();
	}

	template <typename T, int N>
	const Point<T, N> vec_add(Point<T, N> a, Point<T, N> b)
	{
		int i;
		Point<T, N> c;
		for (i = 0; i < N; i++)
		{
			c.v[i] = a.v[i] + b.v[i];
		}
		return c;
	}

	template <typename T>
	const Point<T, 3> vec_add(Point<T, 3> a, Point<T, 3> b)
	{
		a.v[0] += b.v[0];
		a.v[1] += b.v[1];
		a.v[2] += b.v[2];
		return a;
	}

	template <typename T, int N>
	Point<T, N> operator+(const Point<T, N> &a, const Point<T, N> &b)
	{
		return vec_add(a, b);
	}

	template <typename T, int N>
	Point<T, N> &operator+=(Point<T, N> &a, const Point<T, N> &b)
	{
		a = a + b;
		return a;
	}

	template <typename T, int N>
	const Point<T, N> vec_sub(Point<T, N> a, Point<T, N> b)
	{
		int i;
		Point<T, N> c;
		for (i = 0; i < N; i++)
		{
			c.v[i] = a.v[i] - b.v[i];
		}
		return c;
	}

	template <typename T>
	const Point<T, 3> vec_sub(Point<T, 3> a, Point<T, 3> b)
	{
		a.v[0] -= b.v[0];
		a.v[1] -= b.v[1];
		a.v[2] -= b.v[2];
		return a;
	}

	template <typename T>
	const Point<T, 2> vec_sub(Point<T, 2> a, Point<T, 2> b)
	{
		a.v[0] -= b.v[0];
		a.v[1] -= b.v[1];
		return a;
	}

	template <typename T, int N>
	Point<T, N> operator-(const Point<T, N> &a, const Point<T, N> &b)
	{
		return vec_sub(a, b);
	}

	template <typename T, int N>
	Point<T, N> &operator-=(Point<T, N> &a, const Point<T, N> &b)
	{
		a = a - b;
		return a;
	}

	template <typename T, int N>
	Point<T, N> operator-(const Point<T, N> &a)
	{
		return vec_neg(a);
	}

	template <typename T, int N>
	bool vec_nonzero(const Point<T, N> &a)
	{
		return !vec_equal(a, vec_zero<T, N>(), T());
	}

	template <typename T, int N>
	const Point<T, N> vec_scale(Point<T, N> a, T s)
	{
		int i;
		Point<T, N> b;
		for (i = 0; i < N; i++)
		{
			b.v[i] = a.v[i] * s;
		}
		return b;
	}

	template <typename T>
	const Point<T, 3> vec_scale(Point<T, 3> a, T s)
	{
		a.v[0] *= s;
		a.v[1] *= s;
		a.v[2] *= s;
		return a;
	}

	template <typename T, int N>
	Point<T, N> operator*(T s, const Point<T, N> &a)
	{
		return vec_scale(a, s);
	}

	template <typename T, int N>
	Point<T, N> operator*(const Point<T, N> &a, T s)
	{
		return vec_scale(a, s);
	}

	template <typename T, int N>
	Point<T, N> &operator*=(Point<T, N> &a, T s)
	{
		a = a * s;
		return a;
	}

	template <typename T, int N>
	Point<T, N> operator/(const Point<T, N> &a, T s)
	{
		return vec_divide(a, s);
	}

	template <typename T, int N>
	Point<T, N> &operator/=(Point<T, N> &a, T s)
	{
		a = a / s;
		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_divide(Point<T, N> a, T s)
	{
		int i;
		Point<T, N> b;
		for (i = 0; i < N; i++)
		{
			b.v[i] = a.v[i] / s;
		}
		return b;
	}

	template <typename T, int N>
	Point<T, N> vec_shiftright(Point<T, N> a, T s)
	{
		int i;
		Point<T, N> b;
		for (i = 0; i < N; i++)
		{
			b.v[i] = a.v[i] >> s;
		}
		return b;
	}

	template <typename T, int N>
	Point<T, N> vec_fastmodulo(Point<T, N> a, T s)
	{
		int i;
		Point<T, N> b;
		for (i = 0; i < N; i++)
		{
			b.v[i] = a.v[i] & s;
		}
		return b;
	}

	template <typename T, int N>
	const T vec_dot(Point<T, N> a, Point<T, N> b)
	{
		int i;
		T x = T();
		for (i = 0; i < N; i++)
		{
			x += a.v[i] * b.v[i];
		}
		return x;
	}

	template <typename T>
	const T vec_dot(Point<T, 3> a, Point<T, 3> b)
	{
		return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2];
	}

	template <typename T>
	const T vec_dot(Point<T, 4> a, Point<T, 4> b)
	{
		return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2] +
			a.v[3] * b.v[3];
	}

	template <typename T>
	Point<T, 3> vec3_cross(Point<T, 3> a, Point<T, 3> b)
	{
		Point<T, 3> c;
		lwlib_CrossVector(c, a, b);
		return c;
	}

	template <typename T>
	const T vec2_cross(Point<T, 2> a, Point<T, 2> b)
	{
		return a.v[0] * b.v[1] - a.v[1] * b.v[0];
	}

	template <typename T, int N>
	T vec_length_sq(Point<T, N> a)
	{
		int i;
		T x = T();
		for (i = 0; i < N; i++)
		{
			x += a.v[i] * a.v[i];
		}
		return x;
	}

	template <typename T, int N>
	T vec_length(Point<T, N> a)
	{
		return sqrt(vec_length_sq(a));
	}

	template <typename T, int N>
	T vec_sum(Point<T, N> a)
	{
		int i;
		T x = T();
		for (i = 0; i < N; i++)
		{
			x += a.v[i];
		}
		return x;
	}

	template <typename T, int N>
	T vec_cartesian_length(Point<T, N> a)
	{
		return vec_sum(vec_abs(a));
	}

	template <typename T, int N>
	Point<T, N> vec_normalize(Point<T, N> a)
	{
		T n;

		n = sqrt(vec_dot(a, a));
		if (n != 0.0)
		{
			a = vec_scale(a, 1.0 / n);
		}

		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_trunc_length(Point<T, N> a, double maxlsq)
	{
		double lsq;

		lsq = vec_length_sq(a);
		if (lsq > maxlsq)
		{
			a = vec_normalize(a) * sqrt(maxlsq);
		}

		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_neg(Point<T, N> a)
	{
		int i;
		for (i = 0; i < N; i++)
		{
			a.v[i] = -a.v[i];
		}
		return a;
	}

	template <typename T>
	const Point<T, 2> vec_neg(Point<T, 2> a)
	{
		a.v[0] = -a.v[0];
		a.v[1] = -a.v[1];
		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_mult(Point<T, N> a, Point<T, N> b)
	{
		int i;
		Point<T, N> c;
		for (i = 0; i < N; i++)
		{
			c.v[i] = a.v[i] * b.v[i];
		}
		return c;
	}

	template <typename T, int N>
	Point<T, N> vec_div(Point<T, N> a, Point<T, N> b)
	{
		int i;
		Point<T, N> c;
		for (i = 0; i < N; i++)
		{
			c.v[i] = a.v[i] / b.v[i];
		}
		return c;
	}

	template <typename T, int N>
	Point<T, N> vec_mod(Point<T, N> a, Point<T, N> b)
	{
		int i;
		Point<T, N> c;
		for (i = 0; i < N; i++)
		{
			c.v[i] = a.v[i] % b.v[i];
		}
		return c;
	}


	template <typename T, int N>
	Point<T, N> operator/(const Point<T, N> &a, const Point<T, N> &b)
	{
		return vec_div(a, b);
	}

	template <typename T, int N>
	Point<T, N> operator%(const Point<T, N> &a, const Point<T, N> &b)
	{
		return vec_mod(a, b);
	}

	template <typename T, int N>
	T vec_min(Point<T, N> a)
	{
		int i;
		T min = a.v[0];
		for (i = 1; i < N; i++)
		{
			if (a.v[i] < min)
			{
				min = a.v[i];
			}
		}
		return min;
	}

	template <typename T, int N>
	T vec_max(Point<T, N> a)
	{
		int i;
		T max = a.v[0];
		for (i = 1; i < N; i++)
		{
			if (a.v[i] > max)
			{
				max = a.v[i];
			}
		}
		return max;
	}

	template <typename T, int N>
	int vec_maxind(Point<T, N> a)
	{
		int i;
		T max = a.v[0];
		int maxind = 0;
		for (i = 1; i < N; i++)
		{
			if (a.v[i] > max)
			{
				max = a.v[i];
				maxind = i;
			}
		}
		return maxind;
	}

	template <typename T, int N>
	Point<T, N> vec_clip_trunc(Point<T, N> a)
	{
		T max;

		max = vec_max(a);
		if (max > 1.0) 
		{
			a = vec_scale(a, 1.0 / max);
		}
		
		return a;
	}

	template <typename T, int N>
	T vec_range(Point<T, N> a)
	{
		return a.v[1] - a.v[0];
	}

	template <typename T, int N>
	T vec_prod(Point<T, N> a)
	{
		int i;
		T b = a.v[0];
		for (i = 1; i < N; i++)
		{
			b *= a.v[i];
		}
		return b;
	}

	template <typename T, int N>
	Point<T, N> vec_lerp(Point<T, N> a, Point<T, N> b, T t)
	{
		return vec_add(vec_scale(a, 1 - t), vec_scale(b, t));
	}

	template <typename T, int N>
	const Point<T, N> vec_abs(Point<T, N> a)
	{
		int i;
		for (i = 0; i < N; i++)
		{
			if (a.v[i] < T())
			{
				a.v[i] = -a.v[i];
			}
		}
		return a;
	}

	template <typename T, int N>
	Point<T, 3> vec_cross(Point<T, N> a, Point<T, N> b)
	{
		return vec3_cross(vec3(a), vec3(b));
	}

	template <typename T, int N>
	Point<T, N> vec_sign(Point<T, N> a)
	{
		int i;
		for (i = 0; i < N; i++)
		{
			a.v[i] = (a.v[i] > 0 ? 1 : (a.v[i] < 0 ? -1 : 0));
		}
		return a;
	}

	template <typename T, int N>
	Point<int, N> vec_sign_int_nonzero(Point<T, N> a)
	{
		int i;
		Point<int, N> b;
		for (i = 0; i < N; i++)
		{
			b.v[i] = (a.v[i] > 0 ? 1 : -1);
		}
		return b;
	}

	static inline Point2i vec2f_int(Point2f a)
	{
		return vec_convert<double, int, 2, 2>(a);
	}

	static inline Point2f vec2i_float(Point2i a)
	{
		return vec_convert<int, double, 2, 2>(a);
	}

	static inline Point3i vec3f_int(Point3f a)
	{
		return vec_convert<double, int, 3, 3>(a);
	}

	static inline Point3f vec3i_float(Point3i a)
	{
		return vec_convert<int, double, 3, 3>(a);
	}

	template <typename T, int N>
	Point<T, N> vec_swap(Point<T, N> a)
	{
		int i;
		Point<T, N> b;

		for (i = 0; i < N; i++)
		{
			b.v[i] = a.v[N - i - 1];
		}

		return b;
	}

	template <typename T, int N>
	Point<T, N> vec_transform(Point<T, N> a, T (*f)(T))
	{
		int i;
		for (i = 0; i < N; i++)
		{
			a.v[i] = f(a.v[i]);
		}
		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_floor(Point<T, N> a)
	{
		return vec_transform(a, floor);
	}

	template <typename T, int N>
	Point<int, N> vec_floorint(Point<T, N> a)
	{
		return vec_int(vec_floor(a));
	}

	template <typename T, int N>
	T vec_dist_sq(Point<T, N> a, Point<T, N> b)
	{
		return vec_length_sq(vec_sub(b, a));
	}

	template <typename T, int N>
	void vec_get(const Point<T, N> &a, T &x, T &y)
	{
		x = a.v[0];
		y = a.v[1];
	}

	template <typename T, int N>
	void vec2_get(const Point<T, N> &a, T &x, T &y)
	{
		vec_get(a, x, y);
	}

	template <typename T, int N>
	void vec_get(const Point<T, N> &a, T &x, T &y, T &z)
	{
		x = a.v[0];
		y = a.v[1];
		z = a.v[2];
	}

	template <typename T, int N>
	void vec3_get(const Point<T, N> &a, T &x, T &y, T&z)
	{
		vec_get(a, x, y, z);
	}

	template <typename T, int N>
	void vec4_get(const Point<T, N> &a, T &x, T &y, T&z, T &w)
	{
		x = a.v[0];
		y = a.v[1];
		z = a.v[2];
		w = a.v[3];
	}

	template <typename T, int N>
	bool vec_equal(Point<T, N> a, Point<T, N> b, T eps)
	{
		int i;

		for (i = 0; i < N; i++)
		{
			if (a.v[i] - b.v[i] > eps || a.v[i] - b.v[i] < -eps)
			{
				return false;
			}
		}

		return true;
	}

	template <typename T, int N>
	const Point<T, N> vec_zero_component(Point<T, N> a, int i)
	{
		a.v[i] = T();
		return a;
	}

	template <typename T, int N>
	const Point<T, N> vec_neg_component(Point<T, N> a, int i)
	{
		a.v[i] = -a.v[i];
		return a;
	}

	template <typename T, int N>
	bool operator==(const Point<T, N> &a, const Point<T, N> &b)
	{
		return vec_equal(a, b, T());
	}

	template <typename T, int N>
	bool operator!=(const Point<T, N> &a, const Point<T, N> &b)
	{
		return !vec_equal(a, b, T());
	}

	template <typename T, int N>
	int vec_sign_bits(Point<T, N> a, T b)
	{
		int i;
		int bits = 0;

		for (i = 0; i < N; i++)
		{
			if (a.v[i] >= b)
			{
				bits |= (1 << i);
			}
		}
		
		return bits;
	}

	template <typename T, int N>
	Point<T, N> vec_reflect(Point<T, N> v, Point<T, N> n)
	{
		return vec_sub(vec_scale(n, 2 * vec_dot(v, n)), v);
	}

	template <typename T, int N>
	Point<T, N> vec_rot90(Point<T, N> a)
	{
		std::swap(a.v[0], a.v[1]);
		a.v[0] = -a.v[0];
		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_rot_quadrant(Point<T, N> a, int quadrant)
	{
		for (int i = 0; i < quadrant; i++)
		{
			a = vec_rot90(a);
		}
		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_from_bits(int bits)
	{
		int i;
		Point<T, N> a = vec_zero<T, N>();

		for (i = 0; i < N; i++)
		{
			if (bits & (1 << i))
			{
				a.v[i] = (T)1;
			}
		}

		return a;
	}

	template <typename T, int N>
	Point<T, N> vec_box_inc(Point<T, N> a, Point<T, N> b, Point<T, N> c)
	{
		int i;

		for (i = 0; i < N; i++)
		{
			if (c.v[i] < b.v[i] - 1)
			{
				c.v[i]++;
				break;
			}
			else // c.v[i] == b.v[i] - 1
			{
				c.v[i] = a.v[i];
			}
		}

		return c;
	}

	template <typename T, int N>
	Point<T, N> vec_bounded_inc(Point<T, N> a, Point<T, N> b)
	{
		return vec_box_inc(vec_zero<T, N>(), b, a);
	}

	template <typename T, int N>
	Point<T, N> vec_clip_minmax(Point<T, N> a, Point<T, N> b, Point<T, N> c)
	{
		int i;

		for (i = 0; i < N; i++)
		{
			a.v[i] = lwlib_CLIP(a.v[i], b.v[i], c.v[i]);
		}

		return a;
	}

	template <typename T>
	T wrapangle(T angle, T lim)
	{
		while (angle < 0)
		{
			angle += lim;
		}
		while (angle >= lim)
		{
			angle -= lim;
		}

		return angle;
	}
}

#endif
