/*
 * expr.hh
 *
 *  Created on: Dec 26, 2019
 *      Author: jb
 */

#ifndef INCLUDE_ARRAY_HH_
#define INCLUDE_ARRAY_HH_

#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>
#include <boost/math/constants/constants.hpp>
#include "config.hh"
#include "scalar_node.hh"
#include "test_tools.hh"

namespace bparser {

/**
 * Shape of an array.
 */
typedef std::vector<uint> Shape;

inline uint shape_size(Shape s) {
	if (s.size() == 0) return 1;
	uint shape_prod = 1;
	for(uint dim : s) shape_prod *= dim;
	return shape_prod;
}


inline std::string print_shape(const Shape s) {
	std::ostringstream ss;
	ss << '(';
	for(int dim_size : s) {
		if (ss.tellp() > 1) ss << ", ";
		ss << dim_size;
	}
	ss << ')';
	return ss.str();
}


inline bool same_shape(Shape a, Shape b) {
	if (a.size() != b.size()) return false;
	for(uint i=0; i < a.size(); i++ )
		if (a[i] != b[i]) return false;
	return true;
}


const int none_int = std::numeric_limits<int>::max();
typedef std::array<int, 3> Slice;



/**
 * Check that 'i' is correct index to an axis of 'size'.
 * Negative 'i' is supported, converted to the positive index.
 * Return correct positive index less then 'size'.
 */
inline uint absolute_idx(int i, int size) {
	int i_out=i;
	if (i < 0) i_out = size + i;
	if (i_out < 0 || i_out >= size) Throw() << "Index " << i << " out of range (" << size << ").\n";
	return i_out;
}



/**
 * Defines extraction from an Array or broadcasting of smaller Array to the larger shape.
 *
 * Consider MultiIdxRange as a mapping from multiindices of a SOURCE array
 * to the multiindices of the DESTINATION array, i.e.
 *
 * 		destination[i, j, k, ... ] = source[MIR[ i, j, k, ...]]
 *
 * MIR mapping is given as a tensor product of single axis mappings:
 * MIR[i, j, k, ... ] =  (ranges_[T[0], i], ranges_[T[1], j], ranges_[T[2], k], ...)
 *
 * T[1:K] - transpose mapping, contains values (0, .., source.shape.size()) every axis at most once.
 * It can contain only subset of the source axes.
 * Can contain value 'none_int', marking broadcasting axis with the size 1.
 *
 * range_[m] where m is not contained in T[:] must have size 1. This provides a slice index to the source
 * array in the axis 'm'.
 * Shape of the source array is given by the full_shape_. The values of the ranges_ must be compatible:
 *
 *       ranges_[i][:] < full_shape_[i]
 *
 * Shape of the destination array is given by  [n_0, ... ,n_K], where
 * n_i is 1 if T[i] is none_int
 * or n_i is ranges_[T[i]].size() if T[i] is int
 *
 * There aretwo main use cases:
 * 1. Subscription of the array.
 * This is done iteratively by processing the subscription list and calling on of:
 * sub_index, sub_range, sub_slice for every item. This process starts with an empty MultiIdxRange(shape)
 * where 'shape' provides the shape of the source array. Then axes are iteratively added.
 * The 'sub_index' method adds axis only to the source shape and to ranges_, sub_transpose is untouched.
 * The other two methods add axis to both source and destination shape.
 *
 * 2. Broadcasting.
 * This case assumes reversed usage of the mapping in the sense:
 *
 * source[MIR[ijk]] = destination[ijk]
 *
 * The input array is 'destination' while the result of broadcasting is 'source' array.
 * In this case one starts with an identity MIR that can be created by:
 * MultiIdxRange(shape).full().
 * Then broadcasting can is done using 'insert_axis' method.
 *
 *
 */
struct MultiIdxRange {
	//typedef std::vector<std::vector<int>> GeneralRanges;
	/**
	 * AST interface ============================================
	 */
//	MultiIdxRange(Shape full_shape, GeneralRanges ranges)
//	: full_shape_(full_shape) {
//
//		for(uint axis=0; axis < full_shape_.size(); ++axis) {
//			if (axis < ranges.size()) {
//
//			} else {
//				std::vector<uint> full(full_shape_[axis]);
//				for(uint i=0; i < full_shape_[axis]; ++i) full[i] = i;
//			ranges_.push_back(full);
//		}
//	}
//


	// ===================================================



	/**
	 * Identical MIR mapping for given shape.
	 */
	MultiIdxRange(Shape shape)
	: full_shape_(shape),
	  ranges_(),
	  sub_transpose_()
	{}

	MultiIdxRange full() const {
		MultiIdxRange res(full_shape_);
		res.sub_transpose_.resize(full_shape_.size());
		for(uint axis=0; axis<full_shape_.size();axis++)
			res.sub_transpose_[axis] = axis;

		for(uint axis=0; axis < full_shape_.size(); ++axis) {
			std::vector<uint> full(full_shape_[axis]);
			for(uint i=0; i < full_shape_[axis]; ++i) full[i] = i;
			res.ranges_.push_back(full);
		}
		return res;
	}



//	MultiIdxRange(Shape shape)
//	: full_shape_(shape) {

//		void subset(const std::vector<IndexSubset> &range_spec) {
//			ASSERT(range_spec.size() == full_shape_.size());
//			for(IndexSubset subset : range_spec)
//				ranges_.push_back(subset.idx_list(full_shape_));
//		}


	/**
	 * Broadcast the full range to the given shape.
	 * 1. pad full shape from left by ones
	 * 2. extend dimensions equal to one to the given shape
	 * 3. other dimensions must match given shape
	 * TODO: make all methods either pure or inplace
	 */
	MultiIdxRange broadcast(Shape other) const {
		int n_pad = other.size() - full_shape_.size();
		if ( n_pad < 0) {
			Throw() << "Broadcast from longer shape " << full_shape_.size()
				<< " to shorter shape " << other.size() << ".\n";
		}
		Shape res_shape(n_pad, 1);
		res_shape.insert(res_shape.end(), full_shape_.begin(), full_shape_.end());

		auto result = MultiIdxRange(other).full();
		for(uint ax=0; ax < res_shape.size(); ++ax) {
			if (res_shape[ax] == 1)
				result.ranges_[ax] = std::vector<uint>(other[ax], 0);
			else if (res_shape[ax] != other[ax]) {
				Throw() << "Broadcast from " << res_shape[ax] << " to "
					<< other[ax] << " in axis " << ax;
			}

		}
		return result;
	}

	/**
	 * Compute destination shape of symmetric broadcasting of two shapes.
	 *
	 * Set new SHAPE:
	 * - SHAPE length is maximum of shape 'a' and shape 'b' lengths.
	 * - pad 'a' and 'b' from left by ones up to common size
	 * - set SHAPE[i] to a[i] if b[i]==1
	 * - set SHAPE[i] to b[i] if a[i]==1 or a[i] == b[i]
	 */
	static Shape broadcast_common_shape(Shape a, Shape b) {
		uint res_size = std::max(a.size(), b.size());
		Shape res(res_size);
		a.insert(a.begin(), res_size - a.size(), 1);
		b.insert(b.begin(), res_size - b.size(), 1);
		for(uint i=0; i<res_size; ++i) {
			if (a[i]==1) a[i] = b[i];
			if (b[i]==1) b[i] = a[i];
			if (a[i] == b[i]) {
				res[i] =  a[i];
			} else {
				Throw() << "Common broadcast between " << a[i] << " and "
				   << b[i] << " in axis " << i;
			}
		}
		return res;
	}

	/**
	 * Insert 'axis' with continuous sequence from 0 till 'dimension' into the source.
	 * Set
	 * E.g. for axis=1, dimension=2:
	 * range [[1,2],[2,5,6]] -> [[1,2],[0,1],[2,5,6]]
	 *
	 * TODO: return copy
	 */
	void insert_axis(uint dest_axis, uint src_axis, uint dimension=1) {
		BP_ASSERT(src_axis <= full_shape_.size());
		BP_ASSERT(dest_axis <= sub_transpose_.size()); // TODO: check size of sub_transpose_
		full_shape_.insert(full_shape_.begin() + src_axis, dimension);
		ranges_.insert(ranges_.begin() + src_axis, std::vector<uint>());
		for(uint i=0; i<dimension; ++i)
			ranges_[src_axis].push_back(i);
		for(uint axis=0; axis<sub_transpose_.size();axis++)
			if (sub_transpose_[axis] >= src_axis)
				sub_transpose_[axis]++;

		sub_transpose_.insert(sub_transpose_.begin() + dest_axis, src_axis);
	}

	void remove_destination_axis(uint dest_axis) {
		BP_ASSERT(ranges_[sub_transpose_[dest_axis]].size() == 1);
		sub_transpose_.erase(sub_transpose_.begin() + dest_axis);
	}


	void sub_none() {
		sub_transpose_.push_back(none_int);
	}

	/**
	 * AST subscription interface.
	 * Subscribe given axis by the index. Reduce the axis.
	 *
	 * TODO: Need to iteratively expand the sub_transpose.
	 * 'sub_index' must only set ranges_[axis] = {index}
	 * but should not append to the transpose.
	 *
	 */
	void sub_index(int index) {
		//std::cout << "sub_index: " << axis << index << "\n";
		uint src_axis = ranges_.size();
		BP_ASSERT(src_axis < full_shape_.size());
		uint range_size = full_shape_[src_axis];
		uint i_index = absolute_idx(index, range_size);
		ranges_.push_back( std::vector<uint>({i_index}));
	}


	/**
	 * AST subscription interface.
	 * Subscribe given axis by the range given by the list of generalized indices.
	 */
	void sub_range(std::vector<int> index_list) {
		//std::cout << "sub_range: " << axis << index_list[0] << "\n";
		uint src_axis = ranges_.size();
		BP_ASSERT(src_axis < full_shape_.size());
		uint range_size = full_shape_[src_axis];
		std::vector<uint> range;
		for(int idx : index_list) {
			range.push_back(absolute_idx(idx, range_size));
		}
		ranges_.push_back(range);
		sub_transpose_.push_back(src_axis);
	}

	/**
	 * AST subscription interface.
	 * Subscribe given axis by the range given by a slice.
	 */
	void sub_slice(Slice s) {
		//std::cout << "sub_slice: " << axis << s[0] << s[1] << s[2] << "\n";
		uint src_axis = ranges_.size();
		BP_ASSERT(src_axis < full_shape_.size());
		uint range_size = full_shape_[src_axis];
		std::vector<uint> range;
		int start=s[0], end=s[1], step=s[2];
		if (step == 0) {
			Throw() << "Slice step cannot be zero.";
		}

		int i_start, i_end;
		if (start == none_int) {
			i_start = step > 0 ? 0 : range_size - 1;
		} else {
			i_start = absolute_idx(start, range_size);
		}
		if (end == none_int) {
			i_end = step > 0 ? range_size : -1;
		} else {
			i_end   = absolute_idx(end, range_size);
		}
		if (step == none_int) {
			step = 1;
		}

		for(int idx = i_start; (i_end - idx) * step > 0; idx += step) {
			range.push_back(idx);
		}
		ranges_.push_back(range);
		sub_transpose_.push_back(src_axis);
	}

	/**
	 * Shift the range in given 'axis' by given 'shift'.
	 * TODO: return copy
	 */
	void shift_axis(uint axis, uint shift) {
		for(uint &el : ranges_[axis]) el +=shift;
	}

	/**
	 * repeat last element in 'axis' range
	 */
//		void pad(uint n_items, uint axis) {
//			ranges_[axis].insert(ranges_[axis].end(), n_items, ranges_[axis].back());
//		}


	/**
	 * Produce shape of the destination array.
	 */
	Shape sub_shape() const {
		Shape shape;
		uint size;
		for(auto t : sub_transpose_) {
			if (t == none_int) {
				size = 1;
			} else {
				size = ranges_[t].size();
			}
			shape.push_back(size);
		}
		return shape;
	}

	/// Shape of the source Array of the range.
	Shape full_shape_;
	/// For every source axes, indices extracted from the source Array.
	std::vector<std::vector<uint>> ranges_;
	/// Transpose maps destination axis to the source axis.
	/// none_int values are allowed indicating lifting e.g. from a vector to 1xN matrix.
	/// Size is equal to the number of destination axes.
	std::vector<uint> sub_transpose_;

};


/**
 * Multiindex
 * - internally stores multiindex to the source array
 * - can iterate over the source array, method inc_src
 * - can iterate over the destination array, method inc_dest
 * - provides linear index to both source (src_idx) and destination array (dest_idx)
 */
struct MultiIdx {

	typedef std::vector<uint> VecUint;

	/**
	 * Constructor.
	 * Set range, set multiindex to element zero.
	 */
	MultiIdx(MultiIdxRange range)
	: range_(range), src_indices_(range.ranges_.size(), 0), valid_(true)
	{
	}

	bool valid() {
		return valid_;
	}

	/**
	 * Increment the destination multiindex translated to the source multiindex. Last index runs fastest.
	 * Returns true if the increment was sucessfull, return false for the end of the range.
	 *
	 * Parameters:
	 * negative_axis - axis to which account the increment, 0 - means the last axis
	 * inc - increment step, can be negative
	 * carry - true ... carry to higher order axis
	 *       - false .. do not carry switch to the invalid state
	 * this->indices_ are set to zeros after the end of the iteration range.
	 */
	bool inc_dest(uint negative_axis=0, int inc=1, bool carry=true) {
		//std::cout << print_vector(src_indices_) << "\n";
		BP_ASSERT(valid_);
		BP_ASSERT(negative_axis <= range_.sub_transpose_.size());
		valid_=false;
		for(uint dest_axis = range_.sub_transpose_.size() - negative_axis; dest_axis > 0 ; )
		{
			--dest_axis;
			if (range_.sub_transpose_[dest_axis] == none_int) continue;
			uint src_axis = range_.sub_transpose_[dest_axis];
			int new_idx = src_indices_[src_axis] + inc;
			int bound = range_.ranges_[src_axis].size();
			if (0 < new_idx  && new_idx < bound) {
				src_indices_[src_axis] = new_idx;
				valid_ = true;
				break;
			} else {
				if (carry) {
					int reminder = (new_idx % bound + bound) % bound;
					src_indices_[src_axis] = reminder;
					inc = (new_idx - reminder) / bound;
				} else {
					src_indices_[src_axis] = new_idx;
					break;
				}
			}
		}
		return valid_; // false == all indeces_ set to zeros
	}



	/**
	 * Linear index into source array of the range.
	 * Last index runs fastest.
	 */
	uint src_idx() {
		uint lin_idx = 0;
		for(uint axis = 0; axis < src_indices_.size(); ++axis) {
			lin_idx *= range_.full_shape_[axis];
			lin_idx += range_.ranges_[axis][src_indices_[axis]];
		}
		return lin_idx;
	}

	/**
	 * Linear index into destination array of the range.
	 * Last index runs fastest.
	 */
	uint dest_idx() {
		uint lin_idx = 0;
		for(uint dest_axis = 0; dest_axis < range_.sub_transpose_.size(); ++dest_axis) {
			uint src_axis = range_.sub_transpose_[dest_axis];
			if (src_axis == none_int) continue;
			lin_idx *= range_.ranges_[src_axis].size();
			lin_idx += src_indices_[src_axis];
		}
		return lin_idx;
	}

	/// Just for test purposes.
	/// If necessary, we must implement indices_src and indeces_dest consistend with *_idx methods.
	const VecUint &indices() const {
		return src_indices_;
	}


	// List of indices for every axis.
	MultiIdxRange range_;
	/**
	 * Indirectindex into range_.
	 * Last index runs fastest.
	 */
	VecUint src_indices_;
	bool valid_;
};


/**
 *
 */
typedef std::vector<int> IList;




/**
 * Array nodes represents array operations (with array results) visible to user.
 * These are stored as Array of ScalarNodes, there are no derived classes to ArrayNode which simplifies implementation of operators.
 * We implement overloaded operators and other syntactic sugar in order to create the tree from the C++ code as well as from the parser.
 * This also makes operations from parser more readable.
 *
 * Operations:
 *  - elementwise: functions, operators, use common algorithm for expansion to the scalar nodes using related scalar nodes
 *  - stack - make a higher order from list of lower order
 *    stick - append new "column"
 *  - explicit broadcasting -
 */
struct Array {
public:
	typedef details::ScalarNodePtr ScalarNodePtr;

	inline static constexpr double none_value() {
		return std::numeric_limits<double>::signaling_NaN();
	}

	/**
	 * Static methods, mainly used for construction from AST.
	 */


	static Array deg_to_rad_factor() {
		static ScalarNodePtr angle = details::ScalarNode::create_const( boost::math::constants::pi<double>() / 180 );
		Array a;
		a.elements_[0] = angle;
		return a;
	}

	static Array rad_to_deg_factor() {
		static ScalarNodePtr angle = details::ScalarNode::create_const( boost::math::constants::pi<double>() / 180 );
		Array a;
		a.elements_[0] = angle;
		return a;
	}

	static Array empty_array(const Array &UNUSED(a)) {
		return Array();
	}

	static Array none_array(const Array &UNUSED(a)) {
		return Array();
	}

	static Array true_array(const Array &UNUSED(a)) {
		return constant({1});
	}

	static Array false_array(const Array &UNUSED(a)) {
		return constant({0});
	}

	template <class T>
	static Array unary_op(const Array &a) {
		Array result(a.shape_);
		MultiIdx idx(a.range());
		for(;;) {
			result[idx] = details::ScalarNode::create<T>(a[idx]);
			if (! idx.inc_dest()) break;
		}
		return result;
	}


	template <class T>
	static Array binary_op(const Array &a, const Array &b) {

		Shape res_shape = MultiIdxRange::broadcast_common_shape(a.shape_, b.shape_);
		MultiIdxRange a_range = a.range().broadcast(res_shape);
		MultiIdxRange b_range = b.range().broadcast(res_shape);

		MultiIdx a_idx(a_range);
		MultiIdx b_idx(b_range);
		Array result(res_shape);

		//std::cout << print_vector(a.shape());
		//std::cout << print_vector(b.shape());
		//std::cout << print_vector(result.shape());
		for(;;) {
			//std::cout << "ir=" << a_idx.dest_idx() << " ia=" << a_idx.src_idx() << " ib=" << b_idx.src_idx() << "\n";
			BP_ASSERT(a_idx.dest_idx() == b_idx.dest_idx());
			result.elements_[a_idx.dest_idx()] =
					details::ScalarNode::create<T>(
							a.elements_[a_idx.src_idx()],
							b.elements_[b_idx.src_idx()]);
			if (!a_idx.inc_dest() || !b_idx.inc_dest()) break;
		}
		return result;
	}


	static Array if_else(const Array &a, const Array &b, const Array&c) {
		Shape res_shape = MultiIdxRange::broadcast_common_shape(a.shape_, b.shape_);
		res_shape = MultiIdxRange::broadcast_common_shape(res_shape, c.shape_);
		MultiIdxRange a_range = a.range().broadcast(res_shape);
		MultiIdxRange b_range = b.range().broadcast(res_shape);
		MultiIdxRange c_range = c.range().broadcast(res_shape);

		MultiIdx a_idx(a_range);
		MultiIdx b_idx(b_range);
		MultiIdx c_idx(c_range);
		Array result(res_shape);
		for(;;) {
			BP_ASSERT(a_idx.dest_idx() == b_idx.dest_idx());
			BP_ASSERT(a_idx.dest_idx() == c_idx.dest_idx());
			result.elements_[a_idx.dest_idx()] =
					details::ScalarNode::create_ifelse(
							a.elements_[a_idx.src_idx()],
							b.elements_[b_idx.src_idx()],
							c.elements_[c_idx.src_idx()]);
			if (!a_idx.inc_dest() || !b_idx.inc_dest() || !c_idx.inc_dest()) break;
		}
		return result;

	}

	// Const scalar node.
	static Array constant(const std::vector<double> &values, Shape shape = {}) {
		if (values.size() == 1 && values[0] == none_value())
			return Array();

		Array res(shape);
		for(uint i_el=0; i_el < res.elements_.size(); ++i_el) {
			res.elements_[i_el] = details::ScalarNode::create_const(values[i_el]);
		}
		BP_ASSERT(values.size() == shape_size(res.shape()));
		return res;
	}


	// Vector value array with given shape
	static Array value(double *v, uint array_max_size, Shape shape = {})
	{
		Array res(shape);
		for(uint i_el=0; i_el < res.elements_.size(); ++i_el) {
			res.elements_[i_el] = details::ScalarNode::create_value(v);
			v += array_max_size;
		}
		return res;

//		uint i_el=0;
//		Shape idx(shape.size(), 0);
//		int ax = idx.size() - 1;
//		for(;;) {
//			res.elements_[i_el++] = ScalarNode::create_value(v);
//			v += array_max_size;
//			for(;;) {
//				if (ax < 0) goto exit;
//				idx[ax]++;
//				if (idx[ax] < shape[ax]) {
//					ax = 0;
//					break;
//				} else {
//					idx[ax] = 0;
//					ax--;
//				}
//			}
//		}
//		exit:
//		return res;
	}


	// Vector value array with given shape
	static Array value_copy(double *v, uint array_max_size, Shape shape = {})
	{
		Array res(shape);
		for(uint i_el=0; i_el < res.elements_.size(); ++i_el) {
			res.elements_[i_el] = details::ScalarNode::create_val_copy(v);
			v += array_max_size;
		}
		return res;
	}


	static Array stack_zero(const std::vector<Array> &list) {
		if (list.size() == 1) {
			return list[0].promote_in_axis(0);
		} else {
			return stack(list, 0);
		}
	}

	static Array stack(const std::vector<Array> &list, int axis=0) {
		/**
		 * Stack a list of M arrays of common shape (after broadcasting) (N0, ...)
		 * along given axis. Producing array of higher dimension of shape:
		 * (N0, ..., M, ...) where M is inserted the the 'axis' position.
		 *
		 * Axis can be negative to count 'axis' from the end of the common shape.
		 *
		 * List must have at least 1 item.
		 */
		if (list.size() == 0)
			Throw() << "stack: need at least one array";
		if (list.size() == 1) return list[0];
		// check for same shape
		for(auto ar : list)
			if (ar.shape() != list[0].shape()) {
				//std::cout << "a: " << print_vector(ar.shape()) << "b: " << print_vector(list[0].shape()) << "\n";
				Throw() << "stack: all input arrays must have the same shape";
			}

		// new shape
		Shape res_shape = list[0].shape_;
		if (axis < 0) axis = res_shape.size() + axis;
		BP_ASSERT(axis >= 0);
		res_shape.insert(res_shape.begin() + axis, list.size());

		// 3. broadcast arrays and concatenate
		Array result = Array(res_shape);

		for(uint ia=0; ia < list.size(); ++ia) {
			MultiIdxRange bcast_range = list[0].range();
			bcast_range.insert_axis(axis, axis);
			bcast_range.shift_axis(axis, ia);
			result.set_subset(bcast_range, list[ia]);
		}
		return result;
	}


	static Array append_to(const Array &a, const Array &b)
	{
		return a.append(b);
	}

	/**
	 * Stick a list of arrays along `axis`.
	 * Arrays must have same number of dimensions.
	 * The arrays must have same shape in remaining axes.
	 */
	static Array concatenate(const std::vector<Array> &list, uint axis=0) {

		if (list.size() == 0) return Array();
		if (list.size() == 1) return list[0];

		// TODO: iteratively prepare multi index ranges (can be modified in place)
		// then apply at once
		// Ranges - here we need to construct subranges for the new array
		// TODO:

		// determine resulting shape
		// 1. max shape length
		size_t max_len=1;
		for(auto ar : list) max_len = std::max(max_len, ar.shape_.size());
		// 2. max shape
		Shape res_shape(max_len, 1);
		std::vector<MultiIdxRange> list_range;
		uint axis_dimension = 0;
		for(auto ar : list) {
		    MultiIdxRange r = ar.range(); // temporary copy

		    // Propmote to common dimension.
		    uint sub_size = r.ranges_.size();
		    if ( sub_size < max_len -1) {
		    	std::ostringstream ss;
		    	ss << "Can not stick Array of shape" << print_shape(ar.shape())
		    		<< " along axis " << axis;
		    	Throw(ss.str());
		    }
		    if ( sub_size < max_len) {
		    	r.insert_axis(axis, axis);
		    }
			for(uint ax=0; ax < res_shape.size(); ++ax)
				res_shape[ax] = std::max(res_shape[ax], (uint)(r.ranges_[ax].size()));
			axis_dimension += r.ranges_[axis].size();
			list_range.push_back(r);
		}

		// final shape, sum along axis
		// first range, pad to final shape
		res_shape[axis] = axis_dimension;
		// 3. broadcast arrays and concatenate
		Array result = Array(res_shape);
		for(uint ia=1; ia < list.size(); ++ia) {
			Shape s = list_range[0].sub_shape();
			s[axis] = list[0].shape_[axis];
			MultiIdxRange r = list_range[0].broadcast(s);
			r.shift_axis(axis, 0);
			result.set_subset(r, list[0]);
		}
		return result;
	}


	/**
	 * Numpy.matmul:
	 *
	 * a 		has shape (..., i,j, k,l)
	 * b 		has shape (..., i,j, l,m)
	 * result 	has shape (..., i,j, k,m)
	 */
	static Array mat_mult(const Array &a,  const Array &b) {
		std::cout << "mat mult: " << print_vector(a.shape()) << " @ " << print_vector(b.shape()) << "\n";

		if (a.shape().size() == 0)
			Throw() << "Matmult can not multiply by scalar a." << "\n";
		if (b.shape().size() == 0)
			Throw() << "Matmult can not multiply by scalar b." << "\n";

		auto a_broadcast = MultiIdxRange(a.shape()).full();

		if (a.shape().size() == 1) {
			a_broadcast.insert_axis(0, 0, 1);
		}
		auto b_broadcast = MultiIdxRange(b.shape()).full();
		if (b.shape().size() == 1) {
			b_broadcast.insert_axis(1, 1, 1);
		}
		Shape a_shape = a_broadcast.full_shape_;
		//uint a_only_dim = *(a_shape.end() - 2);
		uint a_common_dim = *(a_shape.end() - 1);
		a_shape.insert(a_shape.end()-1, 1);
		// a_shape : (...,i,j,k,1,l)

		Shape b_shape = b_broadcast.full_shape_;
		//uint b_only_dim = *(b_shape.end() - 1);
		uint b_common_dim = *(b_shape.end() - 2);
		b_shape.insert(b_shape.end() - 2, 1);
		// b_shape : (...,i,j,1,l,m)
		if (a_common_dim != b_common_dim)
			Throw() << "Matmult summing dimension mismatch: " << a_common_dim << " != " << b_common_dim << "\n";
		Shape common_shape = MultiIdxRange::broadcast_common_shape(a_shape, b_shape);
		// common_shape : (...,i,j,k,l,m)
		Shape result_shape(common_shape);
		*(result_shape.end() - 2) = 1;
		// result_shape : (...,i,j,k,1,m)

		MultiIdxRange a_range = MultiIdxRange(a_shape).full().broadcast(common_shape);
		MultiIdxRange b_range = MultiIdxRange(b_shape).full().broadcast(common_shape);
		MultiIdxRange result_range = MultiIdxRange(result_shape).full().broadcast(common_shape);
		// transpose a_range and b_range in  order to have common_dim the last one:
		// a_shape : (....i,j, k,l,1)
		// b_shape : (...,i,j,1,m,l)
		*(b_range.sub_transpose_.end() - 1) = b_range.sub_transpose_.size() - 2;
		*(b_range.sub_transpose_.end() - 2) = b_range.sub_transpose_.size() - 1;


		MultiIdx a_idx(a_range);
		MultiIdx b_idx(b_range); // allocated
		MultiIdx result_idx(result_range);
		ScalarNodePtr sum;
		Array result(result_shape);
		for(;;) {
			a_idx.src_indices_ = result_idx.src_indices_;
			//*(a_idx.src_indices_.end() - 1) = 0;
			b_idx.src_indices_ = result_idx.src_indices_;
			//*(b_idx.src_indices_.end() - 3) = 0;
			//*(b_idx.src_indices_.end() - 2) = *(b_idx.src_indices_.end() - 1);
			//*(b_idx.src_indices_.end() - 1) = 0;

			sum = nullptr;
			for(uint l = 0; l < a_common_dim; l++) {
				std::cout << "  " << l << " < " << a_common_dim << "\n";
				std::cout << "  " << result_idx.src_idx() << print_vector(result_idx.src_indices_) << "\n";
				std::cout << "  " << a_idx.dest_idx() << print_vector(a_idx.src_indices_) << "\n";
				std::cout << "  " << b_idx.dest_idx() << print_vector(b_idx.src_indices_) << "\n";

				ScalarNodePtr mult = details::ScalarNode::create<details::_mul_>(
						a.elements_[a_idx.dest_idx()],
						b.elements_[b_idx.dest_idx()]); // error in src_idx
				if (sum == nullptr) {
					sum = mult;
				} else {
					// TODO: how to use inplace operations correctly ??
					sum = details::ScalarNode::create<details::_add_>(sum, mult);
				}
				a_idx.inc_dest();
				b_idx.inc_dest();
			}
			result.elements_[result_idx.src_idx()] = sum;
			if (!result_idx.inc_dest())
				break;

		}


		auto final_range = MultiIdxRange(result.shape()).full();

		std::cout << "  raw res: "<< print_vector(result_shape);
		if (b.shape().size() == 1 && *(result_shape.end() - 1) == 1 ) {
		    // cut -1 axis
			std::cout << "  b cut: "<< result_shape.size()-1 << "\n";
			final_range.remove_destination_axis(result_shape.size()-1);
		}
		BP_ASSERT( *(result_shape.end() - 2) == 1);
		std::cout << "  r cut: "<< result_shape.size()-2 << "\n";
		final_range.remove_destination_axis(result_shape.size()-2);
		    // cut -2 axis always
		if (a.shape().size() == 1 && *(result_shape.end() - 3) == 1 ) {
			// cut -3 axis
			std::cout << "  a cut: "<< result_shape.size()-3 << "\n";
			final_range.remove_destination_axis(result_shape.size()-3);
		}
		std::cout << "  final res: " << print_vector(final_range.sub_shape()) << "\n";
		return Array(result, final_range);

	}

	static Array flatten(const Array &tensor) {
		uint n_elements = shape_size(tensor.shape());
		Shape res_shape(1, n_elements);
		Array result(res_shape);
		for(uint i = 0; i<n_elements; ++i) {
			result.elements_[i] = tensor.elements_[i];
		}
		return result;
	}

	static Array eye(int size) {
		if (size < 1)
				Throw() << "eye works only for positive sizes.";
		Shape shape(2, size);
		Array result = Array(shape);
		MultiIdxRange result_range = MultiIdxRange(shape).full();
		MultiIdx result_idx(result_range);
		for(MultiIdx result_idx(result_range); result_idx.valid(); result_idx.inc_dest()) {
			if (result_idx.indices()[0]  == result_idx.indices()[1]) {
				result.elements_[result_idx.src_idx()] = details::ScalarNode::create_one();
			} else {
				result.elements_[result_idx.src_idx()] = details::ScalarNode::create_zero();
			}
		}
		return result;
	}


	static Array zeros(const Shape &shape) {
		return full_(shape, details::ScalarNode::create_zero());
	}

	static Array ones(const Shape &shape) {
		return full_(shape, details::ScalarNode::create_one());
	}

	static Array full(const Shape &shape, double val) {
		return full_(shape, details::ScalarNode::create_const(val));
	}

	static Array full_(const Shape &shape, details::ScalarNodePtr  val) {
		Array result = (shape);
		MultiIdxRange result_range = MultiIdxRange(shape).full();
		MultiIdx result_idx(result_range);
		for(MultiIdx result_idx(result_range); result_idx.valid(); result_idx.inc_dest()) {
					result.elements_[result_idx.src_idx()] = val;
		}
		return result;
	}


//	static Array slice(const Array &a, const Array &b, const Array&c) {
//		Array res = concatenate({a,b,c});
//		return res;  // TODO: make valid implementation
//	}

//	static Array subscribe(const Array &a, const Array &slice) {
//		Array res = concatenate({a,slice});
//		return res;  // TODO: make valid implementation
//	}

	/**
	 * Constructors.
	 */

	Array()
	: shape_(), elements_()
	{}

	Array(const Array &other)
	: shape_(other.shape_),
	  elements_(other.elements_)
	{}

	/**
	 * Empty array of given shape.
	 */
	Array(const Shape &shape)
	: shape_(shape), elements_({nullptr})
	{
		uint shape_prod = 1;
		for(uint dim : shape) shape_prod *= dim;
		elements_.resize(shape_prod, nullptr);
	}

	/**
	 * Construct destination array from a source array
	 * and the multiindex mapping.
	 */
	Array(const Array &source, const MultiIdxRange &range)
	: Array(range.sub_shape())
	{
		from_subset(source, range);
	}

	~Array() {
	}

	Array operator=(const Array & other)
	{
		elements_ = other.elements_;
		shape_ = other.shape_;
		return *this;
	}



	MultiIdxRange range() const {
		return MultiIdxRange(shape_).full();
	}

	ScalarNodePtr &operator[](MultiIdx idx) {
		return elements_[idx.src_idx()];
	}

	ScalarNodePtr operator[](MultiIdx idx) const {
		return elements_[idx.src_idx()];
	}

	const std::vector<ScalarNodePtr> &elements() const {
		return elements_;
	}

	const Shape &shape() const {
		return shape_;
	}


	/**
	 * Return copy of *this.
	 * promoted to higher tensor of size 1. E.g. x -> [x], [x,y] -> [[x],[y]], etc.
	 * Insert given `axis` of size 1 into the shape. That makes no change in elements_.
	 */
	Array promote_in_axis(int axis = 0) const {
		uint u_axis;
		try {
			u_axis = absolute_idx(axis, shape_.size() + 1);
		} catch (Exception &e) {
			e << "Can not promote axis: " << axis << "\n";
			throw e;
		}
		auto promote_range = MultiIdxRange(shape_).full();
		promote_range.insert_axis(u_axis, u_axis, 1);
		return Array(*this, promote_range);
	}

	/// Create a result array from *this using storage of 'variable'.
	Array make_result(const Array &variable) {
		Array res(shape_);
		for(uint i=0; i<elements_.size(); ++i)
			res.elements_[i] = details::ScalarNode::create_result(elements_[i], variable.elements_[i]->values_);
		return res;
	}

	Shape minimal_shape(Shape other) {
		Shape result;
		for(uint e : other)
			if (e > 1) result.push_back(e);
		return result;
	}


	void _set_shape(Shape shape) {
		shape_ = shape;
		uint shape_prod = 1;
		for(uint dim : shape) shape_prod *= dim;
		elements_.resize(shape_prod);
	}





//	// Const 1D array node.
//	// TODO: do better possibly without transform it is unable to resolve overloaded methods
//	static Array constant1(const std::vector<double> &list)
//	{
//		std::vector<Array> ar_list;
//		std::transform(list.begin(), list.end(), std::back_inserter(ar_list), Array::constant);
//		return Array::stack(ar_list);
//	}
//
//	// Const 2D array node.
//	static Array constant2(const std::vector<std::vector<double>> &list)
//	{
//		std::vector<Array> ar_list;
//		std::transform(list.begin(), list.end(), std::back_inserter(ar_list), Array::constant1);
//		return Array::stack(ar_list);
//	}




	bool is_none() const {
		// shape == {} means a scalar
		return shape_.size() == 0 && elements_.size() == 0;
	}

	/**
	 * Append 'slice' array to 'axis' of 'this' Array.
	 * 'slice' must have compatible shape, matching shape in all other axes of 'this'.
	 */
	Array append(const Array &slice, int axis = 0) const
	{
		auto p_slice = slice.promote_in_axis(axis);
		if (is_none()) {
			return p_slice;
		} else {
			return Array::concatenate({*this, p_slice}, axis);
		}
	}




	/**
	 * - if this->shape_[i] == 1 the array is repeated in that axis according to other[i]
	 * - this->shape_ is padded from the left by ones
	 * - final array must have same shape as other
	 */
//	Array broadcast(Shape other) {
//		MultiIdxRange bcast_range = range().broadcast(other);
//		return Array(*this, bcast_range);
//	}

	/**
	 * Need indexing syntax, preferably close to the Python syntax selected for the parser:
	 *
	 * Python				C++ interface
	 * a[0, 1]      		a(0, 1)
	 * a[[0,1,3], 3]		a({0,1,3}, 3)
	 * a[:, 3]				a(Slice(), 3)
	 * a[1:, 3]				a(Slice(1), 3)
	 * a[:-1, 3]		    a(Slice(0, -1), 3)
	 * a[::2, 3]		    a(Slice(0, None, 2), 3)
	 *
	 * a[None, :]			a(None, Slice())
	 *
	 * x  if cond else y
	 * max
	 * min
	 *
	 * TODO:
	 * 1. variadic arguments for the operator() have to be converted to a vector of IndexSubsets
	 * 2. same we get from the parser
	 * 3. Index Subsets (with known sub_shape) converted to the vector of index sets, the we use the MultiIdx to iterate over
	 *  own and other Array.
	 *
	 *  Not clear how to apply operations.
	 */

//	 void set_subarray_(MultiIdx idx_out, MultiIdx idx_in, Array other, in_subset, ...) {
//
//	 }

private:



	/**
	 * Assign the 'other' array to the subset of *this given by the multiindex mapping 'range'.
	 *
	 * Set subset of *this given by the range to values given by 'other'.
	 * The shape of range have length same as 'shape_' but non 1 dimensions have to match
	 * shape of other.
	 */
	void set_subset(const MultiIdxRange &range, const Array &other) {
		Shape sub_shp = range.sub_shape();
		Shape min_shape_a = minimal_shape(sub_shp);
		Shape min_shape_b = minimal_shape(other.shape_);
		//std::cout << "a: " << print_vector(min_shape_a) << "b: " << print_vector(min_shape_b) << "\n";
		BP_ASSERT(min_shape_a == min_shape_b);
		MultiIdx idx(range);
		for(;;) {
			elements_[idx.src_idx()] = other.elements_[idx.dest_idx()];
			if (! idx.inc_dest()) break;
		}
	}

	/**
	 * Fill all elements of *this to the subset of 'other' given by the 'range'.
	 */
	void from_subset(const Array &other, const MultiIdxRange &range) {
		if (other.elements_.size() == 0) return; // none value
		MultiIdx idx(range);
		for(;;) {
			elements_[idx.dest_idx()] = other.elements_[idx.src_idx()];
			if (! idx.inc_dest()) break;
		}
	}


	Shape shape_;
	std::vector<ScalarNodePtr> elements_;

};



template <class Fn>
Array func(const Array &x) {
	return Array::unary_op<Fn>(x);
}

/**
 * Special function
 */
inline Array deg_fn(const Array & rad) {
	return Array::binary_op<details::_mul_>(rad, Array::deg_to_rad_factor());
}

inline Array rad_fn(const Array & deg) {
	return Array::binary_op<details::_mul_>(deg, Array::rad_to_deg_factor());
}

inline Array gt_op(const Array & a, const Array & b) {
	return Array::binary_op<details::_lt_>(b, a);
}

inline Array ge_op(const Array & a, const Array & b) {
	return Array::binary_op<details::_le_>(b, a);
}

inline Array unary_plus(const Array & a) {
	return a;
}

inline Array floor_div(const Array & a, const Array & b) {
	Array r1 = Array::binary_op<details::_div_>(a, b);
	return Array::unary_op<details::_floor_>(r1);
}

inline Array operator+(const Array &a,  const Array &b) {
	return Array::binary_op<details::_add_>(a, b);
}

inline Array operator*(const Array &a,  const Array &b) {
	return Array::binary_op<details::_mul_>(a, b);
}

/*
% Given a real symmetric 3x3 matrix A, compute the eigenvalues
% Note that acos and cos operate on angles in radians

p1 = A(1,2)^2 + A(1,3)^2 + A(2,3)^2
if (p1 == 0)
   % A is diagonal.
   eig1 = A(1,1)
   eig2 = A(2,2)
   eig3 = A(3,3)
else
   q = trace(A)/3               % trace(A) is the sum of all diagonal values
   p2 = (A(1,1) - q)^2 + (A(2,2) - q)^2 + (A(3,3) - q)^2 + 2 * p1
   p = sqrt(p2 / 6)
   B = (1 / p) * (A - q * I)    % I is the identity matrix
   r = det(B) / 2

   % In exact arithmetic for a symmetric matrix  -1 <= r <= 1
   % but computation error can leave it slightly outside this range.
   if (r <= -1)
      phi = pi / 3
   elseif (r >= 1)
      phi = 0
   else
      phi = acos(r) / 3
   end

   % the eigenvalues satisfy eig3 <= eig2 <= eig1
   eig1 = q + 2 * p * cos(phi)
   eig3 = q + 2 * p * cos(phi + (2*pi/3))
   eig2 = 3 * q - eig1 - eig3     % since trace(A) = eig1 + eig2 + eig3
end
 */

} // bparser

#endif /* INCLUDE_ARRAY_HH_ */
