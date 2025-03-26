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

#ifndef IFCGEOMELEMENT_H
#define IFCGEOMELEMENT_H

#include <string>
#include <algorithm>

#include "../../ifcparse/IfcGlobalId.h"
#include "../../ifcparse/Argument.h"

#include "../../ifcgeom/schema_agnostic/IfcGeomRepresentation.h"
#include "../../ifcgeom/settings.h"
#include "../../ifcgeom/taxonomy.h"

#include "ifc_geom_api.h"

namespace ifcopenshell { namespace geometry {

	class Transformation {
	private:
		element_settings settings_;
		ifcopenshell::geometry::taxonomy::matrix4 matrix_;
	public:
		Transformation(const element_settings& settings, const ifcopenshell::geometry::taxonomy::matrix4& trsf)
			: settings_(settings)
			, matrix_(trsf)
		{
			// Note that in case the CONVERT_BACK_UNITS setting is enabled
			// the translation component of the matrix needs to be divided
			// by the magnitude of the IFC model length unit because
			// internally in IfcOpenShell everything is measured in meters.
			if (settings.get(settings::CONVERT_BACK_UNITS)) {
				for (int i = 0; i <= 2; ++i) {
					matrix_.components()(3, i) /= settings.unit_magnitude();
				}
			}
		}
		const ifcopenshell::geometry::taxonomy::matrix4& data() const { return matrix_; }
		const element_settings& settings() const { return settings_; }
	};

	class Element {
	private:
		int _id;
		int _parent_id;
		std::string _name;
		std::string _type;
		std::string _guid;
		std::string _context;
		std::string _unique_id;
		Transformation _transformation;
        IfcUtil::IfcBaseEntity* product_;
		std::vector<const Element*> _parents;
	public:

		friend bool operator == (const Element & element1, const Element & element2) {
			return element1.id() == element2.id();
		}

		// Use the id to compare, or the elevation is the elements are IfcBuildingStoreys and the elevation is set
		friend bool operator < (const Element & element1, const Element & element2) {
			if (element1.type() == "IfcBuildingStorey" && element2.type() == "IfcBuildingStorey") {
				size_t attr_index = element1.product()->declaration().attribute_index("Elevation");
				Argument* elev_attr1 = element1.product()->data().getArgument(attr_index);
				Argument* elev_attr2 = element2.product()->data().getArgument(attr_index);

				if (!elev_attr1->isNull() && !elev_attr2->isNull()) {
					double elev1 = *elev_attr1;
					double elev2 = *elev_attr2;

					return elev1 < elev2;
				}
			}

			return element1.id() < element2.id();
		}

		int id() const { return _id; }
		int parent_id() const { return _parent_id; }
		const std::string& name() const { return _name; }
		const std::string& type() const { return _type; }
		const std::string& guid() const { return _guid; }
		const std::string& context() const { return _context; }
		const std::string& unique_id() const { return _unique_id; }
		const Transformation& transformation() const { return _transformation; }
        IfcUtil::IfcBaseEntity* product() const { return product_; }
		const std::vector<const Element*> parents() const { return _parents; }
		void SetParents(std::vector<const Element*> newparents) { _parents = newparents; }

		Element(const element_settings& settings, int id, int parent_id, const std::string& name, const std::string& type,
            const std::string& guid, const std::string& context, const ifcopenshell::geometry::taxonomy::matrix4& trsf, IfcUtil::IfcBaseEntity* product)
			: _id(id), _parent_id(parent_id), _name(name), _type(type), _guid(guid), _context(context), _transformation(settings, trsf)
            , product_(product)
		{ 
			std::ostringstream oss;

			if (type == "IfcProject") {
				oss << "project";
			} else {
				try {
					oss << "product-" << IfcParse::IfcGlobalId(guid).formatted();
				} catch (const std::exception& e) {
					oss << "product";
					Logger::Error(e);
				}
			}

			if (!_context.empty()) {
				std::string ctx = _context;
                boost::to_lower(ctx);
                boost::replace_all(ctx, " ", "-");
				oss << "-" << ctx;
			}

			_unique_id = oss.str();
		}

		virtual ~Element() {}
	};

	class NativeElement : public Element {
	private:
		boost::shared_ptr<Representation::BRep> _geometry;
	public:
		const boost::shared_ptr<Representation::BRep>& geometry_pointer() const { return _geometry; }
		const Representation::BRep& geometry() const { return *_geometry; }
		NativeElement(int id, int parent_id, const std::string& name, const std::string& type, const std::string& guid,
            const std::string& context, const ifcopenshell::geometry::taxonomy::matrix4& trsf, const boost::shared_ptr<Representation::BRep>& geometry,
			IfcUtil::IfcBaseEntity* product)
			: Element(geometry->settings() ,id, parent_id, name, type, guid, context, trsf, product)
			, _geometry(geometry)
		{}

		bool calculate_projected_surface_area(double& along_x, double& along_y, double& along_z) const {
			return geometry().calculate_projected_surface_area(this->transformation().data(), along_x, along_y, along_z);
		}
	private:
		NativeElement(const NativeElement& other);
		NativeElement& operator=(const NativeElement& other);		
	};

	class TriangulationElement : public Element {
	private:
		boost::shared_ptr<Representation::Triangulation> _geometry;
	public:
		const Representation::Triangulation& geometry() const { return *_geometry; }
		const boost::shared_ptr< Representation::Triangulation >& geometry_pointer() const { return _geometry; }
		TriangulationElement(const NativeElement& shape_model)
			: Element(shape_model)
			, _geometry(boost::shared_ptr<Representation::Triangulation >(new Representation::Triangulation(shape_model.geometry())))
		{}
		TriangulationElement(const Element& element, const boost::shared_ptr<Representation::Triangulation >& geometry)
			: Element(element)
			, _geometry(geometry)
		{}
	private:
		TriangulationElement(const TriangulationElement& other);
		TriangulationElement& operator=(const TriangulationElement& other);
	};

	class SerializedElement : public Element {
	private:
		Representation::Serialization* _geometry;
	public:
		const Representation::Serialization& geometry() const { return *_geometry; }
		SerializedElement(const NativeElement& shape_model)
			: Element(shape_model)
			, _geometry(new Representation::Serialization(shape_model.geometry()))
		{}
		virtual ~SerializedElement() {
			delete _geometry;
		}
	private:
		SerializedElement(const SerializedElement& other);
		SerializedElement& operator=(const SerializedElement& other);
	};
}}

#endif
