#include"colorspace.h"

RgbFColor RgbF_Create(double r, double g, double b)
{
    RgbFColor color;
    if (RgbF_IsValid(r, g, b) == true)
    {
			color.R = r;
			color.G = g;
			color.B = b;
    }
    return color;
}

RgbIColor RgbI_Create(uint8_t r, uint8_t g, uint8_t b)
{
    RgbIColor color;
    if (RgbI_IsValid(r, g, b) == true)
    {
			color.R = r;
			color.G = g;
			color.B = b;
    }
    return color;
}

HsiColor Hsi_Create(double h, double s, double i)
{
    HsiColor color;
    if (Hsi_IsValid(h, s, i) == true)
    {
			color.H = h;
			color.S = s;
			color.I = i;
    }
    return color;
}

HslColor Hsl_Create(double h, double s, double l)
{
    HslColor color;
    if (Hsl_IsValid(h, s, l) == true)
    {
			color.H = h;
			color.S = s;
			color.L = l;
    }
    return color;
}

HsvColor Hsv_Create(double h, double s, double v)
{
    HsvColor color;
    if (Hsv_IsValid(h, s, v) == true)
    {
			color.H = h;
			color.S = s;
			color.V = v;
    }
    return color;
}

YiqColor Yiq_Create(double y, double i, double q)
{
    YiqColor color;
    if (Yiq_IsValid(y, i, q) == true)
    {
			color.Y = y;
			color.I = i;
			color.Q = q;
    }
    return color;
}

YuvColor Yuv_Create(double y, double u, double v)
{
    YuvColor color;
    if (Yiq_IsValid(y, u, v) == true)
    {
			color.Y = y;
			color.U = u;
			color.V = v;
    }
    return color;
}

