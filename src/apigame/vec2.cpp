
#include "vec2.h"

vec2::vec2()
{
}

vec2::vec2(float x, float y)
{
	this->x = x;
	this->y = y;
}

bool vec2::normalize()
{
	float norm = this->norm();
	if(norm < .00001f)
		return false;
	this->x /= norm;
	this->y /= norm;
	return true;
}

float vec2::norm()
{
	return sqrt(vec2::prod(*this, *this));
}

float vec2::prod(vec2& v1, vec2& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

float vec2::dist(vec2& p)
{
	float t1 = p.x - this->x;
	float t2 = p.y - this->y;
	return sqrt(t1*t1 + t2*t2);
}

void vec2::getLine(vec2 &p1, vec2 &p2, float &m, float &b)
{
	m = (p2.y - p1.y) / (p2.x - p1.x);
	b = p1.y - m*p1.x;
}

void vec2::projPointToLine(vec2 &p1, float m, float b, vec2 &proj)
{
	float m_ = - 1.0f/m;
	float b_ = p1.y - m_*p1.x;
	proj.x = (b_-b)/(m-m_);
	proj.y = m*proj.x + b;
}

void vec2::setRandom(float xMin, float xMax, float yMin, float yMax)
{
	this->x = ((float) rand() / ((float)RAND_MAX)) * (xMax-xMin);
	this->x += xMin;
	this->y = ((float) rand() / ((float)RAND_MAX)) * (yMax-yMin);
	this->y += yMin;
}


vec2 vec2::operator+(const vec2 &p2)
{
	vec2 p;
	p.x = this->x + p2.x;
	p.y = this->y + p2.y;
	return p;
}

vec2 vec2::operator-(const vec2 &p2)
{
	vec2 p;
	p.x = this->x - p2.x;
	p.y = this->y - p2.y;
	return p;
}

vec2 vec2::operator*(const vec2 &p2)
{
	vec2 p;
	p.x = this->x * p2.x;
	p.y = this->y * p2.y;
	return p;
}

vec2 vec2::operator*(const float d)
{
	vec2 p;
	p.x = this->x * d;
	p.y = this->y * d;
	return p;
}

vec2 vec2::operator/(const float d)
{
	vec2 p;
	p.x = this->x / d;
	p.y = this->y / d;
	return p;
}

vec2 vec2::operator+=(const vec2 &p2)
{
	this->x += p2.x;
	this->y += p2.y;
	return *this;
}

vec2 vec2::operator-=(const vec2 &p2)
{
	this->x -= p2.x;
	this->y -= p2.y;
	return *this;
}

vec2 vec2::operator*=(const vec2 &p2)
{
	this->x *= p2.x;
	this->y *= p2.y;
	return *this;
}

vec2 vec2::operator*=(const float d)
{
	this->x *= d;
	this->y *= d;
	return *this;
}

vec2 vec2::operator/=(const float d)
{
	this->x /= d;
	this->y /= d;
	return *this;
}

vec2 vec2::operator = (const vec2 &p)
{
	this->x = p.x;
	this->y = p.y;
	return *this;
}

bool vec2::operator == (const vec2 &p) const
{
    return this->x == p.x && this->y == p.y;
}








