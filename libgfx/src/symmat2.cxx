#include <gfx/gfx.h>
#include <gfx/symmat2.h>

namespace gfx
{

SymMat2 SymMat2::I()
{
    SymMat2 A;
    A(0,0) = A(1,1) = 1;
    return A;
}

Mat2 SymMat2::fullmatrix() const
{
    Mat2 A;

    for(int i=0; i<A.dim(); i++)
	for(int j=0; j<A.dim(); j++)
	    A(i, j) = (*this)(i,j);

    return A;
}

SymMat2 operator*(const SymMat2& n, const SymMat2& m)
{
    SymMat2 A;
    for(int i=0; i<2; i++)  for(int j=i; j<2; j++)
	    A(i,j) = n.row(i)*m.col(j);
    return A;
}

std::ostream &operator<<(std::ostream &out, const SymMat2& M)
{
    for(int i=0; i<M.dim(); i++)
    {
	for(int j=0; j<M.dim(); j++)
	    out << M(i, j) << " ";
	out << std::endl;
    }

    return out;
}

SymMat2 SymMat2::outer_product(const Vec2& v)
{
    SymMat2 A;

    for(int i=0; i<A.dim(); i++)
	for(int j=i; j<A.dim(); j++)
	    A(i, j) = v[i]*v[j];

    return A;
}

double invert(Mat2& m_inv, const SymMat2& m)
{
    return invert(m_inv, m.fullmatrix());
}

} // namespace gfx
