/*
 * test_speed_parser.cc
 *
 *  Created on: Dec 28, 2022
 *      Author: KKSSainz
 */


#include <string>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>

#include "assert.hh"
#include "parser.hh"
#include "test_tools.hh"

#include "arena_alloc.hh"

// Optimized structure, holds data in common arena
struct ExprData {
	ExprData(uint vec_size, uint simd_size)
	: vec_size(vec_size), simd_size(simd_size)
	{
		uint simd_bytes = sizeof(double) * simd_size;

		arena = std::make_shared<bparser::ArenaAlloc>(simd_bytes, 512 * 1012);
		v1 = arena->create_array<double>(vec_size * 3);
		fill_seq(v1, 100, 100 + 3 * vec_size);
		v2 = arena->create_array<double>(vec_size * 3);
		fill_seq(v2, 200, 200 + 3 * vec_size);
		v3 = arena->create_array<double>(vec_size * 3);
		fill_seq(v3, 300, 300 + 3 * vec_size);
		v4 = arena->create_array<double>(vec_size * 3);
		fill_seq(v4, 400, 400 + 3 * vec_size);
		vres = arena->create_array<double>(vec_size * 3);
		fill_const(vres, 3 * vec_size, -100);
		subset = arena->create_array<uint>(vec_size);
		for(uint i=0; i<vec_size/simd_size; i++) subset[i] = i;
		cs1 = 4;
		for (uint i = 0; i < 3; i++)
		{
			cv1[i] = (i+1)*3;
		}
	}	

	~ExprData()
	{}

	std::shared_ptr<bparser::ArenaAlloc> arena;
	uint vec_size;
	uint simd_size;
	double *v1, *v2, *v3, *v4, *vres;
	double cs1;
	double cv1[3];
	uint *subset;

};

class ParserTest
: public bparser::Parser {
public:
	ParserTest(uint max_vec_size, uint simd_size_)
	: Parser(max_vec_size)
	{
		simd_size = simd_size_;
	}
};



/**
 * @param expr       Parser expression
 * @param block_size Number of floats
 * @param i_expr     Specifies C++ expression function
 */
void test_expr(std::string expr, uint block_size, uint simd_size, std::string expr_id, std::ofstream& file) {
	using namespace bparser;
	uint vec_size = 1*block_size;

	// TODO: allow changing variable pointers, between evaluations
	// e.g. p.set_variable could return pointer to that pointer
	// not so easy for vector and tensor variables, there are many pointers to set
	// Rather modify the test to fill the
	uint n_repeats = (1024 / block_size) * 10000;

	ExprData  data(vec_size, simd_size);

	double parser_time;

	{ // one allocation in common arena, set this arena to processor
		ParserTest p(block_size, simd_size);
		p.parse(expr);
		p.set_constant("cs1", {}, 	{data.cs1});
		p.set_constant("cv1", {3}, 	std::vector<double>(data.cv1, data.cv1+3));
		p.set_variable("v1", {3}, data.v1);
		p.set_variable("v2", {3}, data.v2);
		p.set_variable("v3", {3}, data.v3);
		p.set_variable("v4", {3}, data.v4);
		p.set_variable("_result_", {3}, data.vres);
		//std::cout << "vres: " << vres << ", " << vres + block_size << ", " << vres + 2*vec_size << "\n";
		//std::cout << "Symbols: " << print_vector(p.symbols()) << "\n";
		//std::cout.flush();
		p.compile(data.arena);

		std::vector<uint> ss = std::vector<uint>(data.subset, data.subset+vec_size/simd_size);
		p.set_subset(ss);
		auto start_time = std::chrono::high_resolution_clock::now();
		for(uint i_rep=0; i_rep < n_repeats; i_rep++) {
			p.run();
		}
		auto end_time = std::chrono::high_resolution_clock::now();
		parser_time = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
	}

	// check
	double p_sum = 0;
	for(uint dim=0; dim < 3; dim++) {
		for(uint i=0; i<data.vec_size; i++) {
			double v1 = data.vres[dim*data.vec_size + i];
			//std::cout << "res: " << v1 <<std::endl;
			p_sum += v1;
		}
	}
	double n_flop = n_repeats * vec_size * 9;

	// std::cout << "=== Parsing of expression: '" << expr << "' ===\n";
	// std::cout << "Repeats    	: " << n_repeats << "\n";
	// std::cout << "Result	  		: " << p_sum << "\n";
	// std::cout << "Block size		: " << block_size << "\n";
	// std::cout << "Experession id	: " << expr_id << "\n";
	// std::cout << "Parser time 	: " << parser_time << "\n";
	// std::cout << "Parser FLOPS	: " << n_flop / parser_time_shared_arena << "\n";
	// std::cout << "======================================================\n\n";

	std::string processor_type = "";

	switch (simd_size) {
		case 2:
		{
			processor_type = "SSE";
		} break;
		case 4:
		{ 
			processor_type = "AVX2";
		} break;
		case 8:
		{
			processor_type = "AVX512";
		} break;
		default:
		{
			processor_type = "no vectorization";
		} break;
	}

	file << "BParser " + processor_type << ";"<< expr_id << ";"<< expr << ";"<< p_sum << ";"<< n_repeats << ";"<< block_size << ";" << parser_time << ";" << parser_time/n_repeats/vec_size*1e9 << ";" << n_flop/parser_time << "\n";

}


void test_expression(std::string filename, uint simd_size)  {
	std::vector<uint> block_sizes = {64, 256, 1024};

	if(simd_size == 0){
		simd_size = bparser::get_simd_size();
	}

	std::ofstream file;
	if(!filename.empty())
	{
		file.open(filename);
		std::cout << "Outputing to " << filename << "\n";
	}
	else
	{
		file.open("BParser_vystup.csv");
	}

	//header
	file << "Executor;ID;Expression;Result;Repeats;BlockSize;Time;Avg. time per single execution;FLOPS\n";

	for (uint i=0; i<block_sizes.size(); ++i) {
		uint id_counter = 0;
		test_expr("-v1", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++) + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 + v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 - v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 * v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 / v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 % v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);

		test_expr("(v1 == v2)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 != v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 < v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 <= v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 > v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 >= v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("not (v1 == v2)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 or v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 and v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);

		test_expr("abs(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("sqrt(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("exp(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("log(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("log10(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);

		test_expr("sin(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("sinh(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("asin(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("cos(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("cosh(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("acos(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("tan(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("tanh(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("atan(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("ceil(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("floor(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("isnan(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("isinf(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("sgn(v1)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("atan2(v1, v2)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);

		test_expr("v1 ** v2", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("maximum(v1, v2)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("minimum(v1, v2)", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v3 if v1 == v2 else v4", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("cv1 + v1", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("cs1 - v1", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("cs1 * v1", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("cv1 / v1", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("v1 + v2 + v3 + v4", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("3 * v1 + cs1 * v2 + v3 + 2.5 * v4", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
		test_expr("[v2, v2, v2] @ v1 + v3", block_sizes[i], simd_size, "TEST_" + std::to_string(id_counter++)  + "_" + std::to_string(block_sizes[i]), file);
	}
}



int main(int argc, char * argv[])
{
	std::string file = "";
	uint simd_size = 0;
	if(argc > 1)
	{
		file = argv[1]; 
		simd_size = atoi(argv[2]);
	}
	test_expression(file, simd_size);
}
