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

#ifndef CGALCONVERSIONRESULT_H
#define CGALCONVERSIONRESULT_H

#include "../../../ifcgeom/schema_agnostic/IfcGeomElement.h"

#include <boost/property_map/property_map.hpp>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/boost/graph/graph_traits_Polyhedron_3.h>
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Nef_polyhedron_3.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel_;

typedef Kernel_::Aff_transformation_3 cgal_placement_t;
typedef Kernel_::Point_3 cgal_point_t;
typedef Kernel_::Vector_3 cgal_direction_t;
typedef Kernel_::Vector_3 cgal_vector_t;
typedef Kernel_::Plane_3 cgal_plane_t;
typedef std::vector<Kernel_::Point_3> cgal_curve_t;
typedef std::vector<Kernel_::Point_3> cgal_wire_t;

struct cgal_face_t {
  cgal_wire_t outer;
  std::vector<cgal_wire_t> inner;
};

typedef CGAL::Polyhedron_3<Kernel_> cgal_shape_t;
typedef boost::graph_traits<CGAL::Polyhedron_3<Kernel_>>::vertex_descriptor cgal_vertex_descriptor_t;
typedef boost::graph_traits<CGAL::Polyhedron_3<Kernel_>>::face_descriptor cgal_face_descriptor_t;

#include "../../../ifcgeom/schema_agnostic/ConversionResult.h"

namespace ifcopenshell { namespace geometry {

	class CgalShape : public ConversionResultShape {
    public:
        CgalShape(const cgal_shape_t& shape)
            : shape_(shape)
        {}

		const cgal_shape_t& shape() const { return shape_; }
		operator const cgal_shape_t& () { return shape_; }

		virtual void Triangulate(const settings& settings, const ifcopenshell::geometry::taxonomy::matrix4& place, Representation::Triangulation* t, int surface_style_id) const;
		
		virtual void Serialize(std::string&) const {
			throw std::runtime_error("Not implemented");
		}

		virtual ConversionResultShape* clone() const {
			return new CgalShape(shape_);
		}

		virtual double bounding_box(void*& b) const;

		virtual int num_vertices() const;

		virtual void set_box(void* b);

		virtual bool is_manifold() const {
			throw std::runtime_error("Not implemented");
		}

		virtual int surface_genus() const {
			throw std::runtime_error("Not implemented");
		}
    private:
        cgal_shape_t shape_;
	};
    
}}

#endif
