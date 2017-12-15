#include <xmmintrin.h>

class Float4{
	union {
		float w[4];
		__m128 m128;
	};
public:
	Float4(float f):m128(_mm_set1_ps(f)){}
	Float4():m128(_mm_setzero_ps()){}
	Float4(__m128 m):m128(m){}
	Float4(float w0, float w1,float w2, float w3){
		w[0] = w0;
		w[1] = w1;
		w[2] = w2;
		w[3] = w3;
	}
	void operator=(const Float4& f) { m128 = f.m128; }

	Float4 operator+(const Float4& f)const { return _mm_add_ps(m128, f.m128); }
	Float4 operator-(const Float4& f)const { return _mm_sub_ps(m128, f.m128); }
	Float4 operator*(const Float4& f)const { return _mm_mul_ps(m128, f.m128); }
	Float4 operator/(const Float4& f)const { return _mm_div_ps(m128, f.m128); }

	void operator+=(const Float4& f) { m128 = _mm_add_ps(m128, f.m128); }
	void operator-=(const Float4& f) { m128 = _mm_sub_ps(m128, f.m128); }
	void operator*=(const Float4& f) { m128 = _mm_mul_ps(m128, f.m128); }
	void operator/=(const Float4& f) { m128 = _mm_div_ps(m128, f.m128); }

	float sum()const { return w[0] + w[1] + w[2] + w[3]; }
	Float4 sqrt()const {
		return _mm_sqrt_ps(m128);
	}
	void clear(){m128 = _mm_setzero_ps();}
	float operator[](int i)const { return w[i]; }
	float& operator[](int i){ return w[i]; }
};
