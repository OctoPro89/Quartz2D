#include "../include/octomath/o_tspace.h"
#include "../include/octomath/vec3.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define epsilon 1e-5f
#define INTERNAL_RND_SORT_SEED 39871946

static u8 veq(const vec3 v1, const vec3 v2)
{
	return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
}

static vec3	v_add(const vec3 v1, const vec3 v2)
{
	vec3 v_res;

	v_res.x = v1.x + v2.x;
	v_res.y = v1.y + v2.y;
	v_res.z = v1.z + v2.z;

	return v_res;
}


static vec3	v_sub(const vec3 v1, const vec3 v2)
{
	vec3 v_res;

	v_res.x = v1.x - v2.x;
	v_res.y = v1.y - v2.y;
	v_res.z = v1.z - v2.z;

	return v_res;
}

static vec3	v_scale(const float fS, const vec3 v)
{
	vec3 v_res;

	v_res.x = fS * v.x;
	v_res.y = fS * v.y;
	v_res.z = fS * v.z;

	return v_res;
}

static float length_squared(const vec3 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

static float length(const vec3 v)
{
	return sqrtf(length_squared(v));
}

static vec3	normalize(const vec3 v)
{
	return v_scale(1 / length(v), v);
}

static float v_dot(const vec3 v1, const vec3 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

static u8 not_zero(const float fX)
{
	return fabsf(fX) > epsilon;
}

static u8 v_not_zero(const vec3 v)
{
	return not_zero(v.x) || not_zero(v.y) || not_zero(v.z);
}

typedef struct {
	int i_nr_faces;
	int* p_tri_members;
} o_sub_group;

typedef struct {
	int i_nr_faces;
	int* p_face_indices;
	int i_vertex_representitive;
	u8 b_orient_preserving;
} o_group;

// 
#define MARK_DEGENERATE				1
#define QUAD_ONE_DEGEN_TRI			2
#define GROUP_WITH_ANY				4
#define ORIENT_PRESERVING			8



typedef struct {
	int face_neighbors[3];
	o_group* assigned_group[3];

	// normalized first order face derivatives
	vec3 vOs, vOt;
	float f_mag_s, f_mag_t;	// original magnitudes

	// determines if the current and the next triangle are a quad.
	int i_org_face_number;
	int i_flag, i_tspaces_offset;
	unsigned char vert_num[4];
} o_tri_info;

typedef struct {
	vec3 vOs;
	float f_mag_s;
	vec3 vOt;
	float f_mag_t;
	int i_counter;	// this is to average back into quads.
	u8 b_orient;
} o_tspace;

static int generate_initial_vertices_index_list(o_tri_info p_tri_infos[], int pi_tri_list_out[], const o_tspace_context * context, const int i_nr_triangles_in);
static void generate_shared_vertices_index_list(int pi_tri_list_in_and_out[], const o_tspace_context * context, const int i_nr_triangles_in);
static void init_tri_info(o_tri_info p_tri_infos[], const int pi_tri_list_in[], const o_tspace_context * context, const int i_nr_triangles_in);
static int build_4_rule_groups(o_tri_info p_tri_infos[], o_group p_groups[], int pi_group_triangles_buffer[], const int pi_tri_list_in[], const int i_nr_triangles_in);
static u8 generate_tspaces(o_tspace ps_tspace[], const o_tri_info p_tri_infos[], const o_group p_groups[],
	const int i_nr_active_groups, const int pi_tri_list_in[], const float f_thres_cos,
	const o_tspace_context * context);

static int make_index(const int i_face, const int i_vert)
{
	assert(i_vert >= 0 && i_vert < 4 && i_face >= 0);
	return (i_face << 2) | (i_vert & 0x3);
}

static void index_to_data(int* pi_face, int* pi_vert, const int i_index_in)
{
	pi_vert[0] = i_index_in & 0x3;
	pi_face[0] = i_index_in >> 2;
}

static o_tspace average_tspace(const o_tspace * pTS0, const o_tspace * pTS1)
{
	o_tspace ts_res;

	// this if is important. Due to floating point precision
	// averaging when ts0==ts1 will cause a slight difference
	// which results in tangent space splits later on
	if (pTS0->f_mag_s == pTS1->f_mag_s && pTS0->f_mag_t == pTS1->f_mag_t &&
		veq(pTS0->vOs, pTS1->vOs) && veq(pTS0->vOt, pTS1->vOt))
	{
		ts_res.f_mag_s = pTS0->f_mag_s;
		ts_res.f_mag_t = pTS0->f_mag_t;
		ts_res.vOs = pTS0->vOs;
		ts_res.vOt = pTS0->vOt;
	}
	else
	{
		ts_res.f_mag_s = 0.5f * (pTS0->f_mag_s + pTS1->f_mag_s);
		ts_res.f_mag_t = 0.5f * (pTS0->f_mag_t + pTS1->f_mag_t);
		ts_res.vOs = v_add(pTS0->vOs, pTS1->vOs);
		ts_res.vOt = v_add(pTS0->vOt, pTS1->vOt);
		if (v_not_zero(ts_res.vOs)) ts_res.vOs = normalize(ts_res.vOs);
		if (v_not_zero(ts_res.vOt)) ts_res.vOt = normalize(ts_res.vOt);
	}

	return ts_res;
}

static vec3 get_position(const o_tspace_context* context, const int index);
static vec3 get_normal(const o_tspace_context* context, const int index);
static vec3 get_tex_coord(const o_tspace_context* context, const int index);

// degen triangles
static void degen_prologue(o_tri_info p_tri_infos[], int pi_tri_list_out[], const int i_nr_triangles_in, const int i_tot_tris);
static void degen_epilogue(o_tspace ps_tspace[], o_tri_info p_tri_infos[], int pi_tri_list_in[], const o_tspace_context* context, const int i_nr_triangles_in, const int i_tot_tris);

u8 generate_tangent_space_default(const o_tspace_context* context)
{
	return generate_tangent_space(context, 180.0f);
}

u8 generate_tangent_space(const o_tspace_context* context, const float f_angular_threshold)
{
	// count nr_triangles
	int* pi_tri_list_in = NULL, * pi_group_triangles_buffer = NULL;
	o_tri_info* p_tri_infos = NULL;
	o_group* p_groups = NULL;
	o_tspace* ps_tspace = NULL;
	int i_nr_triangles_in = 0, f = 0, t = 0, i = 0;
	int i_nr_tspaces = 0, i_tot_tris = 0, i_degen_triangles = 0, i_nr_max_groups = 0;
	int i_nr_active_groups = 0, index = 0;
	const int i_nr_faces = context->tspace_interface->get_num_faces(context);
	u8 bRes = false;
	const float f_thres_cos = (float)cos((f_angular_threshold * (float)PI) / 180.0f);

	// verify all call-backs have been set
	if (context->tspace_interface->get_num_faces == NULL ||
		context->tspace_interface->get_num_vertices_of_face == NULL ||
		context->tspace_interface->get_position == NULL ||
		context->tspace_interface->get_normal == NULL ||
		context->tspace_interface->get_tex_coord == NULL)
		return false;

	// count triangles on supported faces
	for (f = 0; f < i_nr_faces; f++)
	{
		const int verts = context->tspace_interface->get_num_vertices_of_face(context, f);
		if (verts == 3) ++i_nr_triangles_in;
		else if (verts == 4) i_nr_triangles_in += 2;
	}
	if (i_nr_triangles_in <= 0) return false;

	// allocate memory for an index list
	pi_tri_list_in = (int*)malloc(sizeof(int) * 3 * i_nr_triangles_in);
	p_tri_infos = (o_tri_info*)malloc(sizeof(o_tri_info) * i_nr_triangles_in);
	if (pi_tri_list_in == NULL || p_tri_infos == NULL)
	{
		if (pi_tri_list_in != NULL) free(pi_tri_list_in);
		if (p_tri_infos != NULL) free(p_tri_infos);
		return false;
	}

	// make an initial triangle --> face index list
	i_nr_tspaces = generate_initial_vertices_index_list(p_tri_infos, pi_tri_list_in, context, i_nr_triangles_in);

	// make a welded index list of identical positions and attributes (pos, norm, texc)
	//printf("gen welded index list begin\n");
	generate_shared_vertices_index_list(pi_tri_list_in, context, i_nr_triangles_in);
	//printf("gen welded index list end\n");

	// Mark all degenerate triangles
	i_tot_tris = i_nr_triangles_in;
	i_degen_triangles = 0;
	for (t = 0; t < i_tot_tris; t++)
	{
		const int i0 = pi_tri_list_in[t * 3];
		const int i1 = pi_tri_list_in[t * 3 + 1];
		const int i2 = pi_tri_list_in[t * 3 + 2];
		const vec3 p0 = get_position(context, i0);
		const vec3 p1 = get_position(context, i1);
		const vec3 p2 = get_position(context, i2);
		if (veq(p0, p1) || veq(p0, p2) || veq(p1, p2))	// degenerate
		{
			p_tri_infos[t].i_flag |= MARK_DEGENERATE;
			++i_degen_triangles;
		}
	}
	i_nr_triangles_in = i_tot_tris - i_degen_triangles;

	// mark all triangle pairs that belong to a quad with only one
	// good triangle. These need special treatment in degen_epilogue().
	// Additionally, move all good triangles to the start of
	// p_tri_infos[] and pi_tri_list_in[] without changing order and
	// put the degenerate triangles last.
	degen_prologue(p_tri_infos, pi_tri_list_in, i_nr_triangles_in, i_tot_tris);


	// evaluate triangle level attributes and neighbor list
	//printf("gen neighbors list begin\n");
	init_tri_info(p_tri_infos, pi_tri_list_in, context, i_nr_triangles_in);
	//printf("gen neighbors list end\n");


	// based on the 4 rules, identify groups based on connectivity
	i_nr_max_groups = i_nr_triangles_in * 3;
	p_groups = (o_group*)malloc(sizeof(o_group) * i_nr_max_groups);
	pi_group_triangles_buffer = (int*)malloc(sizeof(int) * i_nr_triangles_in * 3);
	if (p_groups == NULL || pi_group_triangles_buffer == NULL)
	{
		if (p_groups != NULL) free(p_groups);
		if (pi_group_triangles_buffer != NULL) free(pi_group_triangles_buffer);
		free(pi_tri_list_in);
		free(p_tri_infos);
		return false;
	}
	//printf("gen 4rule groups begin\n");
	i_nr_active_groups =
		build_4_rule_groups(p_tri_infos, p_groups, pi_group_triangles_buffer, pi_tri_list_in, i_nr_triangles_in);
	//printf("gen 4rule groups end\n");

	//

	ps_tspace = (o_tspace*)malloc(sizeof(o_tspace) * i_nr_tspaces);
	if (ps_tspace == NULL)
	{
		free(pi_tri_list_in);
		free(p_tri_infos);
		free(p_groups);
		free(pi_group_triangles_buffer);
		return false;
	}
	memset(ps_tspace, 0, sizeof(o_tspace) * i_nr_tspaces);
	for (t = 0; t < i_nr_tspaces; t++)
	{
		ps_tspace[t].vOs.x = 1.0f; ps_tspace[t].vOs.y = 0.0f; ps_tspace[t].vOs.z = 0.0f; ps_tspace[t].f_mag_s = 1.0f;
		ps_tspace[t].vOt.x = 0.0f; ps_tspace[t].vOt.y = 1.0f; ps_tspace[t].vOt.z = 0.0f; ps_tspace[t].f_mag_t = 1.0f;
	}

	// make tspaces, each group is split up into subgroups if necessary
	// based on f_angular_threshold. Finally a tangent space is made for
	// every resulting subgroup
	//printf("gen tspaces begin\n");
	bRes = generate_tspaces(ps_tspace, p_tri_infos, p_groups, i_nr_active_groups, pi_tri_list_in, f_thres_cos, context);
	//printf("gen tspaces end\n");

	// clean up
	free(p_groups);
	free(pi_group_triangles_buffer);

	if (!bRes)	// if an allocation in generate_tspaces() failed
	{
		// clean up and return false
		free(p_tri_infos); free(pi_tri_list_in); free(ps_tspace);
		return false;
	}


	// degenerate quads with one good triangle will be fixed by copying a space from
	// the good triangle to the coinciding vertex.
	// all other degenerate triangles will just copy a space from any good triangle
	// with the same welded index in pi_tri_list_in[].
	degen_epilogue(ps_tspace, p_tri_infos, pi_tri_list_in, context, i_nr_triangles_in, i_tot_tris);

	free(p_tri_infos); free(pi_tri_list_in);

	index = 0;
	for (f = 0; f < i_nr_faces; f++)
	{
		const int verts = context->tspace_interface->get_num_vertices_of_face(context, f);
		if (verts != 3 && verts != 4) continue;


		// I've decided to let degenerate triangles and group-with-anythings
		// vary between left/right hand coordinate systems at the vertices.
		// All healthy triangles on the other hand are built to always be either or.

		/*// force the coordinate system orientation to be uniform for every face.
		// (this is already the case for good triangles but not for
		// degenerate ones and those with bGroupWithAnything==true)
		u8 b_orient = ps_tspace[index].b_orient;
		if (ps_tspace[index].i_counter == 0)	// tspace was not derived from a group
		{
			// look for a space created in generate_tspaces() by i_counter>0
			u8 b_not_found = true;
			int i=1;
			while (i<verts && b_not_found)
			{
				if (ps_tspace[index+i].i_counter > 0) b_not_found=false;
				else ++i;
			}
			if (!b_not_found) b_orient = ps_tspace[index+i].b_orient;
		}*/

		// set data
		for (i = 0; i < verts; i++)
		{
			const o_tspace* p_tspace = &ps_tspace[index];
			float tang[] = { p_tspace->vOs.x, p_tspace->vOs.y, p_tspace->vOs.z };
			float bitang[] = { p_tspace->vOt.x, p_tspace->vOt.y, p_tspace->vOt.z };
			if (context->tspace_interface->set_tspace != NULL)
				context->tspace_interface->set_tspace(context, tang, bitang, p_tspace->f_mag_s, p_tspace->f_mag_t, p_tspace->b_orient, f, i);
			if (context->tspace_interface->set_tspace_basic != NULL)
				context->tspace_interface->set_tspace_basic(context, tang, p_tspace->b_orient == true ? 1.0f : (-1.0f), f, i);

			++index;
		}
	}

	free(ps_tspace);


	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
	float vert[3];
	int index;
} o_tmp_vert;

static const int g_i_cells = 2048;

#ifdef _MSC_VER
	#define NOINLINE __declspec(noinline)
#else
	#define NOINLINE __attribute__ ((noinline))
#endif

// it is IMPORTANT that this function is called to evaluate the hash since
// inlining could potentially reorder instructions and generate different
// results for the same effective input value f_val.
static NOINLINE int find_grid_cell(const float f_min, const float f_max, const float f_val)
{
	const float f_index = g_i_cells * ((f_val - f_min) / (f_max - f_min));
	const int i_index = (int)f_index;
	return i_index < g_i_cells ? (i_index >= 0 ? i_index : 0) : (g_i_cells - 1);
}

static void merge_verts_fast(int pi_tri_list_in_and_out[], o_tmp_vert tmp_vert[], const o_tspace_context * context, const int iL_in, const int iR_in);
static void merge_verts_slow(int pi_tri_list_in_and_out[], const o_tspace_context * context, const int table[], const int entries);
static void generate_shared_vertices_index_list_slow(int pi_tri_list_in_and_out[], const o_tspace_context * context, const int i_nr_triangles_in);

static void generate_shared_vertices_index_list(int pi_tri_list_in_and_out[], const o_tspace_context * context, const int i_nr_triangles_in)
{

	// Generate bounding box
	int* pi_hash_table = NULL, * pi_hash_count = NULL, * pi_hash_offsets = NULL, * pi_hash_count_2 = NULL;
	o_tmp_vert* tmp_vert = NULL;
	int i = 0, i_channel = 0, k = 0, e = 0;
	int i_max_count = 0;
	vec3 vMin = get_position(context, 0), vMax = vMin, vDim;
	float f_min, f_max;
	for (i = 1; i < (i_nr_triangles_in * 3); i++)
	{
		const int index = pi_tri_list_in_and_out[i];

		const vec3 vP = get_position(context, index);
		if (vMin.x > vP.x) vMin.x = vP.x;
		else if (vMax.x < vP.x) vMax.x = vP.x;
		if (vMin.y > vP.y) vMin.y = vP.y;
		else if (vMax.y < vP.y) vMax.y = vP.y;
		if (vMin.z > vP.z) vMin.z = vP.z;
		else if (vMax.z < vP.z) vMax.z = vP.z;
	}

	vDim = v_sub(vMax, vMin);
	i_channel = 0;
	f_min = vMin.x; f_max = vMax.x;
	if (vDim.y > vDim.x && vDim.y > vDim.z)
	{
		i_channel = 1;
		f_min = vMin.y;
		f_max = vMax.y;
	}
	else if (vDim.z > vDim.x)
	{
		i_channel = 2;
		f_min = vMin.z;
		f_max = vMax.z;
	}

	// make allocations
	pi_hash_table = (int*)malloc(sizeof(int) * i_nr_triangles_in * 3);
	pi_hash_count = (int*)malloc(sizeof(int) * g_i_cells);
	pi_hash_offsets = (int*)malloc(sizeof(int) * g_i_cells);
	pi_hash_count_2 = (int*)malloc(sizeof(int) * g_i_cells);

	if (pi_hash_table == NULL || pi_hash_count == NULL || pi_hash_offsets == NULL || pi_hash_count_2 == NULL)
	{
		if (pi_hash_table != NULL) free(pi_hash_table);
		if (pi_hash_count != NULL) free(pi_hash_count);
		if (pi_hash_offsets != NULL) free(pi_hash_offsets);
		if (pi_hash_count_2 != NULL) free(pi_hash_count_2);
		generate_shared_vertices_index_list_slow(pi_tri_list_in_and_out, context, i_nr_triangles_in);
		return;
	}
	memset(pi_hash_count, 0, sizeof(int) * g_i_cells);
	memset(pi_hash_count_2, 0, sizeof(int) * g_i_cells);

	// count amount of elements in each cell unit
	for (i = 0; i < (i_nr_triangles_in * 3); i++)
	{
		const int index = pi_tri_list_in_and_out[i];
		const vec3 vP = get_position(context, index);
		const float f_val = i_channel == 0 ? vP.x : (i_channel == 1 ? vP.y : vP.z);
		const int i_cell = find_grid_cell(f_min, f_max, f_val);
		++pi_hash_count[i_cell];
	}

	// evaluate start index of each cell.
	pi_hash_offsets[0] = 0;
	for (k = 1; k < g_i_cells; k++)
		pi_hash_offsets[k] = pi_hash_offsets[k - 1] + pi_hash_count[k - 1];

	// insert vertices
	for (i = 0; i < (i_nr_triangles_in * 3); i++)
	{
		const int index = pi_tri_list_in_and_out[i];
		const vec3 vP = get_position(context, index);
		const float f_val = i_channel == 0 ? vP.x : (i_channel == 1 ? vP.y : vP.z);
		const int i_cell = find_grid_cell(f_min, f_max, f_val);
		int* table = NULL;

		assert(pi_hash_count_2[i_cell] < pi_hash_count[i_cell]);
		table = &pi_hash_table[pi_hash_offsets[i_cell]];
		table[pi_hash_count_2[i_cell]] = i;	// vertex i has been inserted.
		++pi_hash_count_2[i_cell];
	}
	for (k = 0; k < g_i_cells; k++)
		assert(pi_hash_count_2[k] == pi_hash_count[k]);	// verify the count
	free(pi_hash_count_2);

	// find maximum amount of entries in any hash entry
	i_max_count = pi_hash_count[0];
	for (k = 1; k < g_i_cells; k++)
		if (i_max_count < pi_hash_count[k])
			i_max_count = pi_hash_count[k];
	tmp_vert = (o_tmp_vert*)malloc(sizeof(o_tmp_vert) * i_max_count);


	// complete the merge
	for (k = 0; k < g_i_cells; k++)
	{
		// extract table of cell k and amount of entries in it
		int* table = &pi_hash_table[pi_hash_offsets[k]];
		const int entries = pi_hash_count[k];
		if (entries < 2) continue;

		if (tmp_vert != NULL)
		{
			for (e = 0; e < entries; e++)
			{
				int i = table[e];
				const vec3 vP = get_position(context, pi_tri_list_in_and_out[i]);
				tmp_vert[e].vert[0] = vP.x; tmp_vert[e].vert[1] = vP.y;
				tmp_vert[e].vert[2] = vP.z; tmp_vert[e].index = i;
			}
			merge_verts_fast(pi_tri_list_in_and_out, tmp_vert, context, 0, entries - 1);
		}
		else
			merge_verts_slow(pi_tri_list_in_and_out, context, table, entries);
	}

	if (tmp_vert != NULL) { free(tmp_vert); }
	free(pi_hash_table);
	free(pi_hash_count);
	free(pi_hash_offsets);
}

static void merge_verts_fast(int pi_tri_list_in_and_out[], o_tmp_vert tmp_vert[], const o_tspace_context * context, const int iL_in, const int iR_in)
{
	// make bbox
	int c = 0, l = 0, channel = 0;
	float fv_min[3], fv_max[3];
	float dx = 0, dy = 0, dz = 0, fSep = 0;
	for (c = 0; c < 3; c++)
	{
		fv_min[c] = tmp_vert[iL_in].vert[c]; fv_max[c] = fv_min[c];
	}
	for (l = (iL_in + 1); l <= iR_in; l++) {
		for (c = 0; c < 3; c++) {
			if (fv_min[c] > tmp_vert[l].vert[c]) fv_min[c] = tmp_vert[l].vert[c];
			if (fv_max[c] < tmp_vert[l].vert[c]) fv_max[c] = tmp_vert[l].vert[c];
		}
	}

	dx = fv_max[0] - fv_min[0];
	dy = fv_max[1] - fv_min[1];
	dz = fv_max[2] - fv_min[2];

	channel = 0;
	if (dy > dx && dy > dz) channel = 1;
	else if (dz > dx) channel = 2;

	fSep = 0.5f * (fv_max[channel] + fv_min[channel]);

	// stop if all vertices are NaNs
	if (!isfinite(fSep))
		return;

	// terminate recursion when the separation/average value
	// is no longer strictly between f_min and f_max values.
	if (fSep >= fv_max[channel] || fSep <= fv_min[channel])
	{
		// complete the weld
		for (l = iL_in; l <= iR_in; l++)
		{
			int i = tmp_vert[l].index;
			const int index = pi_tri_list_in_and_out[i];
			const vec3 vP = get_position(context, index);
			const vec3 vN = get_normal(context, index);
			const vec3 vT = get_tex_coord(context, index);

			u8 b_not_found = true;
			int l2 = iL_in, i2rec = -1;
			while (l2 < l && b_not_found)
			{
				const int i2 = tmp_vert[l2].index;
				const int index_2 = pi_tri_list_in_and_out[i2];
				const vec3 vP2 = get_position(context, index_2);
				const vec3 vN2 = get_normal(context, index_2);
				const vec3 vT2 = get_tex_coord(context, index_2);
				i2rec = i2;

				//if (vP==vP2 && vN==vN2 && vT==vT2)
				if (vP.x == vP2.x && vP.y == vP2.y && vP.z == vP2.z &&
					vN.x == vN2.x && vN.y == vN2.y && vN.z == vN2.z &&
					vT.x == vT2.x && vT.y == vT2.y && vT.z == vT2.z)
					b_not_found = false;
				else
					++l2;
			}

			// merge if previously found
			if (!b_not_found)
				pi_tri_list_in_and_out[i] = pi_tri_list_in_and_out[i2rec];
		}
	}
	else
	{
		int iL = iL_in, iR = iR_in;
		assert((iR_in - iL_in) > 0);	// at least 2 entries

		// separate (by fSep) all points between iL_in and iR_in in tmp_vert[]
		while (iL < iR)
		{
			u8 b_ready_left_swap = false, b_ready_right_swap = false;
			while ((!b_ready_left_swap) && iL < iR)
			{
				assert(iL >= iL_in && iL <= iR_in);
				b_ready_left_swap = !(tmp_vert[iL].vert[channel] < fSep);
				if (!b_ready_left_swap) ++iL;
			}
			while ((!b_ready_right_swap) && iL < iR)
			{
				assert(iR >= iL_in && iR <= iR_in);
				b_ready_right_swap = tmp_vert[iR].vert[channel] < fSep;
				if (!b_ready_right_swap) --iR;
			}
			assert((iL < iR) || !(b_ready_left_swap && b_ready_right_swap));

			if (b_ready_left_swap && b_ready_right_swap)
			{
				const o_tmp_vert sTmp = tmp_vert[iL];
				assert(iL < iR);
				tmp_vert[iL] = tmp_vert[iR];
				tmp_vert[iR] = sTmp;
				++iL; --iR;
			}
		}

		assert(iL == (iR + 1) || (iL == iR));
		if (iL == iR)
		{
			const u8 b_ready_right_swap = tmp_vert[iR].vert[channel] < fSep;
			if (b_ready_right_swap) ++iL;
			else --iR;
		}

		// only need to weld when there is more than 1 instance of the (x,y,z)
		if (iL_in < iR)
			merge_verts_fast(pi_tri_list_in_and_out, tmp_vert, context, iL_in, iR);	// weld all left of fSep
		if (iL < iR_in)
			merge_verts_fast(pi_tri_list_in_and_out, tmp_vert, context, iL, iR_in);	// weld all right of (or equal to) fSep
	}
}

static void merge_verts_slow(int pi_tri_list_in_and_out[], const o_tspace_context * context, const int table[], const int entries)
{
	// this can be optimized further using a tree structure or more hashing.
	int e = 0;
	for (e = 0; e < entries; e++)
	{
		int i = table[e];
		const int index = pi_tri_list_in_and_out[i];
		const vec3 vP = get_position(context, index);
		const vec3 vN = get_normal(context, index);
		const vec3 vT = get_tex_coord(context, index);

		u8 b_not_found = true;
		int e2 = 0, i2rec = -1;
		while (e2 < e && b_not_found)
		{
			const int i2 = table[e2];
			const int index_2 = pi_tri_list_in_and_out[i2];
			const vec3 vP2 = get_position(context, index_2);
			const vec3 vN2 = get_normal(context, index_2);
			const vec3 vT2 = get_tex_coord(context, index_2);
			i2rec = i2;

			if (veq(vP, vP2) && veq(vN, vN2) && veq(vT, vT2))
				b_not_found = false;
			else
				++e2;
		}

		// merge if previously found
		if (!b_not_found)
			pi_tri_list_in_and_out[i] = pi_tri_list_in_and_out[i2rec];
	}
}

static void generate_shared_vertices_index_list_slow(int pi_tri_list_in_and_out[], const o_tspace_context * context, const int i_nr_triangles_in)
{
	int i_num_unique_verts = 0, t = 0, i = 0;
	for (t = 0; t < i_nr_triangles_in; t++)
	{
		for (i = 0; i < 3; i++)
		{
			const int offs = t * 3 + i;
			const int index = pi_tri_list_in_and_out[offs];

			const vec3 vP = get_position(context, index);
			const vec3 vN = get_normal(context, index);
			const vec3 vT = get_tex_coord(context, index);

			u8 b_found = false;
			int t2 = 0, index_2_rec = -1;
			while (!b_found && t2 <= t)
			{
				int j = 0;
				while (!b_found && j < 3)
				{
					const int index_2 = pi_tri_list_in_and_out[t2 * 3 + j];
					const vec3 vP2 = get_position(context, index_2);
					const vec3 vN2 = get_normal(context, index_2);
					const vec3 vT2 = get_tex_coord(context, index_2);

					if (veq(vP, vP2) && veq(vN, vN2) && veq(vT, vT2))
						b_found = true;
					else
						++j;
				}
				if (!b_found) ++t2;
			}

			assert(b_found);
			// if we found our own
			if (index_2_rec == index) { ++i_num_unique_verts; }

			pi_tri_list_in_and_out[offs] = index_2_rec;
		}
	}
}

static int generate_initial_vertices_index_list(o_tri_info p_tri_infos[], int pi_tri_list_out[], const o_tspace_context * context, const int i_nr_triangles_in)
{
	int i_tspaces_offset = 0, f = 0, t = 0;
	int i_dst_tri_index = 0;
	for (f = 0; f < context->tspace_interface->get_num_faces(context); f++)
	{
		const int verts = context->tspace_interface->get_num_vertices_of_face(context, f);
		if (verts != 3 && verts != 4) continue;

		p_tri_infos[i_dst_tri_index].i_org_face_number = f;
		p_tri_infos[i_dst_tri_index].i_tspaces_offset = i_tspaces_offset;

		if (verts == 3)
		{
			unsigned char* p_verts = p_tri_infos[i_dst_tri_index].vert_num;
			p_verts[0] = 0; p_verts[1] = 1; p_verts[2] = 2;
			pi_tri_list_out[i_dst_tri_index * 3] = make_index(f, 0);
			pi_tri_list_out[i_dst_tri_index * 3 + 1] = make_index(f, 1);
			pi_tri_list_out[i_dst_tri_index * 3 + 2] = make_index(f, 2);
			++i_dst_tri_index;	// next
		}
		else
		{
			{
				p_tri_infos[i_dst_tri_index + 1].i_org_face_number = f;
				p_tri_infos[i_dst_tri_index + 1].i_tspaces_offset = i_tspaces_offset;
			}

			{
				// need an order independent way to evaluate
				// tspace on quads. This is done by splitting
				// along the shortest diagonal.
				const int i0 = make_index(f, 0);
				const int i1 = make_index(f, 1);
				const int i2 = make_index(f, 2);
				const int i3 = make_index(f, 3);
				const vec3 T0 = get_tex_coord(context, i0);
				const vec3 T1 = get_tex_coord(context, i1);
				const vec3 T2 = get_tex_coord(context, i2);
				const vec3 T3 = get_tex_coord(context, i3);
				const float distSQ_02 = length_squared(v_sub(T2, T0));
				const float distSQ_13 = length_squared(v_sub(T3, T1));
				u8 bQuadDiagIs_02;
				if (distSQ_02 < distSQ_13)
					bQuadDiagIs_02 = true;
				else if (distSQ_13 < distSQ_02)
					bQuadDiagIs_02 = false;
				else
				{
					const vec3 P0 = get_position(context, i0);
					const vec3 P1 = get_position(context, i1);
					const vec3 P2 = get_position(context, i2);
					const vec3 P3 = get_position(context, i3);
					const float distSQ_02 = length_squared(v_sub(P2, P0));
					const float distSQ_13 = length_squared(v_sub(P3, P1));

					bQuadDiagIs_02 = distSQ_13 < distSQ_02 ? false : true;
				}

				if (bQuadDiagIs_02)
				{
					{
						unsigned char* pVerts_A = p_tri_infos[i_dst_tri_index].vert_num;
						pVerts_A[0] = 0; pVerts_A[1] = 1; pVerts_A[2] = 2;
					}
					pi_tri_list_out[i_dst_tri_index * 3] = i0;
					pi_tri_list_out[i_dst_tri_index * 3 + 1] = i1;
					pi_tri_list_out[i_dst_tri_index * 3 + 2] = i2;
					++i_dst_tri_index;	// next
					{
						unsigned char* pVerts_B = p_tri_infos[i_dst_tri_index].vert_num;
						pVerts_B[0] = 0; pVerts_B[1] = 2; pVerts_B[2] = 3;
					}
					pi_tri_list_out[i_dst_tri_index * 3] = i0;
					pi_tri_list_out[i_dst_tri_index * 3 + 1] = i2;
					pi_tri_list_out[i_dst_tri_index * 3 + 2] = i3;
					++i_dst_tri_index;	// next
				}
				else
				{
					{
						unsigned char* pVerts_A = p_tri_infos[i_dst_tri_index].vert_num;
						pVerts_A[0] = 0; pVerts_A[1] = 1; pVerts_A[2] = 3;
					}
					pi_tri_list_out[i_dst_tri_index * 3] = i0;
					pi_tri_list_out[i_dst_tri_index * 3 + 1] = i1;
					pi_tri_list_out[i_dst_tri_index * 3 + 2] = i3;
					++i_dst_tri_index;	// next
					{
						unsigned char* pVerts_B = p_tri_infos[i_dst_tri_index].vert_num;
						pVerts_B[0] = 1; pVerts_B[1] = 2; pVerts_B[2] = 3;
					}
					pi_tri_list_out[i_dst_tri_index * 3] = i1;
					pi_tri_list_out[i_dst_tri_index * 3 + 1] = i2;
					pi_tri_list_out[i_dst_tri_index * 3 + 2] = i3;
					++i_dst_tri_index;	// next
				}
			}
		}

		i_tspaces_offset += verts;
		assert(i_dst_tri_index <= i_nr_triangles_in);
	}

	for (t = 0; t < i_nr_triangles_in; t++)
		p_tri_infos[t].i_flag = 0;

	// return total amount of tspaces
	return i_tspaces_offset;
}

static vec3 get_position(const o_tspace_context * context, const int index)
{
	int iF, iI;
	vec3 res; float pos[3];
	index_to_data(&iF, &iI, index);
	context->tspace_interface->get_position(context, pos, iF, iI);
	res.x = pos[0]; res.y = pos[1]; res.z = pos[2];
	return res;
}

static vec3 get_normal(const o_tspace_context * context, const int index)
{
	int iF, iI;
	vec3 res; float norm[3];
	index_to_data(&iF, &iI, index);
	context->tspace_interface->get_normal(context, norm, iF, iI);
	res.x = norm[0]; res.y = norm[1]; res.z = norm[2];
	return res;
}

static vec3 get_tex_coord(const o_tspace_context * context, const int index)
{
	int iF, iI;
	vec3 res; float texc[2];
	index_to_data(&iF, &iI, index);
	context->tspace_interface->get_tex_coord(context, texc, iF, iI);
	res.x = texc[0]; res.y = texc[1]; res.z = 1.0f;
	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union {
	struct
	{
		int i0, i1, f;
	};
	int array[3];
} o_edge;

static void build_neighbors_fast(o_tri_info p_tri_infos[], o_edge * p_edges, const int pi_tri_list_in[], const int i_nr_triangles_in);
static void build_neighbors_slow(o_tri_info p_tri_infos[], const int pi_tri_list_in[], const int i_nr_triangles_in);

// returns the texture area times 2
static float CalcTexArea(const o_tspace_context * context, const int indices[])
{
	const vec3 t1 = get_tex_coord(context, indices[0]);
	const vec3 t2 = get_tex_coord(context, indices[1]);
	const vec3 t3 = get_tex_coord(context, indices[2]);

	const float t21x = t2.x - t1.x;
	const float t21y = t2.y - t1.y;
	const float t31x = t3.x - t1.x;
	const float t31y = t3.y - t1.y;

	const float f_signed_areas_o_x_2 = t21x * t31y - t21y * t31x;

	return f_signed_areas_o_x_2 < 0 ? (-f_signed_areas_o_x_2) : f_signed_areas_o_x_2;
}

static void init_tri_info(o_tri_info p_tri_infos[], const int pi_tri_list_in[], const o_tspace_context * context, const int i_nr_triangles_in)
{
	int f = 0, i = 0, t = 0;
	// p_tri_infos[f].i_flag is cleared in generate_initial_vertices_index_list() which is called before this function.

	// generate neighbor info list
	for (f = 0; f < i_nr_triangles_in; f++)
		for (i = 0; i < 3; i++)
		{
			p_tri_infos[f].face_neighbors[i] = -1;
			p_tri_infos[f].assigned_group[i] = NULL;

			p_tri_infos[f].vOs.x = 0.0f; p_tri_infos[f].vOs.y = 0.0f; p_tri_infos[f].vOs.z = 0.0f;
			p_tri_infos[f].vOt.x = 0.0f; p_tri_infos[f].vOt.y = 0.0f; p_tri_infos[f].vOt.z = 0.0f;
			p_tri_infos[f].f_mag_s = 0;
			p_tri_infos[f].f_mag_t = 0;

			// assumed bad
			p_tri_infos[f].i_flag |= GROUP_WITH_ANY;
		}

	// evaluate first order derivatives
	for (f = 0; f < i_nr_triangles_in; f++)
	{
		// initial values
		const vec3 v1 = get_position(context, pi_tri_list_in[f * 3]);
		const vec3 v2 = get_position(context, pi_tri_list_in[f * 3 + 1]);
		const vec3 v3 = get_position(context, pi_tri_list_in[f * 3 + 2]);
		const vec3 t1 = get_tex_coord(context, pi_tri_list_in[f * 3]);
		const vec3 t2 = get_tex_coord(context, pi_tri_list_in[f * 3 + 1]);
		const vec3 t3 = get_tex_coord(context, pi_tri_list_in[f * 3 + 2]);

		const float t21x = t2.x - t1.x;
		const float t21y = t2.y - t1.y;
		const float t31x = t3.x - t1.x;
		const float t31y = t3.y - t1.y;
		const vec3 d1 = v_sub(v2, v1);
		const vec3 d2 = v_sub(v3, v1);

		const float f_signed_areas_o_x_2 = t21x * t31y - t21y * t31x;
		//assert(f_signed_areas_o_x_2!=0);
		vec3 vOs = v_sub(v_scale(t31y, d1), v_scale(t21y, d2));	// eq 18
		vec3 vOt = v_add(v_scale(-t31x, d1), v_scale(t21x, d2)); // eq 19

		p_tri_infos[f].i_flag |= (f_signed_areas_o_x_2 > 0 ? ORIENT_PRESERVING : 0);

		if (not_zero(f_signed_areas_o_x_2))
		{
			const float f_abs_area = fabsf(f_signed_areas_o_x_2);
			const float f_len_os = length(vOs);
			const float f_len_ot = length(vOt);
			const float fS = (p_tri_infos[f].i_flag & ORIENT_PRESERVING) == 0 ? (-1.0f) : 1.0f;
			if (not_zero(f_len_os)) p_tri_infos[f].vOs = v_scale(fS / f_len_os, vOs);
			if (not_zero(f_len_ot)) p_tri_infos[f].vOt = v_scale(fS / f_len_ot, vOt);

			// evaluate magnitudes prior to normalization of vOs and vOt
			p_tri_infos[f].f_mag_s = f_len_os / f_abs_area;
			p_tri_infos[f].f_mag_t = f_len_ot / f_abs_area;

			// if this is a good triangle
			if (not_zero(p_tri_infos[f].f_mag_s) && not_zero(p_tri_infos[f].f_mag_t))
				p_tri_infos[f].i_flag &= (~GROUP_WITH_ANY);
		}
	}

	// force otherwise healthy quads to a fixed orientation
	while (t < (i_nr_triangles_in - 1))
	{
		const int i_fo_a = p_tri_infos[t].i_org_face_number;
		const int iFO_b = p_tri_infos[t + 1].i_org_face_number;
		if (i_fo_a == iFO_b)	// this is a quad
		{
			const u8 b_is_deg_a = (p_tri_infos[t].i_flag & MARK_DEGENERATE) != 0 ? true : false;
			const u8 b_is_deg_b = (p_tri_infos[t + 1].i_flag & MARK_DEGENERATE) != 0 ? true : false;

			// bad triangles should already have been removed by
			// degen_prologue(), but just in case check b_is_deg_a and b_is_deg_a are false
			if ((b_is_deg_a || b_is_deg_b) == false)
			{
				const u8 b_orient_a = (p_tri_infos[t].i_flag & ORIENT_PRESERVING) != 0 ? true : false;
				const u8 b_orient_b = (p_tri_infos[t + 1].i_flag & ORIENT_PRESERVING) != 0 ? true : false;
				// if this happens the quad has extremely bad mapping!!
				if (b_orient_a != b_orient_b)
				{
					//printf("found quad with bad mapping\n");
					u8 b_choose_orient_first_tri = false;
					if ((p_tri_infos[t + 1].i_flag & GROUP_WITH_ANY) != 0) b_choose_orient_first_tri = true;
					else if (CalcTexArea(context, &pi_tri_list_in[t * 3 + 0]) >= CalcTexArea(context, &pi_tri_list_in[(t + 1) * 3]))
						b_choose_orient_first_tri = true;

					// force match
					{
						const int t0 = b_choose_orient_first_tri ? t : (t + 1);
						const int t1 = b_choose_orient_first_tri ? (t + 1) : t;
						p_tri_infos[t1].i_flag &= (~ORIENT_PRESERVING);	// clear first
						p_tri_infos[t1].i_flag |= (p_tri_infos[t0].i_flag & ORIENT_PRESERVING);	// copy bit
					}
				}
			}
			t += 2;
		}
		else
			++t;
	}

	// match up edge pairs
	{
		o_edge* p_edges = (o_edge*)malloc(sizeof(o_edge) * i_nr_triangles_in * 3);
		if (p_edges == NULL)
			build_neighbors_slow(p_tri_infos, pi_tri_list_in, i_nr_triangles_in);
		else
		{
			build_neighbors_fast(p_tri_infos, p_edges, pi_tri_list_in, i_nr_triangles_in);

			free(p_edges);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static u8 assign_recur(const int pi_tri_list_in[], o_tri_info ps_tri_infos[], const int i_my_tri_index, o_group * group);
static void add_tri_to_group(o_group * group, const int i_tri_index);

static int build_4_rule_groups(o_tri_info p_tri_infos[], o_group p_groups[], int pi_group_triangles_buffer[], const int pi_tri_list_in[], const int i_nr_triangles_in)
{
	const int i_nr_max_groups = i_nr_triangles_in * 3;
	int i_nr_active_groups = 0;
	int i_offset = 0, f = 0, i = 0;
	(void)i_nr_max_groups;  /* quiet warnings in non debug mode */
	for (f = 0; f < i_nr_triangles_in; f++)
	{
		for (i = 0; i < 3; i++)
		{
			// if not assigned to a group
			if ((p_tri_infos[f].i_flag & GROUP_WITH_ANY) == 0 && p_tri_infos[f].assigned_group[i] == NULL)
			{
				u8 bOrPre;
				int neigh_index_l, neigh_index_r;
				const int vert_index = pi_tri_list_in[f * 3 + i];
				assert(i_nr_active_groups < i_nr_max_groups);
				p_tri_infos[f].assigned_group[i] = &p_groups[i_nr_active_groups];
				p_tri_infos[f].assigned_group[i]->i_vertex_representitive = vert_index;
				p_tri_infos[f].assigned_group[i]->b_orient_preserving = (p_tri_infos[f].i_flag & ORIENT_PRESERVING) != 0;
				p_tri_infos[f].assigned_group[i]->i_nr_faces = 0;
				p_tri_infos[f].assigned_group[i]->p_face_indices = &pi_group_triangles_buffer[i_offset];
				++i_nr_active_groups;

				add_tri_to_group(p_tri_infos[f].assigned_group[i], f);
				bOrPre = (p_tri_infos[f].i_flag & ORIENT_PRESERVING) != 0 ? true : false;
				neigh_index_l = p_tri_infos[f].face_neighbors[i];
				neigh_index_r = p_tri_infos[f].face_neighbors[i > 0 ? (i - 1) : 2];
				if (neigh_index_l >= 0) // neighbor
				{
					const u8 bAnswer =
						assign_recur(pi_tri_list_in, p_tri_infos, neigh_index_l,
							p_tri_infos[f].assigned_group[i]);

					const u8 b_or_pre_2 = (p_tri_infos[neigh_index_l].i_flag & ORIENT_PRESERVING) != 0 ? true : false;
					const u8 bDiff = bOrPre != b_or_pre_2 ? true : false;
					assert(bAnswer || bDiff);
					(void)bAnswer, (void)bDiff;  /* quiet warnings in non debug mode */
				}
				if (neigh_index_r >= 0) // neighbor
				{
					const u8 bAnswer =
						assign_recur(pi_tri_list_in, p_tri_infos, neigh_index_r,
							p_tri_infos[f].assigned_group[i]);

					const u8 b_or_pre_2 = (p_tri_infos[neigh_index_r].i_flag & ORIENT_PRESERVING) != 0 ? true : false;
					const u8 bDiff = bOrPre != b_or_pre_2 ? true : false;
					assert(bAnswer || bDiff);
					(void)bAnswer, (void)bDiff;  /* quiet warnings in non debug mode */
				}

				// update offset
				i_offset += p_tri_infos[f].assigned_group[i]->i_nr_faces;
				// since the groups are disjoint a triangle can never
				// belong to more than 3 groups. Subsequently something
				// is completely screwed if this assertion ever hits.
				assert(i_offset <= i_nr_max_groups);
			}
		}
	}

	return i_nr_active_groups;
}

static void add_tri_to_group(o_group * group, const int i_tri_index)
{
	group->p_face_indices[group->i_nr_faces] = i_tri_index;
	++group->i_nr_faces;
}

static u8 assign_recur(const int pi_tri_list_in[], o_tri_info ps_tri_infos[],
	const int i_my_tri_index, o_group * group)
{
	o_tri_info* p_my_tri_info = &ps_tri_infos[i_my_tri_index];

	// track down vertex
	const int i_vert_rep = group->i_vertex_representitive;
	const int* p_verts = &pi_tri_list_in[3 * i_my_tri_index];
	int i = -1;
	if (p_verts[0] == i_vert_rep) i = 0;
	else if (p_verts[1] == i_vert_rep) i = 1;
	else if (p_verts[2] == i_vert_rep) i = 2;
	assert(i >= 0 && i < 3);

	// early out
	if (p_my_tri_info->assigned_group[i] == group) return true;
	else if (p_my_tri_info->assigned_group[i] != NULL) return false;
	if ((p_my_tri_info->i_flag & GROUP_WITH_ANY) != 0)
	{
		// first to group with a group-with-anything triangle
		// determines it's orientation.
		// This is the only existing order dependency in the code!!
		if (p_my_tri_info->assigned_group[0] == NULL &&
			p_my_tri_info->assigned_group[1] == NULL &&
			p_my_tri_info->assigned_group[2] == NULL)
		{
			p_my_tri_info->i_flag &= (~ORIENT_PRESERVING);
			p_my_tri_info->i_flag |= (group->b_orient_preserving ? ORIENT_PRESERVING : 0);
		}
	}
	{
		const u8 b_orient = (p_my_tri_info->i_flag & ORIENT_PRESERVING) != 0 ? true : false;
		if (b_orient != group->b_orient_preserving) return false;
	}

	add_tri_to_group(group, i_my_tri_index);
	p_my_tri_info->assigned_group[i] = group;

	{
		const int neigh_index_l = p_my_tri_info->face_neighbors[i];
		const int neigh_index_r = p_my_tri_info->face_neighbors[i > 0 ? (i - 1) : 2];
		if (neigh_index_l >= 0)
			assign_recur(pi_tri_list_in, ps_tri_infos, neigh_index_l, group);
		if (neigh_index_r >= 0)
			assign_recur(pi_tri_list_in, ps_tri_infos, neigh_index_r, group);
	}



	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static u8 compare_sub_groups(const o_sub_group * pg1, const o_sub_group * pg2);
static void quick_sort(int* p_sort_buffer, int i_left, int i_right, unsigned int u_seed);
static o_tspace eval_tspace(int face_indices[], const int iFaces, const int pi_tri_list_in[], const o_tri_info p_tri_infos[], const o_tspace_context * context, const int i_vertex_representitive);

static u8 generate_tspaces(o_tspace ps_tspace[], const o_tri_info p_tri_infos[], const o_group p_groups[],
	const int i_nr_active_groups, const int pi_tri_list_in[], const float f_thres_cos,
	const o_tspace_context * context)
{
	o_tspace* p_sub_group_tspace = NULL;
	o_sub_group* p_uni_sub_groups = NULL;
	int* p_tmp_members = NULL;
	int i_max_nr_faces = 0, i_unique_tspaces = 0, g = 0, i = 0;
	for (g = 0; g < i_nr_active_groups; g++)
		if (i_max_nr_faces < p_groups[g].i_nr_faces)
			i_max_nr_faces = p_groups[g].i_nr_faces;

	if (i_max_nr_faces == 0) return true;

	// make initial allocations
	p_sub_group_tspace = (o_tspace*)malloc(sizeof(o_tspace) * i_max_nr_faces);
	p_uni_sub_groups = (o_sub_group*)malloc(sizeof(o_sub_group) * i_max_nr_faces);
	p_tmp_members = (int*)malloc(sizeof(int) * i_max_nr_faces);
	if (p_sub_group_tspace == NULL || p_uni_sub_groups == NULL || p_tmp_members == NULL)
	{
		if (p_sub_group_tspace != NULL) free(p_sub_group_tspace);
		if (p_uni_sub_groups != NULL) free(p_uni_sub_groups);
		if (p_tmp_members != NULL) free(p_tmp_members);
		return false;
	}


	i_unique_tspaces = 0;
	for (g = 0; g < i_nr_active_groups; g++)
	{
		const o_group* group = &p_groups[g];
		int i_unique_sub_groups = 0, s = 0;

		for (i = 0; i < group->i_nr_faces; i++)	// triangles
		{
			const int f = group->p_face_indices[i];	// triangle number
			int index = -1, i_vert_index = -1, iOF_1 = -1, i_members = 0, j = 0, l = 0;
			o_sub_group tmp_group;
			u8 b_found;
			vec3 n, vOs, vOt;
			if (p_tri_infos[f].assigned_group[0] == group) index = 0;
			else if (p_tri_infos[f].assigned_group[1] == group) index = 1;
			else if (p_tri_infos[f].assigned_group[2] == group) index = 2;
			assert(index >= 0 && index < 3);

			i_vert_index = pi_tri_list_in[f * 3 + index];
			assert(i_vert_index == group->i_vertex_representitive);

			// is normalized already
			n = get_normal(context, i_vert_index);

			// project
			vOs = v_sub(p_tri_infos[f].vOs, v_scale(v_dot(n, p_tri_infos[f].vOs), n));
			vOt = v_sub(p_tri_infos[f].vOt, v_scale(v_dot(n, p_tri_infos[f].vOt), n));
			if (v_not_zero(vOs)) vOs = normalize(vOs);
			if (v_not_zero(vOt)) vOt = normalize(vOt);

			// original face number
			iOF_1 = p_tri_infos[f].i_org_face_number;

			i_members = 0;
			for (j = 0; j < group->i_nr_faces; j++)
			{
				const int t = group->p_face_indices[j];	// triangle number
				const int iOF_2 = p_tri_infos[t].i_org_face_number;

				// project
				vec3 vOs2 = v_sub(p_tri_infos[t].vOs, v_scale(v_dot(n, p_tri_infos[t].vOs), n));
				vec3 vOt2 = v_sub(p_tri_infos[t].vOt, v_scale(v_dot(n, p_tri_infos[t].vOt), n));
				if (v_not_zero(vOs2)) vOs2 = normalize(vOs2);
				if (v_not_zero(vOt2)) vOt2 = normalize(vOt2);

				{
					const u8 b_any = ((p_tri_infos[f].i_flag | p_tri_infos[t].i_flag) & GROUP_WITH_ANY) != 0 ? true : false;
					// make sure triangles which belong to the same quad are joined.
					const u8 b_same_org_face = iOF_1 == iOF_2 ? true : false;

					const float f_cos_s = v_dot(vOs, vOs2);
					const float f_cos_t = v_dot(vOt, vOt2);

					assert(f != t || b_same_org_face);	// sanity check
					if (b_any || b_same_org_face || (f_cos_s > f_thres_cos && f_cos_t > f_thres_cos))
						p_tmp_members[i_members++] = t;
				}
			}

			// sort p_tmp_members
			tmp_group.i_nr_faces = i_members;
			tmp_group.p_tri_members = p_tmp_members;
			if (i_members > 1)
			{
				unsigned int u_seed = INTERNAL_RND_SORT_SEED;	// could replace with a random seed?
				quick_sort(p_tmp_members, 0, i_members - 1, u_seed);
			}

			// look for an existing match
			b_found = false;
			l = 0;
			while (l < i_unique_sub_groups && !b_found)
			{
				b_found = compare_sub_groups(&tmp_group, &p_uni_sub_groups[l]);
				if (!b_found) ++l;
			}

			// assign tangent space index
			assert(b_found || l == i_unique_sub_groups);
			//piTempTangIndices[f*3+index] = i_unique_tspaces+l;

			// if no match was found we allocate a new subgroup
			if (!b_found)
			{
				// insert new subgroup
				int* p_indices = (int*)malloc(sizeof(int) * i_members);
				if (p_indices == NULL)
				{
					// clean up and return false
					int s = 0;
					for (s = 0; s < i_unique_sub_groups; s++)
						free(p_uni_sub_groups[s].p_tri_members);
					free(p_uni_sub_groups);
					free(p_tmp_members);
					free(p_sub_group_tspace);
					return false;
				}
				p_uni_sub_groups[i_unique_sub_groups].i_nr_faces = i_members;
				p_uni_sub_groups[i_unique_sub_groups].p_tri_members = p_indices;
				memcpy(p_indices, tmp_group.p_tri_members, i_members * sizeof(int));
				p_sub_group_tspace[i_unique_sub_groups] =
					eval_tspace(tmp_group.p_tri_members, i_members, pi_tri_list_in, p_tri_infos, context, group->i_vertex_representitive);
				++i_unique_sub_groups;
			}

			// output tspace
			{
				const int i_offsets = p_tri_infos[f].i_tspaces_offset;
				const int i_vert = p_tri_infos[f].vert_num[index];
				o_tspace* pTS_out = &ps_tspace[i_offsets + i_vert];
				assert(pTS_out->i_counter < 2);
				assert(((p_tri_infos[f].i_flag & ORIENT_PRESERVING) != 0) == group->b_orient_preserving);
				if (pTS_out->i_counter == 1)
				{
					*pTS_out = average_tspace(pTS_out, &p_sub_group_tspace[l]);
					pTS_out->i_counter = 2;	// update counter
					pTS_out->b_orient = group->b_orient_preserving;
				}
				else
				{
					assert(pTS_out->i_counter == 0);
					*pTS_out = p_sub_group_tspace[l];
					pTS_out->i_counter = 1;	// update counter
					pTS_out->b_orient = group->b_orient_preserving;
				}
			}
		}

		// clean up and offset i_unique_tspaces
		for (s = 0; s < i_unique_sub_groups; s++)
			free(p_uni_sub_groups[s].p_tri_members);
		i_unique_tspaces += i_unique_sub_groups;
	}

	// clean up
	free(p_uni_sub_groups);
	free(p_tmp_members);
	free(p_sub_group_tspace);

	return true;
}

static o_tspace eval_tspace(int face_indices[], const int iFaces, const int pi_tri_list_in[], const o_tri_info p_tri_infos[],
	const o_tspace_context * context, const int i_vertex_representitive)
{
	o_tspace res;
	float f_angle_sum = 0;
	int face = 0;
	res.vOs.x = 0.0f; res.vOs.y = 0.0f; res.vOs.z = 0.0f;
	res.vOt.x = 0.0f; res.vOt.y = 0.0f; res.vOt.z = 0.0f;
	res.f_mag_s = 0; res.f_mag_t = 0;

	for (face = 0; face < iFaces; face++)
	{
		const int f = face_indices[face];

		// only valid triangles get to add their contribution
		if ((p_tri_infos[f].i_flag & GROUP_WITH_ANY) == 0)
		{
			vec3 n, vOs, vOt, p0, p1, p2, v1, v2;
			float f_cos, f_angle, f_mag_s, f_mag_t;
			int i = -1, index = -1, i0 = -1, i1 = -1, i2 = -1;
			if (pi_tri_list_in[3 * f] == i_vertex_representitive) i = 0;
			else if (pi_tri_list_in[3 * f + 1] == i_vertex_representitive) i = 1;
			else if (pi_tri_list_in[3 * f + 2] == i_vertex_representitive) i = 2;
			assert(i >= 0 && i < 3);

			// project
			index = pi_tri_list_in[3 * f + i];
			n = get_normal(context, index);
			vOs = v_sub(p_tri_infos[f].vOs, v_scale(v_dot(n, p_tri_infos[f].vOs), n));
			vOt = v_sub(p_tri_infos[f].vOt, v_scale(v_dot(n, p_tri_infos[f].vOt), n));
			if (v_not_zero(vOs)) vOs = normalize(vOs);
			if (v_not_zero(vOt)) vOt = normalize(vOt);

			i2 = pi_tri_list_in[3 * f + (i < 2 ? (i + 1) : 0)];
			i1 = pi_tri_list_in[3 * f + i];
			i0 = pi_tri_list_in[3 * f + (i > 0 ? (i - 1) : 2)];

			p0 = get_position(context, i0);
			p1 = get_position(context, i1);
			p2 = get_position(context, i2);
			v1 = v_sub(p0, p1);
			v2 = v_sub(p2, p1);

			// project
			v1 = v_sub(v1, v_scale(v_dot(n, v1), n)); if (v_not_zero(v1)) v1 = normalize(v1);
			v2 = v_sub(v2, v_scale(v_dot(n, v2), n)); if (v_not_zero(v2)) v2 = normalize(v2);

			// weight contribution by the angle
			// between the two edge vectors
			f_cos = v_dot(v1, v2); f_cos = f_cos > 1 ? 1 : (f_cos < (-1) ? (-1) : f_cos);
			f_angle = (float)acos(f_cos);
			f_mag_s = p_tri_infos[f].f_mag_s;
			f_mag_t = p_tri_infos[f].f_mag_t;

			res.vOs = v_add(res.vOs, v_scale(f_angle, vOs));
			res.vOt = v_add(res.vOt, v_scale(f_angle, vOt));
			res.f_mag_s += (f_angle * f_mag_s);
			res.f_mag_t += (f_angle * f_mag_t);
			f_angle_sum += f_angle;
		}
	}

	// normalize
	if (v_not_zero(res.vOs)) res.vOs = normalize(res.vOs);
	if (v_not_zero(res.vOt)) res.vOt = normalize(res.vOt);
	if (f_angle_sum > 0)
	{
		res.f_mag_s /= f_angle_sum;
		res.f_mag_t /= f_angle_sum;
	}

	return res;
}

static u8 compare_sub_groups(const o_sub_group * pg1, const o_sub_group * pg2)
{
	u8 b_still_same = true;
	int i = 0;
	if (pg1->i_nr_faces != pg2->i_nr_faces) return false;
	while (i < pg1->i_nr_faces && b_still_same)
	{
		b_still_same = pg1->p_tri_members[i] == pg2->p_tri_members[i] ? true : false;
		if (b_still_same) ++i;
	}
	return b_still_same;
}

static void quick_sort(int* p_sort_buffer, int i_left, int i_right, unsigned int u_seed)
{
	int iL, iR, n, index, i_mid, i_tmp;

	// Random
	unsigned int t = u_seed & 31;
	t = (u_seed << t) | (u_seed >> (32 - t));
	u_seed = u_seed + t + 3;
	// Random end

	iL = i_left; iR = i_right;
	n = (iR - iL) + 1;
	assert(n >= 0);
	index = (int)(u_seed % n);

	i_mid = p_sort_buffer[index + iL];

	do
	{
		while (p_sort_buffer[iL] < i_mid)
			++iL;
		while (p_sort_buffer[iR] > i_mid)
			--iR;

		if (iL <= iR)
		{
			i_tmp = p_sort_buffer[iL];
			p_sort_buffer[iL] = p_sort_buffer[iR];
			p_sort_buffer[iR] = i_tmp;
			++iL; --iR;
		}
	} while (iL <= iR);

	if (i_left < iR)
		quick_sort(p_sort_buffer, i_left, iR, u_seed);
	if (iL < i_right)
		quick_sort(p_sort_buffer, iL, i_right, u_seed);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

static void quick_sort_edges(o_edge* p_sort_buffer, int i_left, int i_right, const int channel, unsigned int u_seed);
static void get_edge(int* i0_out, int* i1_out, int* edge_nu_out, const int indices[], const int i0_in, const int i1_in);

static void build_neighbors_fast(o_tri_info p_tri_infos[], o_edge* p_edges, const int pi_tri_list_in[], const int i_nr_triangles_in)
{
	// build array of edges
	unsigned int u_seed = INTERNAL_RND_SORT_SEED;				// could replace with a random seed?
	int entries = 0, i_cur_start_index = -1, f = 0, i = 0;
	for (f = 0; f < i_nr_triangles_in; f++)
		for (i = 0; i < 3; i++)
		{
			const int i0 = pi_tri_list_in[f * 3 + i];
			const int i1 = pi_tri_list_in[f * 3 + (i < 2 ? (i + 1) : 0)];
			p_edges[f * 3 + i].i0 = i0 < i1 ? i0 : i1;			// put minimum index in i0
			p_edges[f * 3 + i].i1 = !(i0 < i1) ? i0 : i1;		// put maximum index in i1
			p_edges[f * 3 + i].f = f;							// record face number
		}

	// sort over all edges by i0, this is the pricy one.
	quick_sort_edges(p_edges, 0, i_nr_triangles_in * 3 - 1, 0, u_seed);	// sort channel 0 which is i0

	// sub sort over i1, should be fast.
	// could replace this with a 64 bit int sort over (i0,i1)
	// with i0 as msb in the quick_sort call above.
	entries = i_nr_triangles_in * 3;
	i_cur_start_index = 0;
	for (i = 1; i < entries; i++)
	{
		if (p_edges[i_cur_start_index].i0 != p_edges[i].i0)
		{
			const int iL = i_cur_start_index;
			const int iR = i - 1;
			//const int i_elements = i-iL;
			i_cur_start_index = i;
			quick_sort_edges(p_edges, iL, iR, 1, u_seed);	// sort channel 1 which is i1
		}
	}

	// sub sort over f, which should be fast.
	// this step is to remain compliant with build_neighbors_slow() when
	// more than 2 triangles use the same edge (such as a butterfly topology).
	i_cur_start_index = 0;
	for (i = 1; i < entries; i++)
	{
		if (p_edges[i_cur_start_index].i0 != p_edges[i].i0 || p_edges[i_cur_start_index].i1 != p_edges[i].i1)
		{
			const int iL = i_cur_start_index;
			const int iR = i - 1;
			//const int i_elements = i-iL;
			i_cur_start_index = i;
			quick_sort_edges(p_edges, iL, iR, 2, u_seed);	// sort channel 2 which is f
		}
	}

	// pair up, adjacent triangles
	for (i = 0; i < entries; i++)
	{
		const int i0 = p_edges[i].i0;
		const int i1 = p_edges[i].i1;
		const int f = p_edges[i].f;
		u8 b_unassigned_a;

		int i0_A, i1_A;
		int edge_nu_a, edge_nu_b = 0;	// 0,1 or 2
		get_edge(&i0_A, &i1_A, &edge_nu_a, &pi_tri_list_in[f * 3], i0, i1);	// resolve index ordering and edge_num
		b_unassigned_a = p_tri_infos[f].face_neighbors[edge_nu_a] == -1 ? true : false;

		if (b_unassigned_a)
		{
			// get true index ordering
			int j = i + 1, t;
			u8 b_not_found = true;
			while (j < entries && i0 == p_edges[j].i0 && i1 == p_edges[j].i1 && b_not_found)
			{
				u8 b_unassigned_b;
				int i0_B, i1_B;
				t = p_edges[j].f;
				// flip i0_B and i1_B
				get_edge(&i1_B, &i0_B, &edge_nu_b, &pi_tri_list_in[t * 3], p_edges[j].i0, p_edges[j].i1);	// resolve index ordering and edge_num
				//assert(!(i0_A==i1_B && i1_A==i0_B));
				b_unassigned_b = p_tri_infos[t].face_neighbors[edge_nu_b] == -1 ? true : false;
				if (i0_A == i0_B && i1_A == i1_B && b_unassigned_b)
					b_not_found = false;
				else
					++j;
			}

			if (!b_not_found)
			{
				int t = p_edges[j].f;
				p_tri_infos[f].face_neighbors[edge_nu_a] = t;
				//assert(p_tri_infos[t].face_neighbors[edge_nu_b]==-1);
				p_tri_infos[t].face_neighbors[edge_nu_b] = f;
			}
		}
	}
}

static void build_neighbors_slow(o_tri_info p_tri_infos[], const int pi_tri_list_in[], const int i_nr_triangles_in)
{
	int f = 0, i = 0;
	for (f = 0; f < i_nr_triangles_in; f++)
	{
		for (i = 0; i < 3; i++)
		{
			// if unassigned
			if (p_tri_infos[f].face_neighbors[i] == -1)
			{
				const int i0_A = pi_tri_list_in[f * 3 + i];
				const int i1_A = pi_tri_list_in[f * 3 + (i < 2 ? (i + 1) : 0)];

				// search for a neighbor
				u8 b_found = false;
				int t = 0, j = 0;
				while (!b_found && t < i_nr_triangles_in)
				{
					if (t != f)
					{
						j = 0;
						while (!b_found && j < 3)
						{
							// in rev order
							const int i1_B = pi_tri_list_in[t * 3 + j];
							const int i0_B = pi_tri_list_in[t * 3 + (j < 2 ? (j + 1) : 0)];
							//assert(!(i0_A==i1_B && i1_A==i0_B));
							if (i0_A == i0_B && i1_A == i1_B)
								b_found = true;
							else
								++j;
						}
					}

					if (!b_found) ++t;
				}

				// assign neighbors
				if (b_found)
				{
					p_tri_infos[f].face_neighbors[i] = t;
					//assert(p_tri_infos[t].face_neighbors[j]==-1);
					p_tri_infos[t].face_neighbors[j] = f;
				}
			}
		}
	}
}

static void quick_sort_edges(o_edge * p_sort_buffer, int i_left, int i_right, const int channel, unsigned int u_seed)
{
	unsigned int t;
	int iL, iR, n, index, i_mid;

	// early out
	o_edge sTmp;
	const int i_elements = i_right - i_left + 1;
	if (i_elements < 2) return;
	else if (i_elements == 2)
	{
		if (p_sort_buffer[i_left].array[channel] > p_sort_buffer[i_right].array[channel])
		{
			sTmp = p_sort_buffer[i_left];
			p_sort_buffer[i_left] = p_sort_buffer[i_right];
			p_sort_buffer[i_right] = sTmp;
		}
		return;
	}

	// Random
	t = u_seed & 31;
	t = (u_seed << t) | (u_seed >> (32 - t));
	u_seed = u_seed + t + 3;
	// Random end

	iL = i_left;
	iR = i_right;
	n = (iR - iL) + 1;
	assert(n >= 0);
	index = (int)(u_seed % n);

	i_mid = p_sort_buffer[index + iL].array[channel];

	do
	{
		while (p_sort_buffer[iL].array[channel] < i_mid)
			++iL;
		while (p_sort_buffer[iR].array[channel] > i_mid)
			--iR;

		if (iL <= iR)
		{
			sTmp = p_sort_buffer[iL];
			p_sort_buffer[iL] = p_sort_buffer[iR];
			p_sort_buffer[iR] = sTmp;
			++iL; --iR;
		}
	} while (iL <= iR);

	if (i_left < iR)
		quick_sort_edges(p_sort_buffer, i_left, iR, channel, u_seed);
	if (iL < i_right)
		quick_sort_edges(p_sort_buffer, iL, i_right, channel, u_seed);
}

// resolve ordering and edge number
static void get_edge(int* i0_out, int* i1_out, int* edge_nu_out, const int indices[], const int i0_in, const int i1_in)
{
	*edge_nu_out = -1;

	// test if first index is on the edge
	if (indices[0] == i0_in || indices[0] == i1_in)
	{
		// test if second index is on the edge
		if (indices[1] == i0_in || indices[1] == i1_in)
		{
			edge_nu_out[0] = 0;	// first edge
			i0_out[0] = indices[0];
			i1_out[0] = indices[1];
		}
		else
		{
			edge_nu_out[0] = 2;	// third edge
			i0_out[0] = indices[2];
			i1_out[0] = indices[0];
		}
	}
	else
	{
		// only second and third index is on the edge
		edge_nu_out[0] = 1;	// second edge
		i0_out[0] = indices[1];
		i1_out[0] = indices[2];
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Degenerate triangles ////////////////////////////////////

static void degen_prologue(o_tri_info p_tri_infos[], int pi_tri_list_out[], const int i_nr_triangles_in, const int i_tot_tris)
{
	int i_next_good_triangle_search_index = -1;
	u8 b_still_finding_good_ones;

	// locate quads with only one good triangle
	int t = 0;
	while (t < (i_tot_tris - 1))
	{
		const int i_fo_a = p_tri_infos[t].i_org_face_number;
		const int iFO_b = p_tri_infos[t + 1].i_org_face_number;
		if (i_fo_a == iFO_b)	// this is a quad
		{
			const u8 b_is_deg_a = (p_tri_infos[t].i_flag & MARK_DEGENERATE) != 0 ? true : false;
			const u8 b_is_deg_b = (p_tri_infos[t + 1].i_flag & MARK_DEGENERATE) != 0 ? true : false;
			if ((b_is_deg_a ^ b_is_deg_b) != 0)
			{
				p_tri_infos[t].i_flag |= QUAD_ONE_DEGEN_TRI;
				p_tri_infos[t + 1].i_flag |= QUAD_ONE_DEGEN_TRI;
			}
			t += 2;
		}
		else
			++t;
	}

	// reorder list so all degen triangles are moved to the back
	// without reordering the good triangles
	i_next_good_triangle_search_index = 1;
	t = 0;
	b_still_finding_good_ones = true;
	while (t < i_nr_triangles_in && b_still_finding_good_ones)
	{
		const u8 b_is_good = (p_tri_infos[t].i_flag & MARK_DEGENERATE) == 0 ? true : false;
		if (b_is_good)
		{
			if (i_next_good_triangle_search_index < (t + 2))
				i_next_good_triangle_search_index = t + 2;
		}
		else
		{
			int t0, t1;
			// search for the first good triangle.
			u8 b_just_a_degenerate = true;
			while (b_just_a_degenerate && i_next_good_triangle_search_index < i_tot_tris)
			{
				const u8 b_is_good = (p_tri_infos[i_next_good_triangle_search_index].i_flag & MARK_DEGENERATE) == 0 ? true : false;
				if (b_is_good) b_just_a_degenerate = false;
				else ++i_next_good_triangle_search_index;
			}

			t0 = t;
			t1 = i_next_good_triangle_search_index;
			++i_next_good_triangle_search_index;
			assert(i_next_good_triangle_search_index > (t + 1));

			// swap triangle t0 and t1
			if (!b_just_a_degenerate)
			{
				int i = 0;
				for (i = 0; i < 3; i++)
				{
					const int index = pi_tri_list_out[t0 * 3 + i];
					pi_tri_list_out[t0 * 3 + i] = pi_tri_list_out[t1 * 3 + i];
					pi_tri_list_out[t1 * 3 + i] = index;
				}
				{
					const o_tri_info tri_info = p_tri_infos[t0];
					p_tri_infos[t0] = p_tri_infos[t1];
					p_tri_infos[t1] = tri_info;
				}
			}
			else
				b_still_finding_good_ones = false;	// this is not supposed to happen
		}

		if (b_still_finding_good_ones) ++t;
	}

	assert(b_still_finding_good_ones);	// code will still work.
	assert(i_nr_triangles_in == t);
}

static void degen_epilogue(o_tspace ps_tspace[], o_tri_info p_tri_infos[], int pi_tri_list_in[], const o_tspace_context * context, const int i_nr_triangles_in, const int i_tot_tris)
{
	int t = 0, i = 0;
	// deal with degenerate triangles
	// punishment for degenerate triangles is O(N^2)
	for (t = i_nr_triangles_in; t < i_tot_tris; t++)
	{
		// degenerate triangles on a quad with one good triangle are skipped
		// here but processed in the next loop
		const u8 b_skip = (p_tri_infos[t].i_flag & QUAD_ONE_DEGEN_TRI) != 0 ? true : false;

		if (!b_skip)
		{
			for (i = 0; i < 3; i++)
			{
				const int index_1 = pi_tri_list_in[t * 3 + i];
				// search through the good triangles
				u8 b_not_found = true;
				int j = 0;
				while (b_not_found && j < (3 * i_nr_triangles_in))
				{
					const int index_2 = pi_tri_list_in[j];
					if (index_1 == index_2) b_not_found = false;
					else ++j;
				}

				if (!b_not_found)
				{
					const int i_tri = j / 3;
					const int i_vert = j % 3;
					const int i_src_vert = p_tri_infos[i_tri].vert_num[i_vert];
					const int i_src_offset = p_tri_infos[i_tri].i_tspaces_offset;
					const int i_dst_vert = p_tri_infos[t].vert_num[i];
					const int i_dst_offset = p_tri_infos[t].i_tspaces_offset;

					// copy tspace
					ps_tspace[i_dst_offset + i_dst_vert] = ps_tspace[i_src_offset + i_src_vert];
				}
			}
		}
	}

	// deal with degenerate quads with one good triangle
	for (t = 0; t < i_nr_triangles_in; t++)
	{
		// this triangle belongs to a quad where the
		// other triangle is degenerate
		if ((p_tri_infos[t].i_flag & QUAD_ONE_DEGEN_TRI) != 0)
		{
			vec3 vDstP;
			int iOrgF = -1, i = 0;
			u8 b_not_found;
			unsigned char* pV = p_tri_infos[t].vert_num;
			int i_flag = (1 << pV[0]) | (1 << pV[1]) | (1 << pV[2]);
			int i_missing_index = 0;
			if ((i_flag & 2) == 0) i_missing_index = 1;
			else if ((i_flag & 4) == 0) i_missing_index = 2;
			else if ((i_flag & 8) == 0) i_missing_index = 3;

			iOrgF = p_tri_infos[t].i_org_face_number;
			vDstP = get_position(context, make_index(iOrgF, i_missing_index));
			b_not_found = true;
			i = 0;
			while (b_not_found && i < 3)
			{
				const int i_vert = pV[i];
				const vec3 vSrcP = get_position(context, make_index(iOrgF, i_vert));
				if (veq(vSrcP, vDstP) == true)
				{
					const int i_offsets = p_tri_infos[t].i_tspaces_offset;
					ps_tspace[i_offsets + i_missing_index] = ps_tspace[i_offsets + i_vert];
					b_not_found = false;
				}
				else
					++i;
			}
			assert(!b_not_found);
		}
	}
}