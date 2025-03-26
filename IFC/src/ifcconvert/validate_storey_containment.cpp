#ifdef IFOPSH_WITH_CGAL

#include "../ifcgeom/kernels/cgal/CgalKernel.h"
#include "../ifcgeom/IfcGeomFilter.h"
#include "../ifcgeom/Iterator.h"

#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/bbox.h>

#include <algorithm>

void fix_storeycontainment(IfcParse::IfcFile& f, bool no_progress, bool quiet, bool stderr_progress) {
	ifcopenshell::geometry::Settings settings;

	settings.get<ifcopenshell::geometry::settings::UseWorldCoords>().value = false;
	settings.get<ifcopenshell::geometry::settings::WeldVertices>().value = false;
	settings.get<ifcopenshell::geometry::settings::ReorientShells>().value = true;
	settings.get<ifcopenshell::geometry::settings::ConvertBackUnits>().value = true;
	settings.get<ifcopenshell::geometry::settings::IteratorOutput>().value = ifcopenshell::geometry::settings::NATIVE;
	settings.get<ifcopenshell::geometry::settings::DisableOpeningSubtractions>().value = true;

	std::vector<ifcopenshell::geometry::filter_t> no_openings_and_spaces = {
		IfcGeom::entity_filter(false, false, {"IfcOpeningElement", "IfcSpace"})
	};

	IfcGeom::Iterator context_iterator("cgal", settings, &f, no_openings_and_spaces, 1);

	auto get_elevation = [](const IfcUtil::IfcBaseClass* a) {
		return ((const IfcUtil::IfcBaseEntity*)a)->get_value<double>("Elevation", 0.);
	};

	// latebound inverse attribute lookup not working
	auto rels = f.instances_by_type("IfcRelContainedInSpatialStructure");
	std::map<const IfcUtil::IfcBaseClass*, const IfcUtil::IfcBaseClass*> elem_to_storey;
	std::for_each(rels->begin(), rels->end(), [&elem_to_storey](IfcUtil::IfcBaseClass* r) {
		auto elems = ((IfcUtil::IfcBaseEntity*)r)->get_value<aggregate_of_instance::ptr>("RelatedElements");
		auto storey = ((IfcUtil::IfcBaseEntity*)r)->get_value<IfcUtil::IfcBaseClass*>("RelatingStructure");

		if (storey->declaration().name() == "IfcBuildingStorey") {
			for (auto it = elems->begin(); it != elems->end(); ++it) {
				elem_to_storey[*it] = storey;
			}
		}
	});	

	auto storeys = f.instances_by_type("IfcBuildingStorey");
	std::vector<const IfcUtil::IfcBaseClass*> storeys_sorted(storeys->begin(), storeys->end());
	std::sort(storeys_sorted.begin(), storeys_sorted.end(), [&get_elevation](const IfcUtil::IfcBaseClass* a, const IfcUtil::IfcBaseClass* b) {
		return get_elevation(a) < get_elevation(b);
	});

	/*
	std::wcout << "Storeys ";
	for (auto& s : storeys_sorted) {
		auto n = ((IfcUtil::IfcBaseEntity*)s)->get_value<std::string>("Name");
		std::wcout << n.c_str() << " ";
	}
	std::wcout << std::endl;
	*/

	std::vector<double> elevations;
	std::transform(storeys_sorted.begin(), storeys_sorted.end(), std::back_inserter(elevations), get_elevation);

	double LARGE = 1e4;

	std::vector<std::pair<double, double>> elevation_slices;
	for (size_t i = 0; i < elevations.size(); ++i) {
		elevation_slices.push_back({
			i == 0 ? -LARGE : elevations[i],
			i + 1 == elevations.size() ? LARGE : elevations[i + 1]
		});
	}

	std::for_each(elevation_slices.begin(), elevation_slices.end(), [](std::pair<double, double>& p) {
		p.first -= 0.3;
		p.second += 0.3;
	});

	std::vector<CGAL::Nef_polyhedron_3<Kernel_>> nefs;
	std::transform(elevation_slices.begin(), elevation_slices.end(), std::back_inserter(nefs), [&LARGE](const std::pair<double, double>& p) {
		// std::wcout << p.first << " - " << p.second << std::endl;
		Kernel_::Point_3 p1(-LARGE, -LARGE, p.first);
		Kernel_::Point_3 p2(+LARGE, +LARGE, p.second);
		auto poly = ifcopenshell::geometry::utils::create_cube(p1, p2);
		return ifcopenshell::geometry::utils::create_nef_polyhedron(poly);
	});

	/*
	for (auto& n : nefs) {
		auto poly = ifcopenshell::geometry::utils::create_polyhedron(n);
		auto bounds = CGAL::Polygon_mesh_processing::bbox_3(poly);
		for (int i = 0; i < 3; ++i) {
			std::wcout << bounds.min(i) << std::endl;
		}
		for (int i = 0; i < 3; ++i) {
			std::wcout << bounds.max(i) << std::endl;
		}
		std::wcout << "---" << std::endl;
	}
	*/
	
	if (!context_iterator.initialize()) {
		return;
	}

	size_t num_created = 0;
	int old_progress = quiet ? 0 : -1;

	for (;; ++num_created) {
		bool has_more = true;
		if (num_created) {
			has_more = context_iterator.next();
		}
		IfcGeom::BRepElement* geom_object = nullptr;
		if (has_more) {
			geom_object = context_iterator.get_native();
		}
		if (!geom_object) {
			break;
		}

		/*
		std::stringstream ss;
		ss << geom_object->product()->data().toString();
		auto sss = ss.str();
		std::wcout << sss.c_str() << std::endl;
		*/

		if (elem_to_storey.find(geom_object->product()) == elem_to_storey.end()) {
			// std::wcout << "not associated to storey" << std::endl;
			continue;
		}

		std::vector<double> intersection_volumes(nefs.size());

		for (auto& g : geom_object->geometry()) {
			auto s = std::static_pointer_cast<ifcopenshell::geometry::CgalShape>(g.Shape())->poly();
			const auto& m = g.Placement()->ccomponents();
			const auto& n = geom_object->transformation().data()->ccomponents();

			const cgal_placement_t trsf(
				m(0, 0), m(0, 1), m(0, 2), m(0, 3),
				m(1, 0), m(1, 1), m(1, 2), m(1, 3),
				m(2, 0), m(2, 1), m(2, 2), m(2, 3));

			const cgal_placement_t trsf2(
				n(0, 0), n(0, 1), n(0, 2), n(0, 3),
				n(1, 0), n(1, 1), n(1, 2), n(1, 3),
				n(2, 0), n(2, 1), n(2, 2), n(2, 3));

			// Apply transformation
			for (auto &vertex : vertices(s)) {
				vertex->point() = vertex->point().transform(trsf).transform(trsf2);
			}

			/*
			{
				auto bounds = CGAL::Polygon_mesh_processing::bbox_3(s);
				for (int i = 0; i < 3; ++i) {
					std::wcout << bounds.min(i) << std::endl;
				}
				for (int i = 0; i < 3; ++i) {
					std::wcout << bounds.max(i) << std::endl;
				}
				std::wcout << "---" << std::endl;
			}
			*/

			CGAL::Nef_polyhedron_3<Kernel_> part_nef = ifcopenshell::geometry::utils::create_nef_polyhedron(s);

			if (!part_nef.is_simple()) {
				// std::wcout << "not simple" << std::endl;
				continue;
			}

			std::vector<double>::iterator accumulator = intersection_volumes.begin();
			std::for_each(nefs.begin(), nefs.end(), [&accumulator, &part_nef](const CGAL::Nef_polyhedron_3<Kernel_>& storey_nef) {
				auto poly = ifcopenshell::geometry::utils::create_polyhedron(part_nef * storey_nef);
				CGAL::Polygon_mesh_processing::triangulate_faces(poly);
				*accumulator += CGAL::to_double(CGAL::Polygon_mesh_processing::volume(poly));
				accumulator++;
			});
		}

		/*
		std::wcout << "volumes: ";
		for (auto& v : intersection_volumes) {
			std::wcout << v << " ";
		}
		std::wcout << std::endl;
		*/

		auto calc_idx = std::max_element(intersection_volumes.begin(), intersection_volumes.end()) - intersection_volumes.begin();
		auto calc_overlap = intersection_volumes[calc_idx];
		auto assigned_idx = std::distance(storeys_sorted.begin(), std::find(storeys_sorted.begin(), storeys_sorted.end(), elem_to_storey[geom_object->product()]));
		auto assigned_overlap = intersection_volumes[assigned_idx];
		if (calc_overlap > 0 && assigned_overlap < calc_overlap * 0.9) {
			auto s = geom_object->product()->get_value<std::string>("GlobalId");
			auto s1 = ((IfcUtil::IfcBaseEntity*)storeys_sorted[calc_idx])->get_value<std::string>("GlobalId");
			auto s2 = ((IfcUtil::IfcBaseEntity*)elem_to_storey[geom_object->product()])->get_value<std::string>("GlobalId");
			Logger::Error("Element " + s + " contained in " + s2 + " located on " + s1);
		}

		if (!no_progress) {
			if (quiet) {
				const int progress = context_iterator.progress();
				for (; old_progress < progress; ++old_progress) {
					std::cout << ".";
					if (stderr_progress)
						std::cerr << ".";
				}
				std::cout << std::flush;
				if (stderr_progress)
					std::cerr << std::flush;
			} else {
				const int progress = context_iterator.progress() / 2;
				if (old_progress != progress) Logger::ProgressBar(progress);
				old_progress = progress;
			}
		}
	}

	if (!no_progress && quiet) {
		for (; old_progress < 100; ++old_progress) {
			std::cout << ".";
			if (stderr_progress)
				std::cerr << ".";
		}
		std::cout << std::flush;
		if (stderr_progress)
			std::cerr << std::flush;
	} else {
		Logger::Status("\rDone fixing space boundaries for " + boost::lexical_cast<std::string>(num_created) +
			" objects                                ");
	}
}

#endif
