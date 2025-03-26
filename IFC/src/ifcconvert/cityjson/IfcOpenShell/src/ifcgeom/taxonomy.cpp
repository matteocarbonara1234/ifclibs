#include "taxonomy.h"

using namespace ifcopenshell::geometry::taxonomy;

namespace {
	bool compare(const trimmed_curve& a, const trimmed_curve& b);

	bool compare(const collection& a, const collection& b);

	template <typename T>
	bool compare(const eigen_base<T>& t, const eigen_base<T>& u) {
		if (t.components_ == nullptr && u.components_ == nullptr) {
			return false;
		} else if (t.components_ == nullptr && u.components_ != nullptr) {
			return true;
		} else if (t.components_ != nullptr && u.components_ == nullptr) {
			return false;
		}

		auto t_begin = t.components_->data();
		auto t_end = t.components_->data() + t.components_->size();

		auto u_begin = u.components_->data();
		auto u_end = u.components_->data() + u.components_->size();

		return std::lexicographical_compare(t_begin, t_end, u_begin, u_end);
	}

	bool compare(const line& a, const line& b) {
		return compare(a.matrix, b.matrix);
	}

	bool compare(const plane& a, const plane& b) {
		return compare(a.matrix, b.matrix);
	}

	bool compare(const circle& a, const circle& b) {
		if (a.radius == b.radius) {
			return compare(a.matrix, b.matrix);
		}
		return a.radius < b.radius;
	}

	bool compare(const ellipse& a, const ellipse& b) {
		if (a.radius == b.radius && a.radius2 == b.radius2) {
			return compare(a.matrix, b.matrix);
		}
		return
			std::tie(a.radius, a.radius2) <
			std::tie(b.radius, b.radius2);
	}

	bool compare(const bspline_curve&, const bspline_curve&) {
		throw std::runtime_error("not implemented");
	}

	template <typename T>
	typename std::enable_if<std::is_base_of<item, T>::value, int>::type less_to_order(const T& a, const T& b) {
		const bool a_lt_b = compare(a, b);
		const bool b_lt_a = compare(b, a);
		return a_lt_b ?
			-1 : (!b_lt_a ? 0 : 1);
	}

	template <typename T>
	typename std::enable_if<!std::is_base_of<item, T>::value, int>::type less_to_order(const T& a, const T& b) {
		const bool a_lt_b = a < b;
		const bool b_lt_a = b < a;
		return a_lt_b ?
			-1 : (!b_lt_a ? 0 : 1);
	}

	template <typename T>
	int less_to_order_optional(const boost::optional<T>& a, const boost::optional<T>& b) {
		if (a && b) {
			return less_to_order(*a, *b);
		} else if (!a && !b) {
			return 0;
		} else if (a) {
			return 1;
		} else {
			return -1;
		}
	}

	int compare(const boost::variant<point3, double>& a, const boost::variant<point3, double>& b) {
		bool a_lt_b, b_lt_a;
		if (a.which() == 0) {
			a_lt_b = compare(boost::get<point3>(a), boost::get<point3>(b));
			b_lt_a = compare(boost::get<point3>(b), boost::get<point3>(a));
		} else {
			a_lt_b = std::less<double>()(boost::get<double>(a), boost::get<double>(b));
			b_lt_a = std::less<double>()(boost::get<double>(b), boost::get<double>(a));
		}
		return a_lt_b ?
			-1 : (!b_lt_a ? 0 : 1);
	}

	bool compare(const extrusion& a, const extrusion& b) {
		const int order[3] = {
			less_to_order(a.basis, b.basis),
			less_to_order(a.direction, b.direction),
			a.depth < b.depth ? -1 : (a.depth == b.depth ? 0 : 1)
		};
		// find the first non-zero integer.
		auto it = std::find_if(std::begin(order), std::end(order), [](int x) { return x; });
		if (it == std::end(order)) {
			// extrusions can have non-identity matrices
			// 
			// @todo perhaps it's time for a dedicated transform
			//       node and not on the abstract geom_item.
			return compare(a.matrix, b.matrix);
		}
		return *it == -1;
	}

	bool compare(const style& a, const style& b) {
		const int order[5] = {
			less_to_order(a.name, b.name),
			less_to_order(a.diffuse, b.diffuse),
			less_to_order(a.specular, b.specular),
			less_to_order(a.specularity, b.specularity),
			less_to_order(a.transparency, b.transparency)
		};
		auto it = std::find_if(std::begin(order), std::end(order), [](int x) { return x; });
		if (it == std::end(order)) return false;
		return *it == -1;
	}

	/* A compile-time for loop over the taxonomy kinds */
	template <size_t N>
	struct dispatch_comparison {
		static bool dispatch(const item* a, const item* b) {
			if (N == a->kind() && N == b->kind()) {
				auto A = static_cast<const type_by_kind::type<N>*>(a);
				auto B = static_cast<const type_by_kind::type<N>*>(b);
				return compare(*A, *B);
			} else {
				return dispatch_comparison<N + 1>::dispatch(a, b);
			}
		}
	};

	template <>
	struct dispatch_comparison<type_by_kind::max> {
		static bool dispatch(const item*, const item*) {
			return false;
		}
	};
}

bool ifcopenshell::geometry::taxonomy::less(const item* a, const item* b) {
	if (a == b) {
		return false;
	}

	int a_kind = a->kind();
	int b_kind = b->kind();

	if (a_kind != b_kind) {
		return a_kind < b_kind;
	}

	return dispatch_comparison<0>::dispatch(a, b);
}


namespace {
	bool compare(const trimmed_curve& a, const trimmed_curve& b) {
		int a_which_start = a.start.which();
		int a_which_end = a.end.which();
		int b_which_start = b.start.which();
		int b_which_end = b.end.which();
		if (std::tie(a.orientation, a_which_start, a_which_end) ==
			std::tie(b.orientation, b_which_start, b_which_end)) {

			int start_state = compare(a.start, b.start);

			if (start_state == 0) {

				int end_state = compare(a.end, b.end);

				if (end_state == 0) {

					int a_has_basis = !!a.basis;
					int b_has_basis = !!a.basis;

					if (a_has_basis == b_has_basis) {

						if (!a_has_basis) {
							// Finally, equality
							return false;
						} else {
							return less(a.basis, b.basis);
						}

					} else {
						return a_has_basis < b_has_basis;
					}

				} else {
					return end_state == -1;
				}

			} else {
				return start_state == -1;
			}

		} else {
			return
				std::tie(a.orientation, a_which_start, a_which_end) <
				std::tie(b.orientation, b_which_start, b_which_end);
		}
	}

	bool compare(const collection& a, const collection& b) {
		if (a.children.size() == b.children.size()) {
			auto at = a.children.begin();
			auto bt = b.children.begin();
			for (; at != a.children.end(); ++at, ++bt) {
				const bool a_lt_b = less(*at, *bt);
				const bool b_lt_a = less(*bt, *at);
				if (!a_lt_b && !b_lt_a) {
					// Elements equal.
					continue;
				}
				return a_lt_b;
			}
			// Vectors equal, compare matrix (in case of mapped items).
			int matrix_order = less_to_order(a.matrix, b.matrix);
			if (matrix_order == 0) {

				if (a.surface_style == nullptr && b.surface_style == nullptr) {
					return false;
				} else if (a.surface_style == nullptr && b.surface_style != nullptr) {
					return true;
				} else if (a.surface_style != nullptr && b.surface_style == nullptr) {
					return false;
				}

				return compare(*a.surface_style, *b.surface_style);
			} else {
				return matrix_order == -1;
			}
		} else {
			return a.children.size() < b.children.size();
		}
	}
}