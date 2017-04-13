#ifndef _MYPOINT_H_
#define _MYPOINT_H_

#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;

#pragma comment( lib, "gdiplus.lib" )

class MyPoint : public Point
{
public:
	MyPoint()
	{
		X = Y = 0;
	}

	MyPoint(IN const MyPoint &point)
	{
		X = point.X;
		Y = point.Y;
	}

	MyPoint(IN const Size &size)
	{
		X = size.Width;
		Y = size.Height;
	}

	MyPoint(IN INT x,
		IN INT y)
	{
		X = x;
		Y = y;
	}

	BOOL operator==(IN const Point& point)
	{
		return (X == point.X) && (Y == point.Y);
	}

public:

	INT X;
	INT Y;
};

#endif