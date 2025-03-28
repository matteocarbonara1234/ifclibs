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

#include "OpenCascadeBasedSerializer.h"

#include "../ifcparse/utils.h"

#include <string>
#include <fstream>
#include <cstdio>

#include <Standard_Version.hxx>
#include <BRepBuilderAPI_Transform.hxx>

bool OpenCascadeBasedSerializer::ready() {
	std::ofstream test_file(IfcUtil::path::from_utf8(out_filename).c_str(), std::ios_base::binary);
	bool succeeded = test_file.is_open();
	test_file.close();
	IfcUtil::path::delete_file(out_filename);
	return succeeded;
}

void OpenCascadeBasedSerializer::write(const ifcopenshell::geometry::NativeElement* o) {
	ifcopenshell::geometry::OpenCascadeShape* occt_shape = ((ifcopenshell::geometry::OpenCascadeShape*) o->geometry().as_compound());
        TopoDS_Shape compound = occt_shape->shape();
        delete occt_shape;

	if (o->geometry().settings().get(ifcopenshell::geometry::settings::CONVERT_BACK_UNITS)) {
		gp_Trsf scale;
		scale.SetScaleFactor(1.0 / o->geometry().settings().unit_magnitude());
		
		compound = BRepBuilderAPI_Transform(compound, scale, true).Shape();
	}

	ifcopenshell::geometry::OpenCascadeShape s(compound);
	writeShape(&s);
}

#define RATHER_SMALL (1e-3)
#define APPROXIMATELY_THE_SAME(a,b) (fabs(a-b) < RATHER_SMALL)

const char* OpenCascadeBasedSerializer::getSymbolForUnitMagnitude(float mag) {
	if (APPROXIMATELY_THE_SAME(mag, 0.001f)) {
		return "MM";
	} else if (APPROXIMATELY_THE_SAME(mag, 0.01f)) {
		return "CM";
	} else if (APPROXIMATELY_THE_SAME(mag, 1.0f)) {
		return "M";
	} else if (APPROXIMATELY_THE_SAME(mag, 0.3048f)) {
		return "FT";
	} else if (APPROXIMATELY_THE_SAME(mag, 0.0254f)) {
		return "INCH";
	} else {
		return 0;
	}
}
