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

#ifndef STEPSERIALIZER_H
#define STEPSERIALIZER_H

#include <STEPControl_Controller.hxx>
#include <STEPControl_Writer.hxx>

#include "../ifcgeom/IfcGeomObjects.h"

#include "../ifcconvert/OpenCascadeBasedSerializer.h"

class StepSerializer : public OpenCascadeBasedSerializer
{
private:
	STEPControl_Writer writer;	
public:
	explicit StepSerializer(const std::string& out_filename) 
		: OpenCascadeBasedSerializer(out_filename) 
	{}
	virtual ~StepSerializer() {}
	void writeShape(const TopoDS_Shape& shape) {
		writer.Transfer(shape, STEPControl_AsIs);
	}
	void finalize() {
		writer.Write(out_filename.c_str());
	}
};

#endif