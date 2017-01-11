#include <gfx/color.h>
#include <gfx/mat3.h>

namespace gfx
{
    //
    // HSV conversion code based on Foley et al (2nd ed. in C, pp. 592-593)
    //

    static double max3(double x, double y, double z)
    {
	if( x>=y && x>=z )  return x;
	else if( y>=x && y>=z )  return y;
	else return z;
    }

    static double min3(double x, double y, double z)
    {
	if( x<=y && x<=z )  return x;
	else if( y<=x && y<=z )  return y;
	else return z;
    }

    hsvColor RGBtoHSV(const rgbColor& rgb)
    {
	double r=rgb[0],  g=rgb[1],  b=rgb[2];

	double max = max3(r,g,b);
	double min = min3(r,g,b);
	double delta = max - min;

	double h = -1;  // undefined value for case where v outside [0,360]
	double v = max;
	double s = (max!=0) ? (delta/max) : 0;

	if( s != 0 )
	{
	    if(      r==max )  h = (g-b)/delta;
	    else if( g==max )  h = 2 + (b-r)/delta;
	    else if( b==max )  h = 4 + (r-g)/delta;

	    h *= 60;
	    if( h<0 )  h+=360;
	}

	return hsvColor(h, s, v);
    }

    rgbColor HSVtoRGB(const hsvColor& hsv)
    {
	double h = hsv[0],  s=hsv[1],  v=hsv[2];

	// Unsaturated means pure gray
	if(s == 0)  return rgbColor(v, v, v);

	if( h==360.0 )  h=0.0;      // these are equivalent hues
	h /= 60.0;                  // convert to sector [0, 6)
	int i = (int)floor( h );    // integral part of h
	float f = h - i;            // fractional part of h

	// Compute RGB components
	float p = v * ( 1 - s );
	float q = v * ( 1 - s * f );
	float t = v * ( 1 - s * ( 1 - f ) );

	// Map PQT to RGB based on sector of the color cone
	switch( i )
	{
	case 0:  return rgbColor(v, t, p); break;
	case 1:  return rgbColor(q, v, p); break;
	case 2:  return rgbColor(p, v, t); break;
	case 3:  return rgbColor(p, q, v); break;
	case 4:  return rgbColor(t, p, v); break;
	default: return rgbColor(v, p, q); break;
	}
    }

    // Paul Haeberli's comments on luminance coefficients:
    //
    //    Where rwgt is 0.3086, gwgt is 0.6094, and bwgt is 0.0820. This
    //    is the luminance vector. Notice here that we do not use the
    //    standard NTSC weights of 0.299, 0.587, and 0.114. The NTSC
    //    weights are only applicable to RGB colors in a gamma 2.2 color
    //    space. For linear RGB colors the values above are better.
    //
    // Grafica Obscura -- Matrix Operations for Image Processing
    //    http://www.sgi.com/misc/grafica/matrix/index.html
    //
    const rgbColor haeberli_factor(0.3086, 0.6094, 0.0820);
    const rgbColor ntsc_factor(0.299, 0.587, 0.114);

    float rgb_luminance_ntsc(const rgbColor& rgb) { return rgb*ntsc_factor; }
    float rgb_luminance_alt(const rgbColor& rgb) { return rgb*haeberli_factor; }


    const Mat3 M_yiq(ntsc_factor,
		     Vec3(0.596, -0.275, -0.321),
		     Vec3(0.212, -0.523,  0.311));

    yiqColor RGBtoYIQ(const rgbColor& rgb)
    {
	return yiqColor(M_yiq * Vec3(rgb));
    }

    // Matrix conversions taken from the ColorFAQ by Charles Poynton
    const Mat3 M_rgb(Vec3( 3.240479, -1.537150, -0.498535),
                     Vec3(-0.969256,  1.875992,  0.041556),
                     Vec3( 0.055648, -0.204043,  1.057311));

    const Mat3 M_xyz(Vec3(0.412453, 0.357580, 0.180423),
                     Vec3(0.212671, 0.715160, 0.072169),
                     Vec3(0.019334, 0.119193, 0.950227));

    // Derived from equations provided by EasyRGB:
    //     http://www.easyrgb.com/math.php
    //
    xyzColor RGBtoXYZ(const rgbColor& rgb)
        { return xyzColor(M_xyz * Vec3(rgb)); }

    rgbColor XYZtoRGB(const xyzColor& xyz)
        { return rgbColor(M_rgb * Vec3(xyz)); }

    xyChromaticity xyz_chromaticity(const xyzColor& xyz)
    {
        float w = xyz[0] + xyz[1] + xyz[2];
        return xyChromaticity(xyz[0]/w, xyz[1]/w);
    }
}
