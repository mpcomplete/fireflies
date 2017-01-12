/************************************************************************

  Handy 3D geometrical primitives

  $Id: geom3d.cxx 427 2004-09-27 04:45:31Z garland $

 ************************************************************************/

#include <gfx/gfx.h>
#include <gfx/geom3d.h>
#include <gfx/mat4.h>

namespace gfx
{

double tetrahedron_determinant(const Vec3& v0, const Vec3& v1,
			       const Vec3& v2, const Vec3& v3)
{
    Mat4 A( Vec4(v0, 1),
	    Vec4(v1, 1),
	    Vec4(v2, 1),
	    Vec4(v3, 1));

    return det(A);
}

double tetrahedron_volume(const Vec3& v0, const Vec3& v1,
			  const Vec3& v2, const Vec3& v3)
{
    return fabs(tetrahedron_determinant(v0,v1,v2,v3)/6);
}

}
