#pragma once
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct o_tspace_context o_tspace_context;

typedef struct
{
	// returns number of faces (triangles / quads) in the mesh to be processed
	i32 (*get_num_faces)(const o_tspace_context* context);

	// returns the number of verts on the specified face number
	// the specified face number must be in the range 0-get_num_faces()-1
	i32 (*get_num_vertices_of_face)(const o_tspace_context* context, i32 face);
	
	// returns the position / normal / texcoord of the referenced face of vertex number vert
	// vert is in the range 0-2 for triangles and 0-3 for quads
	void (*get_position)(const o_tspace_context* context, f32* fv_pos_out, i32 face, i32 vert);
	void (*get_normal)(const o_tspace_context* context, f32* fv_norm_out, i32 face, i32 vert);
	void (*get_tex_coord)(const o_tspace_context* context, f32* fv_texc_out, i32 face, i32 vert);

	// either or both of the set_tspace callbacks can be set
	// the callback for set_tspace_basic() should be sufficient for most basic normal mapping

	// this function is used to return the tangent and f_sign to the application
	// fv_tanget is a unit length vector, for normal maps it is sufficient to use
	// the following simplified version of the bitangent which is generated at pixel / vertex level
	// bitangent = f_sign * cross(vN, tangnet);
	// NOTE: the results are returned unindexed, it is possible to generate a new index list
	// averaging / overwriting tangent spaces by using an already existing index list *will* produce WRONG results.
	// so don't use an already existing index list
	void (*set_tspace_basic)(const o_tspace_context* context, const f32* fv_tangent, f32 f_sign, i32 face, i32 vert);

	// this function is used to return tangent space results to the application
	// fv_tangent and fv_bitangent are unit length vectors and f_mag_s and f_mag_t are their
	// real magnitures which can be used for relief mapping effects
	// fv_bitangent is thre real bitangeent and thus may not be perpendicular to fv_tangent
	// however both are perpendicular to the vertex's normal, for normal maps it is sufficient to use the following
	// simplified version of the bitangent which is generated at pixel / vertex level
	// f_sign = b_is_orientation_preserving ? 1.0f : -1.0f;
	// bitangent = f_sign * cross(vN, tangent);
	// NOTE: the results are returned unindexed, it is possible to generate a new index list
	// averaging / overwriting tangent spaces by using an already existing index list *will* produce WRONG results.
	// so don't use an already existing index list
	void (*set_tspace)(const o_tspace_context* context, const f32* fv_tangent, const f32* fv_bitangent, f32 f_mag_s, f32 f_mag_t,
		u8 is_orientation_preserving, i32 face, i32 vert);
} o_tspace_interface;

struct o_tspace_context
{
	o_tspace_interface* tspace_interface;	// initialized with callback functions
	void* user_data;						// pointer to client side mesh data etc, passed as the first parameter with evey tspace_interface call
};

// these are thread safe
u8 generate_tangent_space_default(const o_tspace_context* context); // default (recommended) f_anular_threshold is 180 degress (aka threshold disabled);
u8 generate_tangent_space(const o_tspace_context* context, f32 f_angular_threshold);

// to avoid visual errors (distortions / bad hard edges in lighting) when using sampled normal maps,
// the normal map sampler should use the exact inverse of the pixel shader transformation.

#ifdef __cplusplus
}
#endif