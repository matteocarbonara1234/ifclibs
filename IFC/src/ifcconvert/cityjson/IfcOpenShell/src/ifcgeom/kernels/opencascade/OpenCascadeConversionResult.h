﻿/********************************************************************************
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

#ifndef IFCGEOMOPENCASCADEREPRESENTATION_H
#define IFCGEOMOPENCASCADEREPRESENTATION_H

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepGProp_Face.hxx>

#include <Poly_Triangulation.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfPnt2d.hxx>

#include <TopExp_Explorer.hxx>
#include <BRepTools.hxx>

#include <gp_GTrsf.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>

#include "../../../ifcgeom/schema_agnostic/ConversionResult.h"

namespace ifcopenshell {
	namespace geometry {

		class OpenCascadeShape : public ConversionResultShape {
		public:
			OpenCascadeShape(const TopoDS_Shape& shape)
				: shape_(shape) {}

			const TopoDS_Shape& shape() const { return shape_; }
			operator const TopoDS_Shape& () { return shape_; }

			virtual void Triangulate(const settings& settings, const ifcopenshell::geometry::taxonomy::matrix4& place, Representation::Triangulation* t, int surface_style_id) const;

			virtual void Serialize(std::string&) const {
				throw std::runtime_error("Not implemented");
			}

			virtual ConversionResultShape* clone() const {
				return new OpenCascadeShape(shape_);
			}

			virtual bool is_manifold() const;

			virtual double bounding_box(void*& b) const {
				throw std::runtime_error("Not implemented");
			}

			virtual int num_vertices() const {
				throw std::runtime_error("Not implemented");
			}

			virtual void set_box(void* b) {
				throw std::runtime_error("Not implemented");
			}

			virtual int surface_genus() const;
		private:
			TopoDS_Shape shape_;
		};

	}
}

#endif