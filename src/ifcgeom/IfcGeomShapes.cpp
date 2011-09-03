/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

/********************************************************************************
 *                                                                              *
 * Implementations of the various conversion functions defined in IfcRegister.h *
 *                                                                              *
 ********************************************************************************/

#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Trsf2d.hxx>
#include <gp_Mat.hxx>
#include <gp_Ax3.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>

#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <Geom_Line.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_TrimmedCurve.hxx>

#include <BRepOffsetAPI_Sewing.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeShell.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix_Solid.hxx>

#include <BRepFilletAPI_MakeFillet2d.hxx>

#include <TopLoc_Location.hxx>

#include "../ifcgeom/IfcGeom.h"

bool IfcGeom::convert(const Ifc2x3::IfcExtrudedAreaSolid::ptr l, TopoDS_Shape& shape) {
	TopoDS_Face face;
	if ( ! IfcGeom::convert_face(l->SweptArea(),face) ) return false;
	const float height = l->Depth() * Ifc::LengthUnit;
	gp_Trsf trsf;
	IfcGeom::convert(l->Position(),trsf);

	gp_Dir dir;
	convert(l->ExtrudedDirection(),dir);
	
	shape = BRepPrimAPI_MakePrism(face,height*dir);
	shape.Move(trsf);
	return ! shape.IsNull();
}
bool IfcGeom::convert(const Ifc2x3::IfcFacetedBrep::ptr l, TopoDS_Shape& shape) {
	return IfcGeom::convert_shape(l->Outer(),shape);
}
bool IfcGeom::convert(const Ifc2x3::IfcFaceBasedSurfaceModel::ptr l, TopoDS_Shape& shape) {
	Ifc2x3::IfcConnectedFaceSet::list facesets = l->FbsmFaces();
	BRep_Builder builder;
	TopoDS_Compound c;
	builder.MakeCompound(c);
	for( Ifc2x3::IfcConnectedFaceSet::it it = facesets->begin(); it != facesets->end(); ++ it ) {
		TopoDS_Shape s;
		if ( IfcGeom::convert_shape(*it,s) ) {
			builder.Add(c,s);
		}
	}
	shape = c;
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcHalfSpaceSolid::ptr l, TopoDS_Shape& shape) {
	Ifc2x3::IfcSurface::ptr surface = l->BaseSurface();
	if ( ! surface->is(Ifc2x3::Type::IfcPlane) ) {
		// Not implemented
		return false;
	}
	gp_Pln pln;
	IfcGeom::convert(reinterpret_pointer_cast<Ifc2x3::IfcSurface,Ifc2x3::IfcPlane>(surface),pln);
	gp_Pnt pnt = pln.Location();
	bool reverse = l->AgreementFlag();
	if ( l->is(Ifc2x3::Type::IfcPolygonalBoundedHalfSpace) ) reverse = !reverse;
	if ( reverse ) pnt.Translate(-pln.Axis().Direction());
	else pnt.Translate(pln.Axis().Direction());
	shape = BRepPrimAPI_MakeHalfSpace(BRepBuilderAPI_MakeFace(pln),pnt).Solid();
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcPolygonalBoundedHalfSpace::ptr l, TopoDS_Shape& shape) {
	TopoDS_Shape halfspace;
	if ( ! IfcGeom::convert(reinterpret_pointer_cast<Ifc2x3::IfcPolygonalBoundedHalfSpace,Ifc2x3::IfcHalfSpaceSolid>(l),halfspace) ) return false;	
	TopoDS_Wire wire;
	if ( ! IfcGeom::convert_wire(l->PolygonalBoundary(),wire) ) return false;	
	gp_Trsf trsf;
	convert(l->Position(),trsf);
	TopoDS_Shape extrusion = BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(wire),gp_Vec(0,0,20000.0));
	gp_Trsf down; down.SetTranslation(gp_Vec(0,0,-10000.0));
	extrusion.Move(down*trsf);
	shape = BRepAlgoAPI_Cut(extrusion,halfspace);
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcShellBasedSurfaceModel::ptr l, TopoDS_Shape& shape) {
	IfcUtil::IfcAbstractSelect::list shells = l->SbsmBoundary();
	BRep_Builder builder;
	TopoDS_Compound c;
	builder.MakeCompound(c);
	for( IfcUtil::IfcAbstractSelect::it it = shells->begin(); it != shells->end(); ++ it ) {
		TopoDS_Shape s;
		if ( IfcGeom::convert_shape(*it,s) ) {
			builder.Add(c,s);
		}
	}
	shape = c;
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcBooleanClippingResult::ptr l, TopoDS_Shape& shape) {
	TopoDS_Shape s1, s2;
	if ( ! IfcGeom::convert_shape(l->FirstOperand(),s1) )
		return false;
	if ( ! IfcGeom::convert_shape(l->SecondOperand(),s2) )
		return false;
	shape = BRepAlgoAPI_Cut(s1,s2);
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcClosedShell::ptr l, TopoDS_Shape& shape) {
	if ( ! IfcGeom::convert((Ifc2x3::IfcConnectedFaceSet::ptr)l,shape) ) return false;
	try {
		ShapeFix_Solid solid;
		solid.LimitTolerance(0.01);
		shape = solid.SolidFromShell(TopoDS::Shell(shape));
	} catch(...) {}
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcConnectedFaceSet::ptr l, TopoDS_Shape& shape) {
	BRepOffsetAPI_Sewing builder;
	builder.SetTolerance(0.01);
	Ifc2x3::IfcFace::list faces = l->CfsFaces();
	bool facesAdded = false;
	for( Ifc2x3::IfcFace::it it = faces->begin(); it != faces->end(); ++ it ) {
		TopoDS_Face face;
		if ( IfcGeom::convert_face(*it,face) ) {
			builder.Add(face);
			facesAdded = true;
		} else {
			Ifc::LogMessage("Warning","Invalid face:",(*it)->entity);
		}
	}
	if ( ! facesAdded ) return false;
	builder.Perform();
	shape = builder.SewedShape();
	return true;
}
bool IfcGeom::convert(const Ifc2x3::IfcMappedItem::ptr l, TopoDS_Shape& shape) {
	gp_Trsf trsf1;
	Ifc2x3::IfcCartesianTransformationOperator::ptr transform = l->MappingTarget();
	if ( transform->is(Ifc2x3::Type::IfcCartesianTransformationOperator2DnonUniform) || 
		transform->is(Ifc2x3::Type::IfcCartesianTransformationOperator3DnonUniform) ) {
		// Not implemented
		return false;
	} else if ( transform->is(Ifc2x3::Type::IfcCartesianTransformationOperator3D) ) {
		IfcGeom::convert(reinterpret_pointer_cast<Ifc2x3::IfcCartesianTransformationOperator,
			Ifc2x3::IfcCartesianTransformationOperator3D>(transform),trsf1);
	} else if ( transform->is(Ifc2x3::Type::IfcCartesianTransformationOperator2D) ) {
		gp_Trsf2d trsf1_2d;
		IfcGeom::convert(reinterpret_pointer_cast<Ifc2x3::IfcCartesianTransformationOperator,
			Ifc2x3::IfcCartesianTransformationOperator2D>(transform),trsf1_2d);
		trsf1 = trsf1_2d;
	}
	Ifc2x3::IfcRepresentationMap::ptr map = l->MappingSource();
	Ifc2x3::IfcAxis2Placement placement = map->MappingOrigin();
	gp_Trsf trsf2;
	if (placement->is(Ifc2x3::Type::IfcAxis2Placement3D)) {
		IfcGeom::convert((Ifc2x3::IfcAxis2Placement3D*)placement,trsf2);
	} else {
		gp_Trsf2d trsf2_2d;
		IfcGeom::convert((Ifc2x3::IfcAxis2Placement2D*)placement,trsf2_2d);
		trsf2 = trsf2_2d;
	}
	if ( ! IfcGeom::convert_shape(map->MappedRepresentation(),shape) ) return false;
	shape.Move(trsf2 * trsf1);
	return !shape.IsNull();
}
bool IfcGeom::convert(const Ifc2x3::IfcShapeRepresentation::ptr l, TopoDS_Shape& shape) {
	Ifc2x3::IfcRepresentationItem::list items = l->Items();
	if ( ! items->Size() ) return false;
	BRep_Builder builder;
	TopoDS_Compound c;
	builder.MakeCompound(c);
	for ( Ifc2x3::IfcRepresentationItem::it it = items->begin(); it != items->end(); ++ it ) {
		TopoDS_Shape s;
		if ( ! IfcGeom::convert_shape(*it,s)) continue;
		builder.Add(c,s);
	}
	shape = c;
	return !shape.IsNull();
}