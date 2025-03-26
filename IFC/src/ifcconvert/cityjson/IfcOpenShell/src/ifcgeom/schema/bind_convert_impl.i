﻿#ifdef BIND
#undef BIND
#endif

#define BIND(T) \
	if (l->declaration().is(IfcSchema::T::Class())) { \
		try { \
			taxonomy::item* item = map_impl((IfcSchema::T*)l); \
			if (item != nullptr) { \
				item->instance = l; \
				try { \
					if (l->as<IfcSchema::IfcRepresentationItem>() && !l->as<IfcSchema::IfcStyledItem>() && \
						/* @todo */ \
						(item->kind() == taxonomy::SHELL || item->kind() == taxonomy::COLLECTION || item->kind() == taxonomy::EXTRUSION) \
					) { \
						auto style = find_style(l->as<IfcSchema::IfcRepresentationItem>()); \
						if (style) { \
							((taxonomy::geom_item*)item)->surface_style = (taxonomy::style*) map(style); \
						} \
					} \
				} catch (const std::exception& e) { \
					Logger::Message(Logger::LOG_ERROR, std::string(e.what()) + "\nFailed to convert:", l); \
				} \
			} \
			return item; \
		} catch (const std::exception& e) { \
			Logger::Message(Logger::LOG_ERROR, std::string(e.what()) + "\nFailed to convert:", l); \
		} \
		return nullptr; \
	}

#include "mapping.i"
