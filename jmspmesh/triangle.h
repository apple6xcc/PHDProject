// ******************************
// triangle.h
//
// Triangle class.
// Contains vetices & normal to
// the place defined by the 3
// vertices.
// 
// Jeff Somers
// Copyright (c) 2002
//
// jsomers@alumni.williams.edu
// March 27, 2002
// ******************************

#ifndef __triangle_h
#define __triangle_h

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#pragma warning(disable:4710) // function not inlined
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4514) // unreferenced inline function has been removed
#endif

#include <assert.h>
#include <iostream>

#include "vec3.h"

class vertex;

// #include "mesh.h"
// Can't include "mesh.h" here, 'cause it's a circular reference and it won't compile.
class jmsMesh;


// Cannot be called Polygon, because that conflicts with
// a Microsoft function.
class triangle
{
public:
	// Constructors
	triangle() : 
		_vert1(-1), _vert2(-1), _vert3(-1) ,
		_mesh(0), bActive(false), _index(-1) ,bSel(false), bLine(false){};

	triangle(int v1, int v2, int v3) : 
		_vert1(v1), _vert2(v2), _vert3(v3), _mesh(0), bActive(true),
		_index(-1),bSel(false) , bLine(false){};

	triangle(jmsMesh* mp, int v1, int v2, int v3) : 
		_vert1(v1), _vert2(v2), _vert3(v3), _mesh(mp), bActive(true),
		_index(-1),bSel(false), bLine(false)
	{
		assert(mp);

		calcNormal();

	};


	// copy ctor
	triangle(const triangle& t) :  
		_vert1(t._vert1), _vert2(t._vert2), _vert3(t._vert3), 
		_mesh(t._mesh), bActive(t.bActive),
		_index(t._index),bSel(t.bSel), bLine(t.bLine)
	{
		calcNormal();
	};

	// assignment operator
	triangle& operator=(const triangle& t)
	{
		if (&t == this) return *this; // check for assignment to self
		_vert1 = t._vert1;
		_vert2 = t._vert2;
		_vert3 = t._vert3;
		_mesh = t._mesh;
		_normal = t._normal;
		bActive = t.bActive;
		_index = t._index;
		bSel = t.bSel;
		_layer = t._layer;
		bLine = t.bLine;
		return *this;
	}

	// assumes pointing to same list of verts
	bool operator==(const triangle& t)
	{
		return (_vert1 == t._vert1 && 
				_vert2 == t._vert2 && 
				_vert3 == t._vert3 &&
				_mesh == t._mesh
				);
	}

	// reset the normal for the triangle
	void calcNormal();

	void changeMesh(jmsMesh* mp) {_mesh = mp;};

	// if the triangle is not active, we set a flag.
	bool isActive() const {return bActive;};
	void setActive(bool b) {bActive = b;};

	// 设置当前三角形的选定状态，选择或者未选择
	bool isSel() const {return bSel;};
	void setSel(bool b) {bSel = b;};


	bool hasVertex(int vi) {
		return	(vi == _vert1 ||
				 vi == _vert2 ||
				 vi == _vert3);}

	// When we collapse an edge, we may change the 
	// vertex of a triangle.
	void changeVertex(int vFrom, int vTo)
	{
		assert(vFrom != vTo);
		assert(vFrom == _vert1 || vFrom == _vert2 || vFrom == _vert3);
		if (vFrom == _vert1) {
			_vert1 = vTo;
		} 
		else if (vFrom == _vert2) {
			_vert2 = vTo;
		}
		else if (vFrom == _vert3) {
			_vert3 = vTo;
		}
		else {
			//!FIX error
		}
	}

	void getVerts(int& v1, int& v2, int& v3) {v1=_vert1;v2=_vert2;v3=_vert3;}

	const float* getVert1();
	const float* getVert2();
	const float* getVert3();

	vertex& getVert1vertex() const;
	vertex& getVert2vertex() const;
	vertex& getVert3vertex() const;

	float* getNormal() {_normArray[0]=_normal.x;
						_normArray[1]=_normal.y;
						_normArray[2]=_normal.z;
						return _normArray;}

	float calcArea(); // returns area of triangle

	friend std::ostream& operator<<(std::ostream& os, const triangle& to);

	int getVert1Index() const {return _vert1;}
	int getVert2Index() const {return _vert2;}
	int getVert3Index() const {return _vert3;}
	const vec3& getNormalVec3() const {return _normal;}

	int getIndex() const {return _index;}
	void setIndex(int i) {_index = i;}

	// 'd' is from the plane equation ax + by + cz + d = 0
	float getD() const {return _d;}

	void setVertexesLayer(short nLayer);
	void setLayer(short nLayer) { _layer = nLayer; setVertexesLayer(nLayer); }
	short getLayer() { return _layer; }
	// 将mesh指针进行更新，指向新的网格地址 [7/6/2011 Han Honglei]
	void updateMeshPointer(jmsMesh *newMesh) { _mesh = newMesh; }
	bool bLine;

	// backup of the vertex [9/27/2011 Han Honglei]
	void restoreVert() {_vert1 = vertIndex[0];_vert2 = vertIndex[1];_vert3 = vertIndex[2];
}
	int vertIndex[3];

protected:

	int _vert1;
	int _vert2;
	int _vert3;

	vec3 _normal; // normal to plane
	float _normArray[3];

	float _d;	// This parameter is the "d" in the
				// plane equation ax + by + cz + d = 0
				// The plane equation of this triangle is used.

	bool bActive; // active flag

	jmsMesh* _mesh;

	int _index; // index in list of triangles w/in mesh

	// 是否被选中 [8/6/2010 Leo Han]
	bool bSel;
	// 三角形面所属的骨架级别 [1/5/2011 Han Honglei]
	short _layer;
};

#endif //__triangle_h
