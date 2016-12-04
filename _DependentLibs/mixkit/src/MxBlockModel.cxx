/************************************************************************

  MxBlockModel

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.
  
  $Id: MxBlockModel.cxx,v 1.27 2000/11/20 20:36:38 garland Exp $

 ************************************************************************/

#include "stdmix.h"
#include "MxBlockModel.h"
#include "MxVector.h"
//#include "Matrix.h"

////////////////////////////////////////////////////////////////////////
//
// Basic allocation routines
//

MxBlockModel *MxBlockModel::clone(MxBlockModel *m)
{
    if( !m ) m = new MxBlockModel(vert_count(), face_count());

    uint i;

    for(i=0; i<vert_count(); i++)
	m->add_vertex(vertex(i));
    for(i=0; i<face_count(); i++)
	m->add_face(face(i)[0], face(i)[1], face(i)[2]);

    m->normal_binding(normal_binding());
    if( normal_binding() != MX_UNBOUND )
    {
	m->normals->room_for(normal_count());
	m->normals->bitcopy(*normals);
    }

    m->color_binding(color_binding());
    if( color_binding() != MX_UNBOUND )
    {
	m->colors->room_for(color_count());
	m->colors->bitcopy(*colors);
    }

    m->texcoord_binding(texcoord_binding());
    if( texcoord_binding() != MX_UNBOUND )
    {
	m->tcoords->room_for(texcoord_count());
	m->tcoords->bitcopy(*tcoords);
    }

    return m;
}

MxFaceID MxBlockModel::alloc_face(MxVertexID v1, MxVertexID v2, MxVertexID v3)
{
    faces.add(MxFace(v1,v2,v3));
    return faces.last_id();
}

MxVertexID MxBlockModel::alloc_vertex(float x, float y, float z)
{
    vertices.add(MxVertex(x,y,z));
    return vertices.last_id();
}

MxVertexID MxBlockModel::add_vertex(float x, float y, float z)
{
    MxVertexID id = alloc_vertex(x,y,z);
    init_vertex(id);
    return id;
}

void MxBlockModel::remove_vertex(MxVertexID v)
{
    AssertBound( v < vertices.length() );

    free_vertex(v);
    vertices.remove(v);
    if( normal_binding() == MX_PERVERTEX ) normals->remove(v);
    if( color_binding() == MX_PERVERTEX ) colors->remove(v);
    if( texcoord_binding() == MX_PERVERTEX ) tcoords->remove(v);
}

void MxBlockModel::remove_face(MxFaceID f)
{
    AssertBound( f < faces.length() );

    free_face(f);
    faces.remove(f);
    if( normal_binding() == MX_PERFACE ) normals->remove(f);
    if( color_binding() == MX_PERFACE ) colors->remove(f);
    if( texcoord_binding() == MX_PERFACE ) tcoords->remove(f);
}

MxFaceID MxBlockModel::add_face(unsigned int v1,
				unsigned int v2,
				unsigned int v3,
				bool will_link)
{
    MxFaceID id = alloc_face(v1, v2, v3);
    if( will_link )  init_face(id);
    return id;
}

unsigned int MxBlockModel::add_color(float r, float g, float b)
{
    assert( colors );
    MxColor c(r, g, b);
    colors->add(c);
    return colors->last_id();
}

unsigned int MxBlockModel::add_normal(float x, float y, float z)
{
    MxNormal n(x, y, z);
    normals->add(n);
    return normals->last_id();
}

unsigned int MxBlockModel::add_texcoord(float s, float t)
{
    tcoords->add(MxTexCoord(s,t));
    return tcoords->last_id();
}

uint MxBlockModel::add_texmap(MxRaster *t, const char *name)
{
    if( !t ) return MXID_NIL;
    if( !name ) name = "tex";

    if( tex ) delete tex;
    if( tex_name ) delete tex_name;

    tex_name = strdup(name);
    tex = t;
    return 0;
}

////////////////////////////////////////////////////////////////////////
//
// Property binding
//

static const char *bindings[] = {
    "unbound",
    "face",
    "vertex",
    NULL
};

static
uint binding_size(MxBlockModel& m, unsigned char i)
{
    switch( i )
    {
    case MX_UNBOUND: return 0;
    case MX_PERVERTEX: return MAX(1, m.vert_count());
    case MX_PERFACE: return MAX(1, m.face_count());
    default: return 0;
    }
}

const char *MxBlockModel::binding_name(int b)
{
    if( b > MX_MAX_BINDING )
	return NULL;
    else
	return bindings[b];
}
    
int MxBlockModel::parse_binding(const char *name)
{
    for(int i=0; i<=MX_MAX_BINDING; i++)
	if( streq(bindings[i], name) )  return i;

    return MX_UNBOUND;
}

void MxBlockModel::color_binding(unsigned char b)
{
    int size = binding_size(*this, b);

    if( b==MX_UNBOUND )
    {
	if( colors ) { delete colors; colors=NULL; }
	binding_mask &= (~MX_COLOR_MASK);
    }
    else
    {
	if( colors )
	    colors->reset();
	else
	    colors = new MxDynBlock<MxColor>(size);
	binding_mask |= MX_COLOR_MASK;
    }

    cbinding=b;
}

void MxBlockModel::normal_binding(unsigned char b)
{
    int size = binding_size(*this, b);

    if( b==MX_UNBOUND )
    {
	if( normals ) { delete normals; normals=NULL; }
	binding_mask &= (~MX_NORMAL_MASK);
    }
    else
    {
	if( normals )
	    normals->reset();
	else
	    normals = new MxDynBlock<MxNormal>(size);
	binding_mask |= MX_NORMAL_MASK;
    }

    nbinding=b;
}

void MxBlockModel::texcoord_binding(unsigned char b)
{
    if( b!=MX_UNBOUND && b!=MX_PERVERTEX )
	fatal_error("Illegal texture coordinate binding.");

    int size = binding_size(*this, b);
    if( tcoords )  tcoords->reset();
    else tcoords = new MxDynBlock<MxTexCoord>(size);

    tbinding = b;
}

////////////////////////////////////////////////////////////////////////
//
// Utility methods for computing characteristics of faces.
//

void MxBlockModel::compute_face_normal(MxFaceID f, float *n, bool will_unitize)
{
    float *v1 = vertex(face(f)[0]);
    float *v2 = vertex(face(f)[1]);
    float *v3 = vertex(face(f)[2]);

    float a[3], b[3];

    mxv_sub(a, v2, v1, 3);
    mxv_sub(b, v3, v1, 3);
    mxv_cross3(n, a, b);
    if( will_unitize )
	mxv_unitize(n, 3);
}

void MxBlockModel::compute_face_normal(MxFaceID f, double *n,bool will_unitize)
{
    float *v1 = vertex(face(f)[0]);
    float *v2 = vertex(face(f)[1]);
    float *v3 = vertex(face(f)[2]);

    double a[3], b[3];
    for(int i=0; i<3; i++) { a[i] = v2[i]-v1[i];  b[i] = v3[i]-v1[i]; }

    mxv_cross3(n, a, b);
    if( will_unitize )
	mxv_unitize(n, 3);
}

void MxBlockModel::compute_face_plane(MxFaceID f, float *p, bool will_unitize)
{
    compute_face_normal(f, p, will_unitize);
    p[3] = -mxv_dot(p, corner(f, 0), 3);
}

double MxBlockModel::compute_face_area(MxFaceID f)
{
    double n[3];

    compute_face_normal(f, n, false);
    return 0.5 * mxv_norm(n, 3);
}

double MxBlockModel::compute_face_perimeter(MxFaceID fid, bool *flags)
{
    double perim = 0.0;
    const MxFace& f = face(fid);

    for(uint i=0; i<3; i++)
    {
	if( !flags || flags[i] )
	{
	    float *vi = vertex(f[i]),  *vj = vertex(f[(i+1)%3]), e[3];
	    perim += mxv_norm(mxv_sub(e, vi, vj, 3), 3);
	}
    }

    return perim;
}

double MxBlockModel::compute_corner_angle(MxFaceID f, uint i)
{
    uint i_prev = (i==0)?2:i-1;
    uint i_next = (i==2)?0:i+1;

    float e_prev[3], e_next[3];
    mxv_unitize(mxv_sub(e_prev, corner(f, i_prev), corner(f, i), 3), 3);
    mxv_unitize(mxv_sub(e_next, corner(f, i_next), corner(f, i), 3), 3);

    return acos(mxv_dot(e_prev, e_next, 3));
}
// add [2/13/2012 Han]


bool MxBlockModel::GetBoundSize(float size[3])
{
	float max[3], min[3];
	max[0] = max[1] = max[2] = -FLT_MAX;
	min[0] = min[1] = min[2] = FLT_MAX;

	for (int i = 0; i < vertices.size(); ++i)
	{
		const float* pVert = vertices[i].as.pos;
		if (pVert[0] < min[0]) min[0] = pVert[0];
		if (pVert[1] < min[1]) min[1] = pVert[1];
		if (pVert[2] < min[2]) min[2] = pVert[2];
		if (pVert[0] > max[0]) max[0] = pVert[0];
		if (pVert[1] > max[1]) max[1] = pVert[1];
		if (pVert[2] > max[2]) max[2] = pVert[2];
	}
	for(int i = 0; i < 3; i++)
		size[i] = max[i] - min[i];
	return true;
}


bool MxBlockModel::LoadMBlur(FILE* inFile)
{
	if (inFile == NULL)
		return false;
	float fMin = FLT_MAX, fMax = 0.0f;		
	float fBlur = 0.0;
	pVertexMBlur.clear();
	fscanf(inFile, "%f", &fMotionAngle);	// 首先读入运动的方向（和x轴的夹角），横向运动为0°，纵向运动为90°
	fscanf(inFile, "%f", &fWeightPower);	// 然后读入motion blur

	// 接下来读取每个顶点的权重信息
	while(!feof(inFile))
	{
		fscanf(inFile, "%f", &fBlur);
		pVertexMBlur.push_back(fBlur);
		if (fMin > fBlur)
			fMin = fBlur;
		if (fMax < fBlur)
			fMax = fBlur;
	}
	return true;
}
// Get min, max values of all verts
//void MxBlockModel::setMinMax(float min[3], float max[3])
//{
//	max[0] = max[1] = max[2] = -FLT_MAX;
//	min[0] = min[1] = min[2] = FLT_MAX;
//
//	for (int i = 0; i < vertices.size(); ++i)
//	{
//		const float* pVert = vertices[i].as.pos;
//		if (pVert[0] < min[0]) min[0] = pVert[0];
//		if (pVert[1] < min[1]) min[1] = pVert[1];
//		if (pVert[2] < min[2]) min[2] = pVert[2];
//		if (pVert[0] > max[0]) max[0] = pVert[0];
//		if (pVert[1] > max[1]) max[1] = pVert[1];
//		if (pVert[2] > max[2]) max[2] = pVert[2];
//	}
//}
//
//// Center mesh around origin.
//// Fit mesh in box from (-1, -1, -1) to (1, 1, 1)
//void MxBlockModel::Normalize()  
//{
//	float min[3], max[3], Scale;
//
//	setMinMax(min, max);
//	
//	Vec3 minv(min);
//	Vec3 maxv(max);
//
//	Vec3 dimv = maxv - minv;
//
//	float f_scale = 1.0 / dimv.length() * 1.5;
//	Vec3 trans = -f_scale * (minv + 0.5 * (maxv - minv));
//
//	Matrix mxS = scale(f_scale ,f_scale , f_scale);
//	Matrix mxT = translate(trans.x , trans.y , trans.z);
//
//	for (unsigned int i = 0; i < vertices.size(); ++i)
//	{
//		const float* pVert = vertices[i].as.pos;
//		Vec3 posTemp(pVert);
//		transformLoc(mxS, posTemp);
//		transformLoc(mxT, posTemp);
//		for(int j = 0; j < 3; j++)
//			pVert[j] = posTemp[j];
//	}
//}