#ifndef _vec2_h_
#define _vec2_h_

class vec2;

#include <math.h>
#include <stdlib.h>

class vec2
{
	private:
		
	public:
		
		float x, y;
		
		vec2();
		vec2(float x, float y);

		bool normalize();
		float norm();
		float prod(vec2& v1, vec2& v2);
		float dist(vec2& p);

		static void getLine(vec2 &p1, vec2 &p2, float &m, float &b);
		static void projPointToLine(vec2 &p1, float m, float b, vec2 &proj);
		
		void setRandom(float xMin, float xMax, float yMin, float yMax);

		vec2 operator+(const vec2 &p2);
		vec2 operator-(const vec2 &p2);
		vec2 operator*(const vec2 &p2);
		vec2 operator*(const float d);
		vec2 operator/(const float d);
		vec2 operator+=(const vec2 &p2);
		vec2 operator-=(const vec2 &p2);
		vec2 operator*=(const vec2 &p2);
		vec2 operator*=(const float d);
		vec2 operator/=(const float d);
		vec2 operator = (const vec2 &p);
		bool operator == (const vec2 &p) const;
};

#endif
