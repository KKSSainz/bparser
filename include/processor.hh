/*
 * processor.hh
 *
 *  Created on: Dec 29, 2019
 *      Author: jb
 */

#ifndef INCLUDE_PROCESSOR_HH_
#define INCLUDE_PROCESSOR_HH_

#include <stdint.h>
#include <malloc.h>
#include <vector>
#include "config.hh"
#include "assert.hh"
#include "arena_alloc.hh"
#include "scalar_expr.hh"

namespace bparser {
using namespace details;

/**
 *
 */
//class Workspace {
//public:
//	// Size of the single vector operation. E.g. 4 doubles for AVX2.
//	static const uint simd_block_size = 4;
//
//	Workspace(uint vec_size, uint n_vectors, uint n_constants)
//	: Vec_size_(0)
//	{
//		workspace_size_ = 64;
//		workspace_ = new double[workspace_size_];
//		clear();
//	}
//
//	~Workspace() {
//		delete [] workspace_;
//	}
//
//
//	static void set_subset(std::initializer_list<uint> subset, uint Vec_size) {
//		Workspace::instance().set_subset_(std::vector<uint>(subset), Vec_size);
//	}
//
//	// Set new subset structure and size of the full Vec.
//	static void set_subset(const std::vector<uint> &subset, uint Vec_size) {
//		Workspace::instance().set_subset_(subset, Vec_size);
//	}
//
//	static void set_workspace(uint n_doubles) {
//		Workspace::instance().set_workspace_(n_doubles);
//	}
//
//	// Release all temporary slots.
//	static void clear() {
//		Workspace::instance().next_slot_ = Workspace::instance().workspace_;
//	}
//
//	static uint size() {
//		return Workspace::instance().subset_.size();
//	}
//
//	static double * get_slot()  {
//		return Workspace::instance().get_slot_();
//	}
//
//
//protected:
//	static inline Workspace &instance() {
//		static Workspace w;
//		return w;
//	}
//
//	inline void set_subset_(const std::vector<uint> &subset, uint Vec_size) {
//		clear();
//		Vec_size_ = Vec_size;
//		subset_ = subset; // TODO: avoid copy
//		const_subset_.reserve(size());
//		flat_subset_.reserve(size());
//		for(uint i=0; i<size(); ++i) {
//			const_subset_[i] = 0;
//			flat_subset_[i] = i;
//		}
//	}
//
//	void set_workspace_(uint n_doubles) {
//		delete [] workspace_;
//		workspace_size_ = n_doubles;
//		workspace_ = new double[n_doubles];
//	}
//
//	double * get_slot_()  {
//		ASSERT(next_slot_ < workspace_ + workspace_size_);
//		double *ptr = next_slot_;
//		next_slot_ += size();
//		return ptr;
//	}
//
//
//	static uint * subset() {
//		return &(Workspace::instance().subset_[0]);
//	}
//
//	static uint * flat_subset() {
//		return &(Workspace::instance().flat_subset_[0]);
//	}
//
//	static uint * const_subset() {
//		return &(Workspace::instance().const_subset_[0]);
//	}
//
//
//	std::vector<uint > const_subset_;
//	std::vector<uint > flat_subset_;
//	std::vector<uint > subset_;
//	uint Vec_size_;
//
//	double *workspace_;
//	uint workspace_size_;
//	double *next_slot_;
//};

const uint simd_size = 4;
typedef double double4 __attribute__((__vector_size__(32)));


struct Vec {
	double4 *values;
	uint *subset;

	void set(double4 * v, uint * s) {
		values = v;
		subset = s;
	}

	double & value(uint i, uint j) {
		//std::cout << "i: " << i << "j: " << j << " si: " << subset[i] << " v: " << values[subset[i]][j] << "\n";
		return values[subset[i]][j];
	}
};

/**
 * Processor's storage.
 */
struct Workspace {
	uint vector_size;

	Vec *vector;
	uint subset_size;

	uint *const_subset;
	uint *vec_subset;

};


/**
 * Memory aligned representation of single operation.
 */
struct Operation {
	// Op code. See scalar_expr.hh: XYZNode::op_code;
	unsigned char code;
	// index of arguments in the Processors's workspace
	unsigned char arg[3];
};



template<uint NParams, class T>
struct EvalImpl;
//{
//	static inline void eval(Operation op, Workspace &w) {};
//};

template <class T>
struct EvalImpl<1, T> {
	static void eval(Operation op,  Workspace &w) {
		Vec v0 = w.vector[op.arg[0]];
		for(uint i=0; i<w.subset_size; ++i)
			for(uint j=0; j<simd_size; ++j)
				T::eval(v0.value(i, j));
	}
};


template <class T>
struct EvalImpl<2, T> {
	static void eval(Operation op,  Workspace &w) {
		Vec v0 = w.vector[op.arg[0]];
		Vec v1 = w.vector[op.arg[1]];
		for(uint i=0; i<w.subset_size; ++i)
			for(uint j=0; j<simd_size; ++j) {
				double & a0 = v0.value(i, j);
				double & a1 = v1.value(i, j);
				T::eval(a0, a1);
			}
	}
};


template <class T>
struct EvalImpl<3, T> {
	static void eval(Operation op,  Workspace &w) {
		Vec v0 = w.vector[op.arg[0]];
		Vec v1 = w.vector[op.arg[1]];
		Vec v2 = w.vector[op.arg[2]];
		for(uint i=0; i<w.subset_size; ++i)
			for(uint j=0; j<simd_size; ++j)
				T::eval(v0.value(i, j), v1.value(i, j), v2.value(i, j));
	}
};




#define CODE(OP_NAME) \
	case (OP_NAME::op_code): operation_eval<OP_NAME>(*op); break

// Note: Internal operations are at most binary, N-ary operations are decomposed into simpler.

struct ProcessorSetup {
	uint vector_size;
	uint n_operations;
	uint n_vectors;
	uint n_constants;
};



/**
 * Store and execute generated "bytecode".
 */
struct Processor {
	/**
	 *
	 * vector_size: maximum vector size in doubles
	 *
	 * TODO: reimplement full__ns to perform topological sort of nodes
	 * TODO: enclose global expression manipulations into a class ScalarExpression
	 * - topological sort
	 * - make_node<> can be its method
	 * - destruction
	 * - extract allocation info
	 * - dependencies
	 * - assigne result ids
	 * - create processor
	 */
	static Processor *create(std::vector<ScalarNode *> results, uint vector_size) {
		ScalarExpression se(results);
		return create_processor_(se, vector_size);
	}


	static Processor *create_processor_(ScalarExpression &se, uint vector_size) {
		vector_size = (vector_size / simd_size) * simd_size;
		uint simd_bytes = sizeof(double) * simd_size;
		ScalarExpression::NodeVec & sorted_nodes = se.sort_nodes();
		//std::cout << "n_nodes: " << sorted_nodes.size() << " n_vec: " << se.n_vectors() << "\n";
		uint memory_est =
				align_size(simd_bytes, sizeof(Processor)) +
				align_size(simd_bytes, sizeof(Operation) * (sorted_nodes.size() + 4) ) +
				sizeof(double) * vector_size * (se.n_vectors() + 4);
		ArenaAlloc arena(simd_bytes, memory_est);
		return arena.create<Processor>(arena, se, vector_size / simd_size);
	}

	/**
	 * Do not create processor directly, use the create static function.
	 *
	 * vec_size : number of simd blocks (double4).
	 */
	Processor(ArenaAlloc arena, ScalarExpression &se, uint vec_size)
	: arena_(arena)
	{
		workspace_.vector_size = vec_size;
		workspace_.subset_size = 0;
		workspace_.const_subset = arena_.create_array<uint>(vec_size);
		for(uint i=0; i<vec_size;++i) workspace_.const_subset[i] = 0;
		workspace_.vec_subset = (uint *) arena_.allocate(sizeof(uint) * vec_size);
		//std::cout << "&vec_subset: " << &(workspace_.vec_subset) << "\n";
		//std::cout << "aloc vec_subset: " << workspace_.vec_subset << " size: " << vec_size << "\n";
		workspace_.vector = (Vec *) arena_.allocate(sizeof(Vec) * se.n_vectors());
		uint n_temporaries = (se.n_vectors() - se.n_constants - se.n_values);
		double4 * temp_base = (double4 *) arena_.allocate(sizeof(double) * simd_size * vec_size * n_temporaries);
		double4 * temp_ptr = temp_base;
		double4 * const_base = (double4 *) arena_.allocate(sizeof(double) * simd_size * se.n_constants);
		double4 * const_ptr = const_base;
		uint n_operations = se.sorted.size();
		program_ = (Operation *) arena_.allocate(sizeof(Operation));

		Operation *op = program_;
		for(auto it=se.sorted.rbegin(); it != se.sorted.rend(); ++it) {
			ScalarNode * node = *it;
			switch (node->result_storage) {
			case constant: {
				ASSERT(const_ptr < const_base + se.n_constants);
				double c_val = *node->get_value();
				for(uint j=0; j<simd_size; ++j)
					const_ptr[0][j] = c_val;
				workspace_.vector[node->result_idx_].set(const_ptr, workspace_.const_subset);
				const_ptr++;
				continue;}
			case value:
				workspace_.vector[node->result_idx_].set((double4 *)node->get_value(), workspace_.vec_subset);
				continue;
			case temporary:
				ASSERT(temp_ptr < temp_base + vec_size *n_temporaries);
				workspace_.vector[node->result_idx_].set(temp_ptr, workspace_.vec_subset);
				temp_ptr += vec_size;
				*op = make_operation(node);
				++op;
				break;
			case none:
				*op = make_operation(node);
				++op;
				break;
			case expr_result:
				ASSERT(node->n_inputs_ == 1);
				ScalarNode * prev_node = node->inputs_[0];
				ASSERT(prev_node->result_storage == temporary);
				workspace_.vector[prev_node->result_idx_].set((double4 *)node->get_value(), workspace_.vec_subset);
//				std::cout << " ir: " << prev_node->result_idx_ << " a0: "
//						<< workspace_.vector[prev_node->result_idx_].values
//						<< " gv: " << node->get_value()
//						<< "\n";
				continue;
			}
			ASSERT(op < program_ + n_operations);
		}
		op->code = ScalarNode::terminate_op_code;


	}

	~Processor() {
		arena_.destroy();
	}

	Operation make_operation(ScalarNode * node) {
		Operation op;
		op.code = node->op_code_;
		uint i_arg = 0;
		if (node->result_storage == temporary)
			op.arg[i_arg++] = node->result_idx_;
		for(uint j=0; j<node->n_inputs_; ++j)
			op.arg[i_arg++] = node->inputs_[j]->result_idx_;
		return op;
	}


	template<class T>
	inline void operation_eval(Operation op) {
		EvalImpl<T::n_eval_args, T>::eval(op, workspace_);
	}

	void run() {
		for(Operation * op = program_;;++op) {
//			std::cout << "op: " << (int)(op->code)
//					<< " ia0: " << (int)(op->arg[0])
//					<< " a0: " << workspace_.vector[op->arg[0]].values
//					<< " ia1: " << (int)(op->arg[1])
//					<< " a1: " << workspace_.vector[op->arg[1]].values << "\n";

			switch (op->code) {
			CODE(_minus_);
			CODE(_add_);
			CODE(_sub_);
			CODE(_mul_);
			CODE(_div_);
			CODE(_mod_);
			CODE(_eq_);
			CODE(_ne_);
			CODE(_lt_);
			CODE(_le_);
			CODE(_neg_);
			CODE(_or_);
			CODE(_and_);
			CODE(_abs_);
			CODE(_sqrt_);
			CODE(_exp_);
			CODE(_log_);
			CODE(_log10_);
			CODE(_sin_);
			CODE(_sinh_);
			CODE(_asin_);
			CODE(_cos_);
			CODE(_cosh_);
			CODE(_acos_);
			CODE(_tan_);
			CODE(_tanh_);
			CODE(_atan_);
			CODE(_ceil_);
			CODE(_floor_);
			CODE(_isnan_);
			CODE(_isinf_);
			CODE(_sgn_);
			CODE(_copy_);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
//			CODE(__);
			case (ScalarNode::terminate_op_code): return; // terminal operation
			}
		}
	}

	// Set subset indices of active double4 blocks.
	void set_subset(std::vector<uint> subset)
	{
		workspace_.subset_size = subset.size();
		//std::cout << "vec_subset: " << workspace_.vec_subset << "\n";
		for(uint i=0; i<workspace_.subset_size; ++i) {
			//std::cout << "vec_i: " << workspace_.vec_subset + i << " " << i << "\n";
			workspace_.vec_subset[i] = subset[i];
		}
	}

	ArenaAlloc arena_;
	Workspace workspace_;
	Operation * program_;
};



} // bparser namespace



#endif /* INCLUDE_PROCESSOR_HH_ */
