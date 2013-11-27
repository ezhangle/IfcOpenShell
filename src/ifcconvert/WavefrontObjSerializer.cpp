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

#include "../ifcgeom/IfcGeomRenderStyles.h"

#include "WavefrontObjSerializer.h"

#include <iomanip>

bool WaveFrontOBJSerializer::ready() {
	return obj_stream.is_open() && mtl_stream.is_open();
}

void WaveFrontOBJSerializer::writeHeader() {
	obj_stream << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << std::endl;
#ifdef WIN32
	const char dir_separator = '\\';
#else
	const char dir_separator = '/';
#endif
	std::string mtl_basename = mtl_filename;
	std::string::size_type slash = mtl_basename.find_last_of(dir_separator);
	if (slash != std::string::npos) {
		mtl_basename = mtl_basename.substr(slash+1);
	}
	obj_stream << "mtllib " << mtl_basename << std::endl;
	mtl_stream << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << std::endl;	
}

void WaveFrontOBJSerializer::writeMaterial(const IfcGeomObjects::Material& style) {			
	mtl_stream << "newmtl " << style.name() << std::endl;
	if (style.hasDiffuse()) {
		const double* diffuse = style.diffuse();
		mtl_stream << "Kd " << diffuse[0] << " " << diffuse[1] << " " << diffuse[2] << std::endl;
	}
	if (style.hasSpecular()) {
		const double* specular = style.specular();
		mtl_stream << "Ks " << specular[0] << " " << specular[1] << " " << specular[2] << std::endl;
	}
	if (style.hasSpecularity()) {
		mtl_stream << "Ns " << style.specularity() << std::endl;
	}
	if (style.hasTransparency()) {
		const double transparency = 1.0 - style.transparency();
		if (transparency < 1) {
			mtl_stream << "Tr " << transparency << std::endl;
			mtl_stream << "d "  << transparency << std::endl;
			mtl_stream << "D "  << transparency << std::endl;
		}
	}
}
void WaveFrontOBJSerializer::writeTesselated(const IfcGeomObjects::IfcGeomObject* o) {

	std::string tmp = o->name().empty() ? o->guid() : o->name();

	std::replace( tmp.begin(), tmp.end(), ' ', '_');
	const std::string name = tmp;
	obj_stream << "g " << name << std::endl;
	obj_stream << "s 1" << std::endl;

	const IfcGeomObjects::IfcRepresentationTriangulation& mesh = o->mesh();
	
	const int vcount = mesh.verts().size() / 3;
	for ( std::vector<float>::const_iterator it = mesh.verts().begin(); it != mesh.verts().end(); ) {
		const double x = *(it++);
		const double y = *(it++);
		const double z = *(it++);
		obj_stream << "v " << x << " " << y << " " << z << std::endl;
	}
	for ( std::vector<float>::const_iterator it = mesh.normals().begin(); it != mesh.normals().end(); ) {
		const double x = *(it++);
		const double y = *(it++);
		const double z = *(it++);
		obj_stream << "vn " << x << " " << y << " " << z << std::endl;
	}

	int previous_material_id = -2;
	std::vector<int>::const_iterator material_it = mesh.material_ids().begin();

	for ( std::vector<int>::const_iterator it = mesh.faces().begin(); it != mesh.faces().end(); ) {
		
		const int material_id = *(material_it++);
		if (material_id != previous_material_id) {
			IfcGeomObjects::Material material(0);
			if (material_id >= 0) {
				material = mesh.materials()[material_id];
			} else {
				material = IfcGeomObjects::Material(IfcGeom::get_default_style(o->type()));
			}
			const std::string material_name = material.name();
			obj_stream << "usemtl " << material_name << std::endl;
			if (materials.find(material_name) == materials.end()) {
				writeMaterial(material);
				materials.insert(material_name);
			}
			previous_material_id = material_id;
		}

		const int v1 = *(it++)+vcount_total;
		const int v2 = *(it++)+vcount_total;
		const int v3 = *(it++)+vcount_total;
		obj_stream << "f " << v1 << "//" << v1 << " " << v2 << "//" << v2 << " " << v3 << "//" << v3 << std::endl;
	}
    vcount_total += vcount;
}
